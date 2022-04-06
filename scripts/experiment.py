import os, tqdm, subprocess, shutil, sys

import multiprocessing as mp
import process as util

def run_task(task):
    task[0](*task[1])


def run_tasks(tasks):
    cpus = mp.cpu_count()
    print("Running {} tasks with {} parallel processes".format(len(tasks), cpus))

    pool = mp.Pool((4 * mp.cpu_count()) // 5)

    for _ in tqdm.tqdm(pool.imap_unordered(run_task, tasks), total=len(tasks)):
        pass

    pool.close()

def run_bin(args):
    args = [str(arg) for arg in args]
    print('Running: ', ' '.join(["builddir/fyp", *args]))
    subprocess.run(["builddir/fyp", *args])

class Trace:
    def __init__(self, seed: str, path: str):
        self.seed = seed
        self.path = path

def find_traces() -> dict[str, list[Trace]]:
    traces = {}
    for entry in os.scandir("traces"):
        if not entry.is_dir():
            continue

        seed = entry.name.split("-")[1]

        for trace in os.listdir(entry):
            if not trace.endswith(".data"):
                continue

            basename = trace[:-5]
            parts = basename.split("-")
            skew = parts[2]

            if not skew in traces:
                traces[skew] = []

            traces[skew].append(Trace(seed, os.path.join(entry, trace)))
    
    return traces




# # Calculates average performance of flat count min and regular count min (with a range of parameters) with different skews at a fixed memory.
# # The final results are written to "{output_dir}/final"
# def baseline_performance_fixed_mem_generator(traces_dir, output_dir, mem_pow):

def output_averaged_results(averaged_dir, transformed_dir):
    results = { os.path.basename(file).split("-")[1]: os.path.join(averaged_dir, file) for file in os.listdir(averaged_dir) }

    resultsKeysOrdered = list(results.keys())
    resultsKeysOrdered.sort(key=lambda key: float(key))

    columns = util.transform_results(results, resultsKeysOrdered, "skew", transformed_dir)

    for column_name, column in columns.items():
        column_path = column["path"]
        files = column["files"]
        lowest_error_file = os.path.join(column_path, "lowest_error.csv")
        util.lowest_error(files, "skew", "hash functions", column_name, lowest_error_file)

        plot_output = os.path.join(column_path, "lowest_error.png")
        subprocess.run(["/bin/sh", "-c", f"graph {lowest_error_file} -o {plot_output} -x skew -y 'hash functions'"])
        print(f'Generated graph at {plot_output}')

        plot_output = os.path.join(column_path, "graph.png")
        util.generate_graph({ f"n={hash_count}": file for hash_count, file in files.items() }, plot_output)
        print(f'Generated graph at {plot_output}')

def baseline_performance_fixed_mem_synthetic_task(trace, flat_results, traditional_results, mem):
    run_bin(["final_baseline_performance_fixed_mem_synthetic", trace, flat_results, traditional_results, mem])

def baseline_performance_fixed_mem_synthetic(output_dir):
    # TODO: output latex code for figures
    output_dir = os.path.join("results", output_dir)
    averaged_dir = os.path.join(output_dir, "averaged")
    flat_averaged_dir = os.path.join(averaged_dir, "flat")
    traditional_averaged_dir = os.path.join(averaged_dir, "traditional")
    transformed_dir = os.path.join(output_dir, "transformed")

    if os.path.exists(output_dir):
        raise Exception("Output dir already exists")
        # shutil.rmtree(output_dir)
    os.makedirs(os.path.join(output_dir, "original", "flat"))
    os.makedirs(os.path.join(output_dir, "original", "traditional"))
    os.makedirs(os.path.join(output_dir, "transformed", "flat"))
    os.makedirs(os.path.join(output_dir, "transformed", "traditional"))
    os.makedirs(averaged_dir)
    os.makedirs(flat_averaged_dir)
    os.makedirs(traditional_averaged_dir)

    mem_pows = [16, 18, 20]
    mems = [2 ** mem_pow for mem_pow in mem_pows]

    skew_traces = find_traces()

    tasks = []
    flat_average_groups = {}
    traditional_average_groups = {}
    by_mem_size = {}
    for mem in mems:
        by_mem_size[mem] = { "flat": {}, "traditional": {} }

    for skew, traces in skew_traces.items():
        group_key = f"skew-{skew}-fixed_mem.csv"
        flat_average_groups[group_key] = []
        traditional_average_groups[group_key] = []

        for mem in mems:
            by_mem_size[mem]["flat"][group_key] = []
            by_mem_size[mem]["traditional"][group_key] = []

            for trace in traces:
                base_name = f"seed-{trace.seed}-skew-{skew}-mem-{mem}.csv"
                flat_results = os.path.join(output_dir, "original", "flat", base_name)
                traditional_results = os.path.join(output_dir, "original", "traditional", base_name)
                tasks.append((baseline_performance_fixed_mem_synthetic_task, [trace.path, flat_results, traditional_results, mem]))
                flat_average_groups[group_key].append(flat_results)
                traditional_average_groups[group_key].append(traditional_results)

    run_tasks(tasks)

    avr_columns = ["normalized error", "heavy hitter error"]
    util.average_results(traditional_averaged_dir, traditional_average_groups, avr_columns)
    util.average_results(flat_averaged_dir, flat_average_groups, avr_columns)
    # util.average_results(averaged_dir, average_groups, ["normalized error", "heavy hitter error", "sketch error"])

    for variant in ["flat", "traditional"]:
        output_averaged_results(os.path.join(averaged_dir, variant), os.path.join(output_dir, "transformed", variant))
        # results = { os.path.basename(file).split("-")[1]: os.path.join(averaged_dir, variant, file) for file in os.listdir(os.path.join(averaged_dir, variant)) }

        # resultsKeysOrdered = list(results.keys())
        # resultsKeysOrdered.sort(key=lambda key: float(key))

        # columns = util.transform_results(results, resultsKeysOrdered, "skew", os.path.join(output_dir, "transformed", variant))

        # for column_name, column in columns.items():
        #     column_path = column["path"]
        #     files = column["files"]
        #     lowest_error_file = os.path.join(column_path, "lowest_error.csv")
        #     util.lowest_error(files, "skew", "hash functions", column_name, lowest_error_file)

        #     plot_output = os.path.join(column_path, "lowest_error.png")
        #     subprocess.run(["/bin/sh", "-c", f"graph {lowest_error_file} -o {plot_output} -x skew -y 'hash functions'"])
        #     print(f'Generated graph at {plot_output}')

        #     plot_output = os.path.join(column_path, "graph.png")
        #     util.generate_graph({ f"n={hash_count}": file for hash_count, file in files.items() }, plot_output)
        #     print(f'Generated graph at {plot_output}')

    for mem, average_groups in by_mem_size.items():
        for variant in ["flat", "traditional"]:
            mem_average_dir = os.path.join(averaged_dir, f"mempow-{mem}-{variant}")
            mem_transformed_dir = os.path.join(transformed_dir, f"mempow-{mem}-{variant}")
            if not os.path.exists(mem_average_dir):
                os.makedirs(mem_average_dir)
            if not os.path.exists(mem_transformed_dir):
                os.makedirs(mem_transformed_dir)

            util.average_results(mem_average_dir, average_groups[variant], avr_columns)
            output_averaged_results(mem_average_dir, mem_transformed_dir)


if __name__ == "__main__":
    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    os.chdir(os.path.abspath(os.path.join(dname, os.pardir)))

    try:
        experiment = sys.argv[1]
    except IndexError:
        print("usage: python3 experiment.py <experiment> [experiment args]")
        sys.exit(1)

    if experiment == "baseline_synthetic_fixed_mem":
        # output = sys.argv[2]
        baseline_performance_fixed_mem_synthetic("final_baseline_synthetic_fixed_mem")
