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
    mem = 2 ** 8;

    files = list(filter(lambda path: os.path.splitext(path)[1] == ".data", os.listdir("traces")))
    print("Testing all traces ({}) in the `traces` folder (fixed mem = {} bytes w/ varying skew)".format(len(files), mem))
    print("Output location =", output)

    tasks = [(test_traces_fixed_mem_task, [trace, output, mem]) for trace in files]

    run_tasks(tasks)


    results = { os.path.basename(file).split("-")[2]: os.path.join(output, "original", file) for file in os.listdir(os.path.join(output, "original")) }

    print(results)

    skews = sorted(list(map(lambda key: float(key), results.keys())))

    hash_counts = []

    # Open one of the skews to get the hash functions
    with open(results[str(skews[0])], "r") as trace:
        lines = trace.read().splitlines()
        for line in lines:
            try:
                hash_count = int(line.split(',')[0])
                hash_counts.append(hash_count)
            except ValueError:
                # There's always a label which can't be converted to an int
                continue


    files = { hash_count: open(os.path.join(output, "transformed", "trace-{}.csv".format(hash_count)), "w") for hash_count in hash_counts }

    # Write CSV header to all the output files
    for count, file in files.items():
        file.write("skew,normalized error\n")

    for skew in skews:
        with open(results[str(skew)], "r") as trace:
            lines = trace.read().splitlines()
            for line in lines:
                try:
                    hash_count = int(line.split(',')[0])
                except ValueError:
                    # There's always a label which can't be converted to an int
                    continue

                error = line.split(',')[1]

                files[hash_count].write("{},{}\n".format(skew, error))

    for count, file in files.items():
        file.close()


if __name__ == "__main__":
    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    os.chdir(dname)

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


