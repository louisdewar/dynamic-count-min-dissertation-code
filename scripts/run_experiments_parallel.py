import multiprocessing as mp
import tqdm
import subprocess
import os
import shutil
import sys

def run_task(task):
    task[0](*task[1])


def run_tasks(tasks):
    cpus = mp.cpu_count()
    print("Running {} tasks with {} parallel processes".format(len(tasks), cpus))

    pool = mp.Pool(mp.cpu_count())

    for _ in tqdm.tqdm(pool.imap_unordered(run_task, tasks), total=len(tasks)):
        pass

    # results = [pool.apply(f, args, kwargs) for (f, args, kwargs) in tasks]

    pool.close()

def run_bin(args):
    args = [str(arg) for arg in args]
    print('Running: ', ' '.join(["builddir/fyp", *args]))
    subprocess.run(["builddir/fyp", *args])


def genzip_task(skew, n_samples, seed):
    run_bin(["genzipf", f"traces/trace-{n_samples}-{skew}.data", n_samples, skew, seed])


def genzipf(n_samples):
    print("Generating zipf traces into `traces` folder")

    if os.path.exists("traces"):
        shutil.rmtree("traces")
    os.makedirs("traces")

    # Multiplied by 10 to prevent floating point errors
    skew_delta = 1
    skew_start = 6
    skew_end = 13
    seed = 750

    skew = skew_start

    tasks = []
    while skew <= skew_end:
        tasks.append((genzip_task, [skew / 10, n_samples, seed]))
        skew += skew_delta

    run_tasks(tasks)

def test_traces_task(trace):
    # Get the trace file name without the extension to use as the base
    basename = os.path.splitext(os.path.basename(trace))[0]

    run_bin(["experiment", os.path.join("traces", trace), os.path.join("results", basename)])

def test_traces_fixed_mem_task(trace, output, mem):
    # Get the trace file name without the extension to use as the base
    basename = os.path.splitext(os.path.basename(trace))[0]

    run_bin(["test_traces_fixed_mem", os.path.join("traces", trace), os.path.join(output, "original", basename), mem])

def test_traces():
    files = list(filter(lambda path: os.path.splitext(path)[1] == ".data", os.listdir("traces")))
    print("Testing all traces ({}) in the `traces` folder".format(len(files)))

    tasks = [(test_traces_task, [trace]) for trace in files]

    run_tasks(tasks)

def test_traces_fixed_mem(output):
    output = os.path.join("results", output)

    if os.path.exists(output):
        shutil.rmtree(output)
    os.makedirs(os.path.join(output, "original"))
    os.makedirs(os.path.join(output, "transformed"))
    mem = 2 ** 16

    files = list(filter(lambda path: os.path.splitext(path)[1] == ".data", os.listdir("traces")))
    print("Testing all traces ({}) in the `traces` folder (fixed mem = {} bytes w/ varying skew)".format(len(files), mem))
    print("Output location =", output)

    tasks = [(test_traces_fixed_mem_task, [trace, output, mem]) for trace in files]

    run_tasks(tasks)

    results = { os.path.basename(file).split("-")[2]: os.path.join(output, "original", file) for file in os.listdir(os.path.join(output, "original")) }


    columns = transform_results(results, "skew", os.path.join(output, "transformed"))

    for _, column in columns.items():
        column_path = column["path"]
        files = column["files"]
        output = os.path.join(column_path, "graph.png")
        generate_graph({ f"n={hash_count}": file for hash_count, file in files.items() }, output)
        print(f'Generated graph at {output}')

def test_trace_fixed_alpha_task(output, trace, alpha, mem):
    run_bin(["test_trace_fixed_alpha", trace, os.path.join(output, "original", f"alpha-{alpha}-counters-{mem}-.csv"), alpha, mem])

def test_trace_fixed_alpha(output, trace, alpha, startPow, endPow):
    output = os.path.join("results", output)

    if os.path.exists(output):
        shutil.rmtree(output)
    os.makedirs(os.path.join(output, "original"))
    os.makedirs(os.path.join(output, "transformed"))
    mem = 2 ** (startPow);

    tasks = []

    for i in range(startPow, endPow + 1):
        mem = 2 ** i
        tasks.append((test_trace_fixed_alpha_task, [output, trace, alpha, mem]))

    run_tasks(tasks)

    results = { os.path.basename(file).split("-")[3]: os.path.join(output, "original", file) for file in os.listdir(os.path.join(output, "original")) }

    columns = transform_results(results, "counters", os.path.join(output, "transformed"))

    for _, column in columns.items():
        column_path = column["path"]
        files = column["files"]
        output = os.path.join(column_path, "graph.png")
        generate_graph({ f"n={hash_count}": file for hash_count, file in files.items() }, output)
        print(f'Generated graph at {output}')
    # skews = sorted(list(map(lambda key: float(key), results.keys())))

    # hash_counts = []
    # columns = []

    # # Open one of the skews to get the hash functions
    # with open(results[str(skews[0])], "r") as trace:
    #     lines = trace.read().splitlines()
    #     header = lines[0].split(',')

    #     columns = header[1:]

    #     for line in lines[1:]:
    #         hash_count = int(line.split(',')[0])
    #         hash_counts.append(hash_count)

    # print("Processing the following different metrics", columns)

    # column_paths = [os.path.join(output, "transformed", column.replace(" ", "-")) for column in columns]

    # for column_path in column_paths:
    #     # This is only here because sometimes for debugging I remove the code above in order to skip re-running the data
    #     if os.path.exists(column_path):
    #         shutil.rmtree(column_path)

    #     os.makedirs(column_path)

    # files = [{ hash_count: open(os.path.join(column_path, "trace-{}.csv".format(hash_count)), "w") for hash_count in hash_counts } for column_path in column_paths]

    # # Write CSV header to all the output files
    # for column_index, column in enumerate(columns):
    #     for _, file in files[column_index].items():
    #         file.write("skew,{}\n".format(column))

    # for skew in skews:
    #     with open(results[str(skew)], "r") as trace:
    #         lines = trace.read().splitlines()
    #         for line in lines:
    #             try:
    #                 hash_count = int(line.split(',')[0])
    #             except ValueError:
    #                 # There's always a label which can't be converted to an int
    #                 continue

    #             values = line.split(',')[1:]

    #             for column, value in enumerate(values):
    #                 files[column][hash_count].write("{},{}\n".format(skew, value))

    # for column in files:
    #     for _, file in column.items():
    #         file.close()
    
    # for column_index, column in enumerate(columns):
    #     column_path = column_paths[column_index]
    #     output = os.path.join(column_path, "graph.png")
    #     generate_graph({ f"n={hash_count}": file.name for hash_count, file in files[column_index].items() }, output)
    #     print(f'Generated graph at {output}')


# Results should be a dictionary where the key is what becomes the transformed x component and the value is the file containing that data.
# This outputs files to `{output_base}/{column_header}/trace-{x_i}.csv` (x_i is from the original traces x values).
# All input files must have the same x values and same column headers (or at least labelled identically after the x column).
# Returns a dictionary where the key is the extracted column names and the value is a dictionary with two keys `path` (the directory where the files are contained)
# and `files` (a dictionary of x_value to path)
def transform_results(results, results_key_header, output_base):
    # == Get column names and create files with headers

    # Look at one particular results file:
    
    columns = {}
    print(results.keys())
    # NOTE: trace is probably used incorrectly here (it should probably be results but I need a unique name)
    with open(results[next(iter(results.keys()))], "r") as trace:
        lines = trace.read().splitlines()
        headers = [header.replace(" ", "-") for header in lines[0].split(',')]
        x_header = headers[0]
        headers = headers[1:]

        print("Found the following headers:", headers)

        # the x values get split into individual files to each column, where the file has the x value in its name
        x_values = []
        for line in lines[1:]:
            x_values.append(line.split(',')[0])

        for column in headers:
            column_path = os.path.join(output_base, column)

            if os.path.exists(column_path):
                shutil.rmtree(column_path)
            os.makedirs(column_path)

            columns[column] = {
                "path": column_path,
                "files": { x_value: open(os.path.join(column_path, f"trace-{x_value}-{x_header}.csv"), "w") for x_value in x_values }
            }

            for file in columns[column]["files"].values():
                # Add headers to files
                file.write(f"{results_key_header},{column}\n")

    # == Go over each pre-transformed file and map it to the new files
    for key, input_path in results.items():
        with open(input_path, "r") as trace:
            lines = trace.read().splitlines()
            headers = [header.replace(" ", "-") for header in lines[0].split(',')]

            for line in lines[1:]:
                line = line.split(",")
                x_value = line[0]

                for column_index in range(1, len(headers)):
                    column_name = headers[column_index]
                    column_value = line[column_index]

                    columns[column_name]["files"][x_value].write(f"{key},{column_value}\n")

    # == Close all the files
    for column in columns.values():
        for x_value,file in column["files"].items():
            path = file.name
            file.close()
            # Map the file to the path (we return this value)
            column["files"][x_value] = path
            
        # columns = { [name]: { path: os.path.join(output_base, name), files: { [x_value]: { file: open(os.path.join(output_base, name, f"trace-{x_value}-{x_header}.csv"), "w") } for x_value in x_values } } for name in headers }

    #return { [column_name]: column["path"] for column_name, column in columns.items() }
    return columns

# Requires: `pip install graph-cli`
# There appears to be a bug with this tool such that the labels for the x-y axis are swapped
def generate_graph(files, output):
    commandparts = []
    for label, file in files.items():
        commandparts.append(f"graph {file} --marker='' --legend='{label}'")

    command = " --chain | ".join(commandparts)
    command += f" -o {output}"

    subprocess.run(["/usr/bin/sh", "-c", command])
    

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
        if len(sys.argv) < 3:
            print("Missing arguments for test_traces_fixed_mem")
            sys.exit(1)

        output = sys.argv[2]
        test_traces_fixed_mem(output)
    elif experiment == "test_trace_fixed_alpha":
        if len(sys.argv) < 7:
            print("Missing arguments for test_trace_fixed_alpha")
            sys.exit(1)

        trace = sys.argv[2]
        output = sys.argv[3]
        alpha = sys.argv[4]
        startPow = int(sys.argv[5])
        endPow = int(sys.argv[6])
        test_trace_fixed_alpha(output, trace, alpha, startPow, endPow)



