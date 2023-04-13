#include <algorithm>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef LOCAL
#define log if (true) std::cerr
#else
#define log if (false) std::cerr
#endif

struct Machine {
    std::vector<int> weekDayPatterns;
    std::vector<int> weekEndPatterns;

    std::vector<double> weekDayPatternCosts;
    std::vector<double> weekEndPatternCosts;

    std::vector<double> loads;
    std::vector<int> noDelays;
};

struct State {
    std::vector<Machine> machines;

    long score = 0;
    int noViolations = 0;
    int noDelays = 0;
};

enum class OptimizationPartType {
    WEEK_DAY,
    WEEK_END
};

struct OptimizationPart {
    int machine;
    int week;
    OptimizationPartType type;
    int from;
    int to;
    double costImprovement;

    static OptimizationPart weekDay(const State &state, int machine, int week, int newPattern) {
        return {
                machine,
                week,
                OptimizationPartType::WEEK_DAY,
                state.machines[machine].weekDayPatterns[week],
                newPattern,
                state.machines[machine].weekDayPatternCosts[state.machines[machine].weekDayPatterns[week] - 1]
                - state.machines[machine].weekDayPatternCosts[newPattern - 1]
        };
    }

    static OptimizationPart weekEnd(const State &state, int machine, int week, int newPattern) {
        return {
                machine,
                week,
                OptimizationPartType::WEEK_END,
                state.machines[machine].weekEndPatterns[week],
                newPattern,
                state.machines[machine].weekEndPatternCosts[state.machines[machine].weekEndPatterns[week] - 1]
                - state.machines[machine].weekEndPatternCosts[newPattern - 1]
        };
    }

    void apply(State &state) const {
        if (type == OptimizationPartType::WEEK_DAY) {
            state.machines[machine].weekDayPatterns[week] = to;
        } else {
            state.machines[machine].weekEndPatterns[week] = to;
        }
    }

    void undo(State &state) const {
        if (type == OptimizationPartType::WEEK_DAY) {
            state.machines[machine].weekDayPatterns[week] = from;
        } else {
            state.machines[machine].weekEndPatterns[week] = from;
        }
    }
};

struct Optimization {
    std::string id;
    std::string name;
    double costImprovement;
    std::vector<OptimizationPart> parts;

    explicit Optimization(std::string name, std::vector<OptimizationPart> parts)
            : name(std::move(name)), parts(std::move(parts)) {
        std::stringstream idStream;
        costImprovement = 0.0;

        for (int i = 0; i < this->parts.size(); i++) {
            const auto &part = this->parts[i];

            idStream << part.machine
                     << "-" << part.week
                     << "-" << (int) part.type
                     << "-" << part.from
                     << "-" << part.to;

            costImprovement += part.costImprovement;

            if (i != this->parts.size() - 1) {
                idStream << "_";
            }
        }

        id = idStream.str();
    }

    void apply(State &state) const {
        for (const auto &part : parts) {
            part.apply(state);
        }
    }

    void undo(State &state) const {
        for (const auto &part : parts) {
            part.undo(state);
        }
    }
};

struct Solver {
    int noWeeks;
    int noMachines;
    int maxChanges;
    int noInteractions;

    std::vector<State> states;

    std::unordered_set<std::string> badOptimizations;

    bool reduceGlobalFailed = false;
    std::optional<Optimization> previousOptimization;
    long bestScore = 0;

    Solver(int noWeeks, int noMachines, int maxChanges, int noInteractions)
            : noWeeks(noWeeks),
              noMachines(noMachines),
              maxChanges(maxChanges),
              noInteractions(noInteractions) {
        State initialState;
        initialState.machines.resize(noMachines);
        states.push_back(initialState);
    }

    void setInitialPatterns() {
        auto &state = states[0];

        for (int i = 0; i < noMachines; i++) {
            auto &machine = state.machines[i];

            for (int j = 0; j < noWeeks; j++) {
                machine.weekDayPatterns[j] = 9;
                machine.weekEndPatterns[j] = 9;
            }
        }
    }

    void refine() {
        auto &state = states[states.size() - 1];

        bestScore = std::max(bestScore, state.score);

        if (previousOptimization.has_value() && (state.score == 0 || state.score < bestScore)) {
            log << "Optimization " << previousOptimization->name << " does not work, reverting" << std::endl;

            previousOptimization->undo(state);
            badOptimizations.insert(previousOptimization->id);

            if (previousOptimization->name == "ReduceGlobal") {
                reduceGlobalFailed = true;
            }
        } else if (previousOptimization.has_value()) {
            log << "Optimization " << previousOptimization->name << " works" << std::endl;
        }

        auto optimizations = generateOptimizations(state);

        std::optional<Optimization> bestOptimization;
        double bestCostImprovement = -1;

        for (const auto &optimization : optimizations) {
            if (optimization.costImprovement > bestCostImprovement
                && badOptimizations.find(optimization.id) == badOptimizations.end()) {
                bestOptimization = optimization;
                bestCostImprovement = optimization.costImprovement;
            }
        }

        if (bestOptimization.has_value()) {
            log << "Trying optimization " << bestOptimization->name << std::endl;
            bestOptimization->apply(state);
        } else {
            log << "No optimizations to try" << std::endl;
        }

        previousOptimization = bestOptimization;
    }

    [[nodiscard]] std::vector<Optimization> generateOptimizations(const State &state) {
        std::vector<Optimization> optimizations;

        std::vector<OptimizationPart> reduceGlobalParts;

        for (int i = 0; i < noMachines; i++) {
            auto &machine = state.machines[i];

            auto [lastOperatingWeekDay, lastOperatingWeekEnd] = getLastOperatingWeeks(machine);

            bool canReduceGlobalWeekDay = lastOperatingWeekDay != -1;
            double weekDayLoadSum = 0.0;
            for (int j = 0; j <= lastOperatingWeekDay; j++) {
                weekDayLoadSum += machine.loads[j];
                if (machine.weekDayPatterns[j] != machine.weekDayPatterns[0]) {
                    canReduceGlobalWeekDay = false;
                    break;
                }
            }

            if (noInteractions != 300 && (weekDayLoadSum / ((double) (lastOperatingWeekDay + 1))) > 0.75) {
                canReduceGlobalWeekDay = false;
            }

            bool canReduceGlobalWeekEnd = lastOperatingWeekEnd != -1;
            double weekEndLoadSum = 0.0;
            for (int j = 1; j <= lastOperatingWeekEnd; j++) {
                weekEndLoadSum += machine.loads[j];
                if (machine.weekEndPatterns[j] != machine.weekEndPatterns[0]) {
                    canReduceGlobalWeekEnd = false;
                    break;
                }
            }

            if (noInteractions != 300 && (weekEndLoadSum / ((double) (lastOperatingWeekEnd + 1))) > 0.75) {
                canReduceGlobalWeekEnd = false;
            }

            if (canReduceGlobalWeekDay && canReduceGlobalWeekEnd) {
                std::vector<OptimizationPart> parts;

                for (int j = 0; j <= lastOperatingWeekDay && j <= lastOperatingWeekEnd; j++) {
                    parts.push_back(OptimizationPart::weekDay(state, i, j, machine.weekDayPatterns[j] - 1));
                    parts.push_back(OptimizationPart::weekEnd(state, i, j, machine.weekEndPatterns[j] - 1));
                    reduceGlobalParts.push_back(OptimizationPart::weekDay(state, i, j, machine.weekDayPatterns[j] - 1));
                    reduceGlobalParts.push_back(OptimizationPart::weekEnd(state, i, j, machine.weekEndPatterns[j] - 1));
                }

                optimizations.emplace_back("ReduceGlobal" + std::to_string(i), parts);
            }

            if (canReduceGlobalWeekDay) {
                std::vector<OptimizationPart> parts;

                for (int j = 0; j <= lastOperatingWeekDay; j++) {
                    parts.push_back(OptimizationPart::weekDay(state, i, j, machine.weekDayPatterns[j] - 1));
                }

                optimizations.emplace_back("ReduceGlobalWeekDay" + std::to_string(i), parts);
            }

            if (canReduceGlobalWeekEnd) {
                std::vector<OptimizationPart> parts;

                for (int j = 0; j <= lastOperatingWeekEnd; j++) {
                    parts.push_back(OptimizationPart::weekEnd(state, i, j, machine.weekEndPatterns[j] - 1));
                }

                optimizations.emplace_back("ReduceGlobalWeekEnd" + std::to_string(i), parts);
            }

            int weekDayChanges = getChanges(machine.weekDayPatterns);
            int weekEndChanges = getChanges(machine.weekEndPatterns);

            double createSplitThreshold = 0.4;
            double improveSplitThreshold = 0.9;

            if (lastOperatingWeekDay != -1) {
                std::vector<std::pair<int, int>> existingSplits;
                existingSplits.emplace_back(0, 1);
                for (int j = 1; j <= lastOperatingWeekDay; j++) {
                    if (machine.weekDayPatterns[j] != machine.weekDayPatterns[j - 1]) {
                        existingSplits.emplace_back(j, 1);
                    } else {
                        existingSplits[existingSplits.size() - 1].second++;
                    }
                }

                for (auto &[start, size] : existingSplits) {
                    bool canImprove = true;
                    double loadSum = 0.0;
                    for (int j = start; j < start + size; j++) {
                        loadSum += machine.loads[j];
                        if (machine.weekDayPatterns[j] == 1) {
                            canImprove = false;
                            break;
                        }
                    }

                    if ((loadSum / ((double) size)) > improveSplitThreshold) {
                        canImprove = false;
                    }

                    if (canImprove) {
                        std::vector<OptimizationPart> parts;

                        for (int j = start; j < start + size; j++) {
                            parts.push_back(OptimizationPart::weekDay(state, i, j, machine.weekDayPatterns[j] - 1));
                        }

                        optimizations.emplace_back("ImproveSplitWeekDay" + std::to_string(i), parts);
                    }
                }

                std::vector<OptimizationPart> parts;
                std::vector<int> newPatterns = machine.weekDayPatterns;

                double loadSum = 0.0;

                for (int j = lastOperatingWeekDay; j >= 0; j--) {
                    loadSum += machine.loads[j];
                    if ((loadSum / (lastOperatingWeekDay - j + 1)) > createSplitThreshold) {
                        break;
                    }

                    parts.push_back(OptimizationPart::weekDay(state, i, j, machine.weekDayPatterns[j] - 1));
                    newPatterns[j]--;
                }

                int newChanges = getChanges(newPatterns);
                int newRemainingChanges = maxChanges - newChanges - weekEndChanges;

                if (!parts.empty() && newRemainingChanges >= 0) {
                    optimizations.emplace_back("CreateSplitWeekDay" + std::to_string(i), parts);
                }
            }

            if (lastOperatingWeekEnd != -1) {
                std::vector<std::pair<int, int>> existingSplits;
                existingSplits.emplace_back(0, 1);
                for (int j = 1; j <= lastOperatingWeekEnd; j++) {
                    if (machine.weekEndPatterns[j] != machine.weekEndPatterns[j - 1]) {
                        existingSplits.emplace_back(j, 1);
                    } else {
                        existingSplits[existingSplits.size() - 1].second++;
                    }
                }

                for (auto &[start, size] : existingSplits) {
                    bool canImprove = true;
                    double loadSum = 0.0;
                    for (int j = start; j < start + size; j++) {
                        loadSum += machine.loads[j];
                        if (machine.weekEndPatterns[j] == 1) {
                            canImprove = false;
                            break;
                        }
                    }

                    if ((loadSum / ((double) size)) > improveSplitThreshold) {
                        canImprove = false;
                    }

                    if (canImprove) {
                        std::vector<OptimizationPart> parts;

                        for (int j = start; j < start + size; j++) {
                            parts.push_back(OptimizationPart::weekEnd(state, i, j, machine.weekEndPatterns[j] - 1));
                        }

                        optimizations.emplace_back("ImproveSplitWeekEnd" + std::to_string(i), parts);
                    }
                }

                std::vector<OptimizationPart> parts;
                std::vector<int> newPatterns = machine.weekEndPatterns;

                double loadSum = 0.0;

                for (int j = lastOperatingWeekEnd; j >= 0; j--) {
                    loadSum += machine.loads[j];
                    if ((loadSum / (lastOperatingWeekDay - j + 1)) > createSplitThreshold) {
                        break;
                    }

                    parts.push_back(OptimizationPart::weekEnd(state, i, j, machine.weekEndPatterns[j] - 1));
                    newPatterns[j]--;
                }

                int newChanges = getChanges(newPatterns);
                int newRemainingChanges = maxChanges - weekDayChanges - newChanges;

                if (!parts.empty() && newRemainingChanges >= 0) {
                    optimizations.emplace_back("CreateSplitWeekEnd" + std::to_string(i), parts);
                }
            }
        }

        if (noInteractions != 300 && !reduceGlobalFailed) {
            optimizations.emplace_back("ReduceGlobal", reduceGlobalParts);
        }

        if (states.size() == noInteractions) {
            std::vector<OptimizationPart> parts;

            for (int i = 0; i < noMachines; i++) {
                auto &machine = state.machines[i];

                int remainingChanges = getRemainingChanges(machine);
                if (remainingChanges == 0) {
                    continue;
                }

                auto [lastOperatingWeekDay, lastOperatingWeekEnd] = getLastOperatingWeeks(machine);

                std::vector<OptimizationPart> partsAll;
                std::vector<OptimizationPart> partsWeekDaysOnly;
                std::vector<OptimizationPart> partsWeekEndsOnly;

                for (int j = std::max(lastOperatingWeekDay, lastOperatingWeekEnd); j >= 0; j--) {
                    if (machine.loads[j] > 0) {
                        break;
                    }

                    partsAll.push_back(OptimizationPart::weekDay(state, i, j, 1));
                    partsAll.push_back(OptimizationPart::weekEnd(state, i, j, 1));
                    partsWeekDaysOnly.push_back(OptimizationPart::weekDay(state, i, j, 1));
                    partsWeekEndsOnly.push_back(OptimizationPart::weekEnd(state, i, j, 1));
                }

                if (remainingChanges == 1) {
                    if (Optimization("", partsWeekDaysOnly).costImprovement
                        > Optimization("", partsWeekEndsOnly).costImprovement) {
                        parts.insert(parts.end(), partsWeekDaysOnly.begin(), partsWeekDaysOnly.end());
                    } else {
                        parts.insert(parts.end(), partsWeekEndsOnly.begin(), partsWeekEndsOnly.end());
                    }
                } else {
                    parts.insert(parts.end(), partsAll.begin(), partsAll.end());
                }
            }

            optimizations.emplace_back("Shutdown", parts);
        }

        return optimizations;
    }

    [[nodiscard]] std::pair<int, int> getLastOperatingWeeks(const Machine &machine) const {
        int lastOperatingWeekDay = -1;
        int lastOperatingWeekEnd = -1;

        for (int i = noWeeks - 1; i >= 0 && (lastOperatingWeekDay == -1 || lastOperatingWeekEnd == -1); i--) {
            if (lastOperatingWeekDay == -1 && machine.weekDayPatterns[i] != 1) {
                lastOperatingWeekDay = i;
            }

            if (lastOperatingWeekEnd == -1 && machine.weekEndPatterns[i] != 1) {
                lastOperatingWeekEnd = i;
            }
        }

        return {lastOperatingWeekDay, lastOperatingWeekEnd};
    }

    [[nodiscard]] int getRemainingChanges(const Machine &machine) const {
        return maxChanges - getChanges(machine.weekDayPatterns) - getChanges(machine.weekEndPatterns);
    }

    [[nodiscard]] int getChanges(const std::vector<int> &patterns) const {
        int changes = 0;

        for (int i = 0; i < noWeeks - 1; i++) {
            if (patterns[i] != patterns[i + 1]) {
                changes++;
            }
        }

        return changes;
    }
};

int main() {
    int noWeeks, noMachines, maxChanges, noInteractions;
    std::cin >> noWeeks >> noMachines >> maxChanges >> noInteractions;

    log << "noWeeks = " << noWeeks
        << ", noMachines = " << noMachines
        << ", maxChanges = " << maxChanges
        << ", noInteractions = " << noInteractions
        << std::endl;

    Solver solver(noWeeks, noMachines, maxChanges, noInteractions);

    for (int i = 0; i < noMachines; i++) {
        auto &machine = solver.states[0].machines[i];

        machine.weekDayPatterns.resize(noWeeks);
        machine.weekEndPatterns.resize(noWeeks);

        machine.weekDayPatternCosts.reserve(noWeeks);
        machine.weekEndPatternCosts.reserve(noWeeks);

        for (int j = 0; j < 9; j++) {
            int weekDayCost, weekEndCost;
            std::cin >> weekDayCost >> weekEndCost;

            machine.weekDayPatternCosts.push_back(weekDayCost);
            machine.weekEndPatternCosts.push_back(weekEndCost);
        }
    }

    log << "Interaction 1" << std::endl;
    solver.setInitialPatterns();

    for (int i = 0; i < noInteractions; i++) {
        auto &currentState = solver.states[i];

        for (int j = 0; j < noMachines; j++) {
            auto &machine = currentState.machines[j];

            for (int k = 0; k < noWeeks; k++) {
                std::cout << machine.weekDayPatterns[k] << machine.weekEndPatterns[k];
            }

            std::cout << std::endl;
        }

        std::cin >> currentState.score >> currentState.noViolations >> currentState.noDelays;

        log << "score = " << currentState.score
            << ", noViolations = " << currentState.noViolations
            << ", noDelays = " << currentState.noDelays
            << std::endl;

        for (int j = 0; j < noMachines; j++) {
            auto &machine = currentState.machines[j];

            machine.loads.resize(noWeeks);
            machine.noDelays.resize(noWeeks);

            for (int k = 0; k < noWeeks; k++) {
                std::cin >> machine.loads[k] >> machine.noDelays[k];
            }
        }

        if (i == noInteractions - 1) {
            break;
        }

        log << "Interaction " << (i + 2) << std::endl;

        State nextState(currentState);
        solver.states.push_back(nextState);

        solver.refine();
    }

    return 0;
}
