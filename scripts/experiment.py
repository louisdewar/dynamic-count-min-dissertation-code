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

def ensure_dir_exists(path):
    if not os.path.exists(path):
        os.makedirs(path)


# # Calculates average performance of flat count min and regular count min (with a range of parameters) with different skews at a fixed memory.
# # The final results are written to "{output_dir}/final"
# def baseline_performance_fixed_mem_generator(traces_dir, output_dir, mem_pow):

# def find_lower_upper_bounds(output_dir_map, transformed_dirs):
#     metrics = {}
# 
#     for transformed_dir in transformed_dirs:
#         for metric in os.listdir(transformed_dir):
#             #if not metric in metrics:
#             #    metrics[metric] = { "lower": None, "upper": None }
#             with open(os.path.join(transformed_dir, metric, "lowest_error.csv"), "r") as f:
#                 rows = map(lambda line: line.split(","), f.readlines()[1:])
#                 rows = [[row[0], int(row[1])] for row in rows]
# 
#                 if not metric in metrics:
#                     metrics[metric] = { "lower": rows, "upper": [row.copy() for row in rows] }
#                 else:
#                     lower = metrics[metric]["lower"]
#                     upper = metrics[metric]["upper"]
# 
#                     for row_i, row in enumerate(rows):
#                         lower[row_i][1] = min(lower[row_i][1], row[1])
#                         upper[row_i][1] = max(upper[row_i][1], row[1])
# 
#     for metric, results in metrics.items():
#         output_dir = output_dir_map[metric]
#         with open(os.path.join(output_dir, f"{metric}-lower.csv"), "w") as f:
#             f.write("skew,lower optimal\n")
#             for [skew, val] in results["lower"]:
#                 f.write(f"{skew},{val}\n")
#                         
#         with open(os.path.join(output_dir, f"{metric}-upper.csv"), "w") as f:
#             f.write("skew,upper optimal\n")
#             for [skew, val] in results["upper"]:
#                 f.write(f"{skew},{val}\n")

def output_transformed_results(averaged_dir, transformed_dir, average_groups):
    raw_results = { os.path.basename(key).split("-")[1]: value  for key, value in average_groups.items() }

    results = { os.path.basename(file).split("-")[1]: os.path.join(averaged_dir, file) for file in os.listdir(averaged_dir) }

    results_keys_ordered = list(results.keys())
    results_keys_ordered.sort(key=lambda key: float(key))

    columns = util.transform_results(results, results_keys_ordered, "skew", transformed_dir)

    for column_name, column in columns.items():
        column_path = column["path"]
        files = column["files"]
        error_bounds_file = os.path.join(column_path, "error_bounds.csv")
        util.error_bounds(raw_results, results_keys_ordered, "hash functions", column["original_name"], error_bounds_file)

        lowest_error_file = os.path.join(column_path, "lowest_error.csv")
        util.lowest_error(files, "skew", "hash functions", column_name, lowest_error_file)

        plot_output = os.path.join(column_path, "lowest_error.png")
        subprocess.run(["/bin/sh", "-c", f"graph {lowest_error_file} -o {plot_output} -x skew -y 'hash functions'"])
        print(f'Generated graph at {plot_output}')

        plot_output = os.path.join(column_path, "graph.png")
        util.generate_graph({ f"n={hash_count}": file for hash_count, file in files.items() }, plot_output)
        print(f'Generated graph at {plot_output}')

def baseline_performance_fixed_mem_synthetic_task(trace, flat_results, traditional_results, skew_estimation_results, mem):
    run_bin(["final_baseline_performance_fixed_mem_synthetic", trace, flat_results, traditional_results, skew_estimation_results, mem])

def baseline_performance_fixed_mem_synthetic(output_dir):
    # TODO: output latex code for figures
    output_dir = os.path.join("results", output_dir)
    averaged_dir = os.path.join(output_dir, "averaged")
    flat_averaged_dir = os.path.join(averaged_dir, "flat")
    traditional_averaged_dir = os.path.join(averaged_dir, "traditional")
    transformed_dir = os.path.join(output_dir, "transformed")

    should_run_tasks = True
    if os.path.exists(output_dir):
        should_run_tasks = False
        print("Not running experiments because output dir already exists")
        # raise Exception("Output dir already exists")
        # shutil.rmtree(output_dir)
    ensure_dir_exists(os.path.join(output_dir, "original", "flat"))
    ensure_dir_exists(os.path.join(output_dir, "original", "traditional"))
    ensure_dir_exists(os.path.join(output_dir, "original", "skew_estimates"))
    ensure_dir_exists(os.path.join(output_dir, "transformed", "flat"))
    ensure_dir_exists(os.path.join(output_dir, "transformed", "traditional"))
    ensure_dir_exists(averaged_dir)
    ensure_dir_exists(flat_averaged_dir)
    ensure_dir_exists(traditional_averaged_dir)

    mem_pows = [16, 18, 20]
    #mem_pows = [12]

    skew_traces = find_traces()

    tasks = []
    flat_average_groups = {}
    traditional_average_groups = {}
    by_mem_size = {}
    for mem_pow in mem_pows:
        by_mem_size[mem_pow] = { "flat": {}, "traditional": {} }

    for skew, traces in skew_traces.items():
        group_key = f"skew-{skew}-fixed_mem.csv"
        flat_average_groups[group_key] = []
        traditional_average_groups[group_key] = []

        for mem_pow in mem_pows:
            mem = 2**mem_pow
            by_mem_size[mem_pow]["flat"][group_key] = []
            by_mem_size[mem_pow]["traditional"][group_key] = []

            for trace in traces:
                base_name = f"seed-{trace.seed}-skew-{skew}-mem-{mem}.csv"
                flat_results = os.path.join(output_dir, "original", "flat", base_name)
                traditional_results = os.path.join(output_dir, "original", "traditional", base_name)
                skew_estimation_results = os.path.join(output_dir, "original", "skew_estimates", base_name)
                tasks.append((baseline_performance_fixed_mem_synthetic_task, [trace.path, flat_results, traditional_results, skew_estimation_results, mem]))
                flat_average_groups[group_key].append(flat_results)
                traditional_average_groups[group_key].append(traditional_results)
                by_mem_size[mem_pow]["flat"][group_key].append(flat_results)
                by_mem_size[mem_pow]["traditional"][group_key].append(traditional_results)

    if should_run_tasks:
        run_tasks(tasks)

    avr_columns = ["normalized error", "heavy hitter error"]
    util.average_results(traditional_averaged_dir, traditional_average_groups, avr_columns)
    util.average_results(flat_averaged_dir, flat_average_groups, avr_columns)
    # util.average_results(averaged_dir, average_groups, ["normalized error", "heavy hitter error", "sketch error"])

    variant_avr_groups = {"flat": flat_average_groups, "traditional": traditional_average_groups}

    for variant in ["flat", "traditional"]:
        output_transformed_results(os.path.join(averaged_dir, variant), os.path.join(output_dir, "transformed", variant), variant_avr_groups[variant])

    for mem, average_groups in by_mem_size.items():
        for variant in ["flat", "traditional"]:
            mem_average_dir = os.path.join(averaged_dir, f"mem-{mem}-{variant}")
            mem_transformed_dir = os.path.join(transformed_dir, f"mem-{mem}-{variant}")
            ensure_dir_exists(mem_average_dir)
            ensure_dir_exists(mem_transformed_dir)

            util.average_results(mem_average_dir, average_groups[variant], avr_columns)
            output_transformed_results(mem_average_dir, mem_transformed_dir, average_groups[variant])


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
