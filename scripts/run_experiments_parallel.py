import multiprocessing as mp
import tqdm
import subprocess
import os
import shutil
import sys
from process import average_results, lowest_error, transform_results, generate_graph, topk_histogram_graph


def run_task(task):
    task[0](*task[1])


def run_tasks(tasks):
    cpus = mp.cpu_count()
    print("Running {} tasks with {} parallel processes".format(len(tasks), cpus))

    pool = mp.Pool(mp.cpu_count())

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


def genzip_task(skew, n_samples, seed):
    run_bin(["genzipf", f"traces/seed-{seed}/trace-{n_samples}-{skew}.data", n_samples, skew, seed])

def genzipf(n_samples):
    print("Generating zipf traces into `traces` folder")

    if os.path.exists("traces"):
        shutil.rmtree("traces")
    os.makedirs("traces")

    # Multiplied by 10 to prevent floating point errors
    skew_delta = 1
    skew_start = 6
    skew_end = 13

    seeds = [100, 211, 356, 439, 578]

    tasks = []
    for seed in seeds:
        skew = skew_start
        os.makedirs(f"traces/seed-{seed}/")

        while skew <= skew_end:
            tasks.append((genzip_task, [skew / 10, n_samples, seed]))
            skew += skew_delta

    run_tasks(tasks)

def test_traces_task(trace):
    # Get the trace file name without the extension to use as the base
    basename = os.path.splitext(os.path.basename(trace))[0]

    run_bin(["experiment", os.path.join("traces", trace), os.path.join("results", basename)])

def test_traces_fixed_mem_task(trace, output, mem, alpha):
    run_bin(["test_traces_fixed_mem", trace, output, mem, alpha])

def test_traces():
    files = list(filter(lambda path: os.path.splitext(path)[1] == ".data", os.listdir("traces")))
    print("Testing all traces ({}) in the `traces` folder".format(len(files)))

    tasks = [(test_traces_task, [trace]) for trace in files]

    run_tasks(tasks)

def test_traces_fixed_mem(output_dir, mem_pow, alpha):
    output_dir = os.path.join("results", output_dir)
    averaged_dir = os.path.join(output_dir, "averaged")

    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(os.path.join(output_dir, "original"))
    os.makedirs(os.path.join(output_dir, "transformed"))
    os.makedirs(averaged_dir)

    mem = 2 ** mem_pow

    skew_traces = find_traces()

    tasks = []
    average_groups = {}
    for skew, traces in skew_traces.items():
        group_key = f"skew-{skew}-fixed_mem.csv"
        average_groups[group_key] = []

        for trace in traces:
            output_file = os.path.join(output_dir, "original", f"seed-{trace.seed}-skew-{skew}-mem-{mem}.csv")
            tasks.append((test_traces_fixed_mem_task, [trace.path, output_file, mem, alpha]))
            average_groups[group_key].append(output_file)

    run_tasks(tasks)
    average_results(averaged_dir, average_groups, ["normalized error", "heavy hitter error", "sketch error"])

    results = { os.path.basename(file).split("-")[1]: os.path.join(averaged_dir, file) for file in os.listdir(averaged_dir) }

    resultsKeysOrdered = list(results.keys())
    resultsKeysOrdered.sort(key=lambda key: float(key))

    columns = transform_results(results, resultsKeysOrdered, "skew", os.path.join(output_dir, "transformed"))

    for column_name, column in columns.items():
        column_path = column["path"]
        files = column["files"]
        lowest_error_file = os.path.join(column_path, "lowest_error.csv")
        lowest_error(files, "skew", "hash functions", column_name, lowest_error_file)

        plot_output = os.path.join(column_path, "lowest_error.png")
        subprocess.run(["/bin/sh", "-c", f"graph {lowest_error_file} -o {plot_output} -x skew -y 'hash functions'"])
        print(f'Generated graph at {plot_output}')

        plot_output = os.path.join(column_path, "graph.png")
        generate_graph({ f"n={hash_count}": file for hash_count, file in files.items() }, plot_output)
        print(f'Generated graph at {plot_output}')


def test_trace_fixed_alpha_task(output, trace, alpha, mem):
    run_bin(["test_trace_fixed_alpha", trace, output, alpha, mem])

def test_skew_fixed_alpha(output_dir, skew, alpha, startMemPow, endMemPow):
    output_dir = os.path.join("results", output_dir)
    averaged_dir = os.path.join(output_dir, "averaged")

    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(os.path.join(output_dir, "original"))
    os.makedirs(os.path.join(output_dir, "transformed"))
    os.makedirs(averaged_dir)

    mem = 2 ** (startMemPow);

    tasks = []

    traces = find_traces()[skew]

    # Used for averaging across traces
    result_groups = {}
    for i in range(startMemPow, endMemPow + 1):
        mem = 2 ** i
        group_key = f"alpha-{alpha}-counters-{mem}-.csv"
        result_groups[group_key] = []
        for trace in traces:
            output_path = os.path.join(output_dir, "original", f"counters-{mem}-seed-{trace.seed}.csv")
            result_groups[group_key].append(output_path)
            tasks.append((test_trace_fixed_alpha_task, [output_path, trace.path, alpha, mem]))

    run_tasks(tasks)

    average_results(averaged_dir, result_groups, ["normalized error", "heavy hitter error", "sketch error"])

    results = { os.path.basename(file).split("-")[3]: os.path.join(averaged_dir, file) for file in os.listdir(averaged_dir) }

    resultsKeysOrdered = list(results.keys())
    resultsKeysOrdered.sort(key=lambda key: float(key))

    columns = transform_results(results, resultsKeysOrdered, "counters", os.path.join(output_dir, "transformed"))

    for _, column in columns.items():
        column_path = column["path"]
        files = column["files"]
        output_dir = os.path.join(column_path, "graph.png")
        generate_graph({ f"n={hash_count}": file for hash_count, file in files.items() }, output_dir)
        print(f'Generated graph at {output_dir}')

        generate_lowest_error_report(files, [0.0, 0.05], column_path)

def generate_lowest_error_report(files, margins, output_base):
    file_data = []
    for file_label, path in files.items():
        with open(path, "r") as f:
            file_data.append({ "label": file_label, "rows": [line.split(',') for line in f.read().splitlines()[1:]] })
    
    margin_results = [{ "margin": margin, "labels": {data["label"]: [] for data in file_data } } for margin in margins]
    for row in range(0, len(file_data[0]["rows"])):
        values = [{ "x": float(data["rows"][row][0]), "y": float(data["rows"][row][1]), "label": data["label"] } for data in file_data]
        lowest_value = values[0]["y"]

        for value in values:
            if value["y"] < lowest_value:
                lowest_value = value["y"]

        for margin_i, margin in enumerate(margins):
            threshold = lowest_value * (1 + margin)

            for value in values:
                if value["y"] <= threshold:
                    margin_results[margin_i]["labels"][value["label"]].append(value["x"])

        

    with open(os.path.join(output_base, "margin_report.txt"), "w") as f:
        f.write("""\
Margin report
=============

Margin: | Results within margin% of the lowest error:
""")
        for result in margin_results:
            margin = 100 * result["margin"]
            f.write(f"{margin}%  | ")

            for label, values_within_threshold in result["labels"].items():
                if len(values_within_threshold) > 0:
                    within_threshold = ', '.join(map(lambda val: str(val), values_within_threshold))
                    f.write(f" - {label} ({within_threshold})")

            f.write("\n")


def top_k_task(trace, histogram_output, skew_cost_output, harmonic_estimate_output, k, mem, hash_functions):
    run_bin(["test_top_k", trace, histogram_output, skew_cost_output, harmonic_estimate_output, k, mem, hash_functions])

# Tests top k on all the traces
def test_top_k(output_dir, k, mem_pow, hash_functions):
    output_dir = os.path.join("results", output_dir)
    histogram_averaged_dir = os.path.join(output_dir, "averaged_histogram")
    skew_cost_averaged_dir = os.path.join(output_dir, "averaged_skew_cost")
    histogram_original_dir = os.path.join(output_dir, "original_histogram")
    skew_cost_original_dir = os.path.join(output_dir, "original_skew_cost")
    harmonic_estimate_original_dir = os.path.join(output_dir, "original_harmonic_estimate")


    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)
    os.makedirs(histogram_averaged_dir)
    os.makedirs(histogram_original_dir)
    os.makedirs(skew_cost_averaged_dir)
    os.makedirs(skew_cost_original_dir)
    os.makedirs(harmonic_estimate_original_dir)

    mem = 2 ** mem_pow

    skew_traces = find_traces()

    tasks = []
    histogram_average_groups = {}
    skew_cost_average_groups = {}
    for skew, traces in skew_traces.items():
        group_key = f"top-{k}-hash-{hash_functions}-skew-{skew}-top_k.csv"
        histogram_average_groups[group_key] = []
        skew_cost_average_groups[group_key] = []

        for trace in traces:
            trace_output_file_name = f"top-{k}-hash-{hash_functions}-seed-{trace.seed}-skew-{skew}.csv"
            histogram_output = os.path.join(histogram_original_dir, trace_output_file_name)
            skew_cost_output = os.path.join(skew_cost_original_dir, trace_output_file_name)
            harmonic_estimate_output = os.path.join(harmonic_estimate_original_dir, trace_output_file_name)

            tasks.append((top_k_task, [trace.path, histogram_output, skew_cost_output, harmonic_estimate_output, k, mem, hash_functions]))
            histogram_average_groups[group_key].append(histogram_output)
            skew_cost_average_groups[group_key].append(skew_cost_output)
    
    run_tasks(tasks)

    average_results(histogram_averaged_dir, histogram_average_groups, ["frequency"])
    average_results(skew_cost_averaged_dir, skew_cost_average_groups, ["cost"])

    histogram_results = { os.path.basename(file).removesuffix(".csv"): os.path.join(histogram_averaged_dir, file) for file in histogram_average_groups.keys() }
    skew_cost_results = { os.path.basename(file).removesuffix(".csv"): os.path.join(skew_cost_averaged_dir, file) for file in skew_cost_average_groups.keys() }

    for key, file in histogram_results.items():
        skew = key.split('-')[5]
        graph_output = os.path.join(output_dir, key + ".png")
        print(f"Generating histogram {graph_output}")
        topk_histogram_graph(file, graph_output, f"Histogram of topK for skew f{skew}")

    for key, file in skew_cost_results.items():
        skew = key.split('-')[5]
        graph_output = os.path.join(output_dir, "topk-skew-cost-" + key + ".png")
        print(f"Generating skew cost graph {graph_output}")
        subprocess.run(["/bin/sh", "-c", f"graph '{file}' -o '{graph_output}' --title 'Skew cost analysis where true skew={skew}' --figsize '1600x1000'"])

def top_flat_k_task(trace, skew_estimate_output, k, mem, hash_functions, estimate_frequency):
    run_bin(["test_flat_top_k", trace, skew_estimate_output, k, mem, hash_functions, estimate_frequency])

# Tests top k on all the traces
def test_flat_top_k(output_dir, k, mem_pow, hash_functions, estimate_frequency):
    output_dir = os.path.join("results", output_dir)

    skew_estimate_original_dir = os.path.join(output_dir, "original_skew_estimate")


    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)
    os.makedirs(output_dir)

    os.makedirs(skew_estimate_original_dir)

    mem = 2 ** mem_pow

    skew_traces = find_traces()

    tasks = []
    for skew, traces in skew_traces.items():
        for trace in traces:
            trace_output_file_name = f"top-{k}-hash-{hash_functions}-seed-{trace.seed}-skew-{skew}.csv"
            skew_estimate_output = os.path.join(skew_estimate_original_dir, trace_output_file_name)

            tasks.append((top_flat_k_task, [trace.path, skew_estimate_output, k, mem, hash_functions, estimate_frequency]))
    
    run_tasks(tasks)


# This command is a bit unusual in that it acts over different groups of results already produced as opposed to creating new results
# This finds lower and upper bounds for the optimal number of hash functions across different sets of results (different counter amounts)
# by looking at the `lower_bound.csv` in each error metric.
# The result_dirs should be folders to the top level of the test_traces_fixed_mem results (i.e. the folder that contains the transformed folder)
def lower_upper_bounds(output_dir, result_dirs):
    output_dir = os.path.join("results", output_dir)
    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)

    os.makedirs(output_dir)
    
    metrics = {}

    for result_dir in result_dirs:
        transformed_dir = os.path.join(result_dir, "transformed")
        if not os.path.exists(transformed_dir):
            raise Exception(f"No `transformed` directory inside {result_dir}")

        for metric in os.listdir(transformed_dir):
            #if not metric in metrics:
            #    metrics[metric] = { "lower": None, "upper": None }
            with open(os.path.join(transformed_dir, metric, "lowest_error.csv"), "r") as f:
                rows = map(lambda line: line.split(","), f.readlines()[1:])
                rows = [[row[0], int(row[1])] for row in rows]

                if not metric in metrics:

                    metrics[metric] = { "lower": rows, "upper": [row.copy() for row in rows] }
                else:
                    lower = metrics[metric]["lower"]
                    upper = metrics[metric]["upper"]

                    for row_i, row in enumerate(rows):
                        lower[row_i][1] = min(lower[row_i][1], row[1])
                        upper[row_i][1] = max(upper[row_i][1], row[1])

    for metric, results in metrics.items():
        with open(os.path.join(output_dir, f"{metric}-lower.csv"), "w") as f:
            f.write("skew,lower optimal\n")
            for [skew, val] in results["lower"]:
                f.write(f"{skew},{val}\n")
                        
        with open(os.path.join(output_dir, f"{metric}-upper.csv"), "w") as f:
            f.write("skew,upper optimal\n")
            for [skew, val] in results["upper"]:
                f.write(f"{skew},{val}\n")
                        
def convert_pcap_task(pcap, converted_path):
    #decompressed = pcap.removesuffix(".gz")
    # subprocess.run(["gunzip", "-f", decompressed])
    # print("unzipped")
    args = ["bash", "-c", f"""tcpdump -q -n -t -r {pcap} | grep IP[^6] | sed 's/\\./ /g' | awk '{{print $2" "$3" "$4" "$5"-"$6"\\t"$8" "$9" "$10" "$11"-"$12""$13}}'"""]
    print(" ".join(map(lambda arg: '"' + arg + '"', args)))
    process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)

    def decode_line(line: str):
        def decode_address_port(address_port):
            [address, port] = address_port.split("-")
            port = int(port)
            address = [int(part) for part in address.split(" ")]
            return (address, port)

        [addresses, protocol] = line.split(":")
        protocol = "".join([c for c in protocol if c.isalpha()])
        [source, destination] = map(lambda address: decode_address_port(address), addresses.split("\t"))

        protocol_number = 0 if protocol == "udp" else 1

        return (source, destination, protocol_number)

    def encode_address_port(address):
        (address, port) = address
        parts = [part.to_bytes(1, "big") for part in address] + [port.to_bytes(2, "big")]
        # print(parts)
        # print(b"".join(parts))
        #return bytearray([byte for bytes in parts for byte in bytes])
        return b"".join(parts)

    with open(converted_path, "wb") as binary_file:
        while True:
            line = process.stdout.readline().decode().lower()
            if not line:
                break
            
            if "udp" not in line and "tcp" not in line:
                continue

            #print(line)
            (source, destination, protocol) = decode_line(line)
            #print(f"source={source}, destination={destination}, protocol={protocol}")
            binary_file.write(encode_address_port(source))
            binary_file.write(encode_address_port(destination))
            binary_file.write(protocol.to_bytes(1, "big"))
            


def convert_pcap_gz(pcap_folder, output_dir):
    converted_dir = os.path.join(output_dir, "converted")
    if not os.path.exists(output_dir):
        os.makedirs(converted_dir)

    tasks = []
    for pcap in os.listdir(pcap_folder):
        if not pcap.endswith(".pcap"):
            continue
        basename = pcap#.removesuffix(".gz")
        pcap_path = os.path.join(pcap_folder, pcap)

        tasks.append((convert_pcap_task, [pcap_path, os.path.join(converted_dir, basename)]))

    run_tasks(tasks)


if __name__ == "__main__":
    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    os.chdir(os.path.abspath(os.path.join(dname, os.pardir)))

    try:
        experiment = sys.argv[1]
    except IndexError:
        print("usage: python3 run_experiments_parallel.py <experiment>")
        sys.exit(1)

    if experiment == "genzipf":
        if len(sys.argv) < 3:
            print("Missing arguments for genzipf")
            sys.exit(1)

        n_samples = int(sys.argv[2])
        genzipf(n_samples)
    elif experiment == "test_traces":
        test_traces()
    elif experiment == "test_traces_fixed_mem":
        if len(sys.argv) < 5:
            print("Missing arguments for test_traces_fixed_mem")
            sys.exit(1)

        output = sys.argv[2]
        mem_pow = int(sys.argv[3])
        alpha = sys.argv[4]
        test_traces_fixed_mem(output, mem_pow, alpha)
    elif experiment == "test_skew_fixed_alpha":
        if len(sys.argv) < 7:
            print("Missing arguments for test_skew_fixed_alpha")
            sys.exit(1)

        trace = sys.argv[2]
        output = sys.argv[3]
        alpha = sys.argv[4]
        startPow = int(sys.argv[5])
        endPow = int(sys.argv[6])
        test_skew_fixed_alpha(output, trace, alpha, startPow, endPow)
    elif experiment == "test_top_k":
        if len(sys.argv) < 6:
            print("Missing arguments for test_top_k")
            sys.exit(1)

        k = int(sys.argv[2])
        mem_pow = int(sys.argv[3])
        hash_functions = int(sys.argv[4])
        output = sys.argv[5]
        test_top_k(output, k, mem_pow, hash_functions)
    elif experiment == "lower_upper_bounds":
        output_dir = sys.argv[2]
        files = sys.argv[3:]
        lower_upper_bounds(output_dir, files)
    elif experiment == "convert_pcap":
        pcap_folder = sys.argv[2]
        output_dir = sys.argv[3]
        convert_pcap_gz(pcap_folder, output_dir)
    elif experiment == "test_flat_top_k":
        k = int(sys.argv[2])
        mem_pow = int(sys.argv[3])
        hash_functions = int(sys.argv[4])
        estimate_frequency = int(sys.argv[5])
        output_dir = sys.argv[6]
        test_flat_top_k(output_dir, k, mem_pow, hash_functions, estimate_frequency)
    else:
        print(f"Unrecognised command {experiment}")
        sys.exit(1)


