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

def find_real_world_final_traces():
    final_traces_dir = "traces_converted/final/"
    return [os.path.join(final_traces_dir, trace) for trace in os.listdir(final_traces_dir) if trace.endswith("pcap")]

def ensure_dir_exists(path):
    if not os.path.exists(path):
        os.makedirs(path)


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
        subprocess.run(["/bin/sh", "-c", f"graph '{lowest_error_file}' -o '{plot_output}' -x skew -y 'hash functions'"])
        print(f'Generated graph at {plot_output}')

        plot_output = os.path.join(column_path, "graph.png")
        util.generate_graph({ f"n={hash_count}": file for hash_count, file in files.items() }, plot_output)
        print(f'Generated graph at {plot_output}')

def output_transformed_results_dynamic(averaged_dir, transformed_dir, average_groups):
    raw_results = { os.path.basename(key).split("-")[1]: value  for key, value in average_groups.items() }

    results = { os.path.basename(file).split("-")[1]: os.path.join(averaged_dir, file) for file in os.listdir(averaged_dir) }

    results_keys_ordered = list(results.keys())
    results_keys_ordered.sort(key=lambda key: float(key))

    columns = util.transform_results(results, results_keys_ordered, "skew", transformed_dir)

    for column_name, column in columns.items():
        column_path = column["path"]
        files = column["files"]
        error_bounds_file = os.path.join(column_path, "error_bounds.csv")
        util.error_bounds(raw_results, results_keys_ordered, "variant", column["original_name"], error_bounds_file)

        lowest_error_file = os.path.join(column_path, "lowest_error.csv")
        util.lowest_error(files, "skew", "variant", column_name, lowest_error_file)

def baseline_performance_fixed_mem_synthetic_task(trace, flat_results, traditional_results, skew_estimation_results, mem):
    run_bin(["final_baseline_performance_fixed_mem_synthetic", trace, flat_results, traditional_results, skew_estimation_results, mem])

def baseline_performance_fixed_mem_synthetic(output_dir):
    output_dir = os.path.join("results", output_dir)
    averaged_dir = os.path.join(output_dir, "averaged")
    flat_averaged_dir = os.path.join(averaged_dir, "flat")
    traditional_averaged_dir = os.path.join(averaged_dir, "traditional")
    transformed_dir = os.path.join(output_dir, "transformed")

    should_run_tasks = True
    if os.path.exists(output_dir):
        should_run_tasks = False
        print("Not running experiments because output dir already exists")
    ensure_dir_exists(os.path.join(output_dir, "original", "flat"))
    ensure_dir_exists(os.path.join(output_dir, "original", "traditional"))
    ensure_dir_exists(os.path.join(output_dir, "original", "skew_estimates"))
    ensure_dir_exists(os.path.join(output_dir, "transformed", "flat"))
    ensure_dir_exists(os.path.join(output_dir, "transformed", "traditional"))
    ensure_dir_exists(averaged_dir)
    ensure_dir_exists(flat_averaged_dir)
    ensure_dir_exists(traditional_averaged_dir)

    mem_pows = [16, 20]
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

    avr_columns = ["normalized error", "heavy hitter error"]#, "sketch error e", "sketch error 2e", "sketch error 4e", "sketch error 8e"]
    util.average_results(traditional_averaged_dir, traditional_average_groups, avr_columns)
    util.average_results(flat_averaged_dir, flat_average_groups, avr_columns)

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

def baseline_performance_fixed_mem_real_world_task(trace, flat_results, traditional_results, skew_estimation_results, mem):
    run_bin(["final_baseline_performance_fixed_mem_real_world", trace, flat_results, traditional_results, skew_estimation_results, mem])

def baseline_performance_fixed_mem_real_world(output_dir):
    output_dir = os.path.join("results", output_dir)
    transformed_dir = os.path.join(output_dir, "transformed")

    should_run_tasks = True
    if os.path.exists(output_dir):
        should_run_tasks = False
        print("Not running experiments because output dir already exists")
    ensure_dir_exists(os.path.join(output_dir, "original", "flat"))
    ensure_dir_exists(os.path.join(output_dir, "original", "traditional"))
    ensure_dir_exists(os.path.join(output_dir, "original", "skew_estimates"))
    ensure_dir_exists(transformed_dir)

    skew_traces = find_real_world_final_traces()

    mem_pows = [12]

    tasks = []
    by_mem = {}
    for mem_pow in mem_pows:
        by_mem[mem_pow] = {"flat": [], "traditional": []}
        mem = 2**mem_pow
        for trace in skew_traces:
            tracename = f"{os.path.basename(trace).removesuffix('.pcap')}-pow-{mem_pow}.csv"
            flat_results = os.path.join(output_dir, "original", "flat", tracename)
            traditional_results = os.path.join(output_dir, "original", "traditional", tracename)
            skew_estimation_results = os.path.join(output_dir, "original", "skew_estimates", tracename)
            by_mem[mem_pow]["flat"].append(flat_results)
            by_mem[mem_pow]["traditional"].append(traditional_results)
            tasks.append((baseline_performance_fixed_mem_real_world_task, [trace, flat_results, traditional_results, skew_estimation_results, mem]))

    if should_run_tasks:
        run_tasks(tasks)

    for mem_pow, variants in by_mem.items():
        for variant in ["flat", "traditional"]:
            results = { os.path.basename(trace).split('-')[0]: trace for trace in variants[variant] }
            util.transform_results(results, ["equinix", "univ1", "univ2"], "trace", os.path.join(transformed_dir, f"mem-{mem_pow}-{variant}"))

def dynamic_performance_fixed_mem_real_world_task(trace, variant_results, skew_estimation_results, mem):
    run_bin(["final_dynamic_performance_fixed_mem_real_world", trace, variant_results, skew_estimation_results, mem])

def dynamic_performance_fixed_mem_real_world(output_dir):
    output_dir = os.path.join("results", output_dir)
    transformed_dir = os.path.join(output_dir, "transformed")

    should_run_tasks = True
    if os.path.exists(output_dir):
        should_run_tasks = False
        print("Not running experiments because output dir already exists")
    ensure_dir_exists(os.path.join(output_dir, "original", "dynamic"))
    ensure_dir_exists(os.path.join(output_dir, "original", "skew_estimates"))
    ensure_dir_exists(transformed_dir)

    skew_traces = find_real_world_final_traces()

    mem_pows = [10, 12, 14, 16, 18]

    tasks = []
    by_mem = {}
    for mem_pow in mem_pows:
        by_mem[mem_pow] = []
        mem = 2**mem_pow
        for trace in skew_traces:
            tracename = f"{os.path.basename(trace).removesuffix('.pcap')}-pow-{mem_pow}.csv"
            variant_results = os.path.join(output_dir, "original", "dynamic", tracename)
            skew_estimation_results = os.path.join(output_dir, "original", "skew_estimates", tracename)
            by_mem[mem_pow].append(variant_results)
            tasks.append((dynamic_performance_fixed_mem_real_world_task, [trace, variant_results, skew_estimation_results, mem]))

    if should_run_tasks:
        run_tasks(tasks)

    for mem_pow, traces in by_mem.items():
        results = { os.path.basename(trace).split('-')[0]: trace for trace in traces }
        util.transform_results(results, ["equinix", "univ1", "univ2"], "trace", os.path.join(transformed_dir, f"mem-{mem_pow}"))



def dynamic_performance_fixed_mem_synthetic_task(trace, variant_results, skew_estimation_results, mem):
    run_bin(["final_dynamic_performance_fixed_mem_synthetic", trace, variant_results, skew_estimation_results, mem])

def dynamic_performance_fixed_mem_synthetic(output_dir):
    output_dir = os.path.join("results", output_dir)
    averaged_dir = os.path.join(output_dir, "averaged")
    dynamic_averaged_dir = os.path.join(averaged_dir, "dynamic")
    transformed_dir = os.path.join(output_dir, "transformed")

    should_run_tasks = True
    if os.path.exists(output_dir):
        should_run_tasks = False
        print("Not running experiments because output dir already exists", output_dir)
    ensure_dir_exists(os.path.join(output_dir, "original", "dynamic"))
    ensure_dir_exists(os.path.join(output_dir, "original", "skew_estimates"))
    ensure_dir_exists(os.path.join(output_dir, "transformed", "dynamic"))
    ensure_dir_exists(averaged_dir)
    ensure_dir_exists(dynamic_averaged_dir)

    mem_pows = [20]

    skew_traces = find_traces()

    tasks = []
    dynamic_average_groups = {}
    by_mem_size = {}
    for mem_pow in mem_pows:
        by_mem_size[mem_pow] = { "dynamic": {} }

    for skew, traces in skew_traces.items():
        group_key = f"skew-{skew}-fixed_mem.csv"
        dynamic_average_groups[group_key] = []

        for mem_pow in mem_pows:
            mem = 2**mem_pow
            by_mem_size[mem_pow]["dynamic"][group_key] = []

            for trace in traces:
                base_name = f"seed-{trace.seed}-skew-{skew}-mem-{mem}.csv"
                dynamic_results = os.path.join(output_dir, "original", "dynamic", base_name)
                skew_estimation_results = os.path.join(output_dir, "original", "skew_estimates", base_name)
                tasks.append((dynamic_performance_fixed_mem_synthetic_task, [trace.path, dynamic_results, skew_estimation_results, mem]))
                dynamic_average_groups[group_key].append(dynamic_results)
                by_mem_size[mem_pow]["dynamic"][group_key].append(dynamic_results)

    if should_run_tasks:
        run_tasks(tasks)

    avr_columns = ["normalized error", "heavy hitter error"]#, "sketch error e", "sketch error 2e", "sketch error 4e", "sketch error 8e"]
    util.average_results(dynamic_averaged_dir, dynamic_average_groups, avr_columns)
    # util.average_results(averaged_dir, average_groups, ["normalized error", "heavy hitter error", "sketch error"])


    output_transformed_results_dynamic(dynamic_averaged_dir, os.path.join(output_dir, "transformed", "dynamic"), dynamic_average_groups)

    for mem, dynamic_average_groups in by_mem_size.items():
        for variant in ["dynamic"]:
            mem_average_dir = os.path.join(averaged_dir, f"mem-{mem}-{variant}")
            mem_transformed_dir = os.path.join(transformed_dir, f"mem-{mem}-{variant}")
            ensure_dir_exists(mem_average_dir)
            ensure_dir_exists(mem_transformed_dir)

            util.average_results(mem_average_dir, dynamic_average_groups[variant], avr_columns)
            output_transformed_results_dynamic(mem_average_dir, mem_transformed_dir, dynamic_average_groups[variant])

def merge_traces(dir, starting_with, output):
    import natsort
    traces = [path for path in os.listdir(dir) if path.startswith(starting_with)]
    traces = natsort.natsorted(traces)

    print("Concatenating in this order:", " ".join(traces))
    subprocess.run(["sh", "-c", "cat {} > {}".format(" ".join([os.path.join(dir, trace) for trace in traces]), output)])

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
    elif experiment == "baseline_real_world_fixed_mem":
        # output = sys.argv[2]
        baseline_performance_fixed_mem_real_world("final_baseline_real_world_fixed_mem")
    elif experiment == "dynamic_synthetic":
       dynamic_performance_fixed_mem_synthetic("final_dynamic_synthetic_fixed_mem")
    elif experiment == "dynamic_real_world":
       dynamic_performance_fixed_mem_real_world("final_dynamic_real_world_fixed_mem")
    elif experiment == "merge_traces":
        dir = sys.argv[2]
        starts_with = sys.argv[3]
        output = sys.argv[4]
        merge_traces(dir, starts_with, output)

