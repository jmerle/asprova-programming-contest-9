import argparse
import json
import shutil
import subprocess
from pathlib import Path
from multiprocessing import Pool
from typing import List

def get_score_from_logs(logs_file: Path) -> int:
    logs_content = logs_file.read_text(encoding="utf-8")
    for line in reversed(logs_content.splitlines()):
        if line.startswith("Score = "):
            return int(line.split(" = ")[1])

    return 0

def update_overview() -> None:
    scores_by_solver = {}
    outputs_root = Path(__file__).parent / "output"

    for directory in outputs_root.iterdir():
        scores_by_seed = {}

        for file in directory.iterdir():
            if file.name.endswith(".log"):
                scores_by_seed[file.stem] = get_score_from_logs(file)

        scores_by_solver[directory.name] = scores_by_seed

    overview_template_file = Path(__file__).parent / "overview.tmpl.html"
    overview_file = Path(__file__).parent / "overview.html"

    overview_template = overview_template_file.read_text(encoding="utf-8")
    overview = overview_template.replace("/* scores_by_solver */{}", json.dumps(scores_by_solver))

    with overview_file.open("w+", encoding="utf-8") as file:
        file.write(overview)

    print(f"Overview: file://{overview_file}")

def run_seed(solver: Path, seed: int, output_directory: Path) -> int:
    input_file = Path(__file__).parent / "input" / f"{seed}.in"
    if not input_file.is_file():
        args_input_file = input_file.parent / f"{seed}."
        generated_input_file = input_file.parent / f"{seed}.0000.txt"
        generator = Path(__file__).parent.parent / "cmake-build-release" / "generator"

        subprocess.run([str(generator), str(args_input_file)], input=f"-seed {seed}\n".encode("utf-8"))

        generated_input_file.rename(input_file)

    judge = Path(__file__).parent.parent / "cmake-build-release" / "judge"
    output_file = output_directory / f"{seed}.out"
    logs_file = output_directory / f"{seed}.log"

    with input_file.open("rb") as input:
        with output_file.open("wb+") as output:
            with logs_file.open("wb+") as logs:
                try:
                    process = subprocess.run([str(judge), str(solver)],
                                             stdin=input,
                                             stdout=output,
                                             stderr=logs,
                                             timeout=5)

                    if process.returncode != 0:
                        raise RuntimeError(f"Judge exited with status code {process.returncode} for seed {seed}")

                    return get_score_from_logs(logs_file)
                except subprocess.TimeoutExpired:
                    raise RuntimeError(f"Judge timed out for seed {seed}")

def run(solver: Path, seeds: List[int], output_directory: Path) -> None:
    if not output_directory.is_dir():
        output_directory.mkdir(parents=True)

    with Pool() as pool:
        scores = pool.starmap(run_seed, [(solver, seed, output_directory) for seed in seeds])

    for i, seed in enumerate(seeds):
        print(f"{seed}: {scores[i]:,.0f}")

    if len(seeds) > 1:
        print(f"Total score: {sum(scores):,.0f}")

def main() -> None:
    parser = argparse.ArgumentParser(description="Run a solver.")
    parser.add_argument("solver", type=str, help="the solver to run")
    parser.add_argument("--seed", type=int, help="the seed to run (defaults to 1-100)")

    args = parser.parse_args()

    solver = Path(__file__).parent.parent / "cmake-build-release" / f"solver_{args.solver}"
    if not solver.is_file():
        raise RuntimeError(f"Solver not found, {solver} is not a file")

    output_directory = Path(__file__).parent / "output" / args.solver

    if args.seed is None:
        if output_directory.is_dir():
            shutil.rmtree(output_directory)

        run(solver, list(range(1, 101)), output_directory)
    else:
        run(solver, [args.seed], output_directory)

    update_overview()

if __name__ == "__main__":
    main()
