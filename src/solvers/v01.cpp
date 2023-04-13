#include <iostream>
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

    int optimizingMachine = 0;

    long score = 0;
    int noViolations = 0;
    int noDelays = 0;
};

struct Solver {
    int noWeeks;
    int noMachines;
    int maxChanges;
    int noInteractions;

    std::vector<State> states;

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
        auto &previousState = states[states.size() - 2];
        auto &currentState = states[states.size() - 1];

        if (currentState.optimizingMachine >= noMachines) {
            return;
        }

        if (currentState.noDelays > 0) {
            auto &machine = currentState.machines[currentState.optimizingMachine];

            int safePattern = machine.weekDayPatterns[0] + 1;
            for (int i = 0; i < noWeeks; i++) {
                machine.weekDayPatterns[i] = safePattern;
                machine.weekEndPatterns[i] = safePattern;
            }

            currentState.optimizingMachine++;
        }

        if (currentState.optimizingMachine >= noMachines) {
            return;
        }

        if (currentState.machines[currentState.optimizingMachine].weekDayPatterns[0] == 1) {
            currentState.optimizingMachine++;
        }

        if (currentState.optimizingMachine >= noMachines) {
            return;
        }

        auto &machine = currentState.machines[currentState.optimizingMachine];

        int patternToTest = machine.weekDayPatterns[0] - 1;
        for (int i = 0; i < noWeeks; i++) {
            machine.weekDayPatterns[i] = patternToTest;
            machine.weekEndPatterns[i] = patternToTest;
        }
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

    solver.setInitialPatterns();

    for (int i = 0; i < noInteractions; i++) {
        log << "Interaction " << (i + 1) << std::endl;

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

        State nextState(currentState);
        solver.states.push_back(nextState);

        solver.refine();
    }

    return 0;
}
