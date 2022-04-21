import os, shutil, subprocess

from pygnuplot import gnuplot

# Results should be a dictionary where the key is what becomes the transformed x component and the value is the file containing that data.
# This outputs files to `{output_base}/{column_header}/trace-{x_i}.csv` (x_i is from the original traces x values).
# All input files must have the same x values and same column headers (or at least labelled identically after the x column).
# Returns a dictionary where the key is the extracted column names and the value is a dictionary with two keys `path` (the directory where the files are contained)
# and `files` (a dictionary of x_value to path)
def transform_results(results, resultsKeysOrdered, results_key_header, output_base):
    print('order:', resultsKeysOrdered)
    # == Get column names and create files with headers

    # Look at one particular results file:
    
    columns = {}
    # NOTE: trace is probably used incorrectly here (it should probably be results but I need a unique name)
    with open(results[next(iter(results.keys()))], "r") as trace:
        lines = trace.read().splitlines()
        original_headers = lines[0].split(',')
        headers = [header.replace(" ", "-") for header in original_headers]
        x_header = headers[0]
        headers = headers[1:]

        print("Found the following headers:", headers)

        # the x values get split into individual files to each column, where the file has the x value in its name
        x_values = []
        for line in lines[1:]:
            x_values.append(line.split(',')[0])

        for column, original_name in zip(headers, original_headers[1:]):
            column_path = os.path.join(output_base, column)

            if os.path.exists(column_path):
                shutil.rmtree(column_path)
            os.makedirs(column_path)

            columns[column] = {
                "original_name": original_name,
                "path": column_path,
                "files": { x_value: open(os.path.join(column_path, f"trace-{x_value}-{x_header}.csv"), "w") for x_value in x_values }
            }

            for file in columns[column]["files"].values():
                # Add headers to files
                file.write(f"{results_key_header},{column}\n")

    # == Go over each pre-transformed file and map it to the new files
    for key in resultsKeysOrdered:
        input_path = results[key]
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
            

    return columns

def lowest_error(labelled_files: dict[str, str], x_column: str, new_column_name: str, y_column: str, output_path: str):
    output_file = open(output_path, "w")

    output_file.write(f"{x_column},{new_column_name},{y_column}\n")

    ordered_rows = None
    lowest = {}
    for key, file in labelled_files.items():
        with open(file, "r") as csv:
            rows = csv.read().splitlines()
            headers = rows[0].split(",")
            rows = rows[1:]

            if not y_column in headers:
                raise Exception(f"{y_column} (y) was not a header of {file}")
            if not x_column in headers:
                raise Exception(f"{x_column} (x) was not a header of {file}")

            x_column_index = headers.index(x_column)
            y_column_index = headers.index(y_column)

            # For the first CSV
            if ordered_rows is None:
                lowest 
                ordered_rows = []
                for row in rows:
                    values = row.split(",")
                    x_value = values[x_column_index]
                    y_value = float(values[y_column_index])
                    ordered_rows.append(x_value)
                    lowest[x_value] = (key, y_value)
            else:
                for row in rows:
                    values = row.split(",")
                    x_value = values[x_column_index]
                    y_value = float(values[y_column_index])

                    if lowest[x_value][1] > y_value:
                        lowest[x_value] = (key, y_value)
    
    if ordered_rows is None:
        raise Exception("No files")

    for x_value in ordered_rows:
        (label, y_value) = lowest[x_value]

        output_file.write(f"{x_value},{label},{y_value}\n")

# labelled_files: [skew, [file]]
def error_bounds(labelled_files: dict[str, list[str]], results_keys_ordered, x_column: str, y_column: str, output_path: str):

    # The lowest value of x which was at some point an optimal value
    lowest_best_map = {}
    # The highest value of x which was at some point an optimal value
    highest_best_map = {}
    for skew, files in labelled_files.items():
        lowest_best = None
        highest_best = None

        for file in files:
            best = None
            # Find best in single file
            with open(file, "r") as csv:
                rows = csv.read().splitlines()
                headers = rows[0].split(",")
                rows = rows[1:]

                if not y_column in headers:
                    raise Exception(f"{y_column} (y) was not a header of {file}")
                if not x_column in headers:
                    raise Exception(f"{x_column} (x) was not a header of {file}")

                x_column_index = headers.index(x_column)
                y_column_index = headers.index(y_column)


                for row in rows:
                    values = row.split(",")
                    x_value = values[x_column_index]
                    y_value = float(values[y_column_index])

                    if best is None:
                        best = (x_value, y_value)

                    if best[1] > y_value:
                        best = (x_value, y_value)

            assert best != None
            if lowest_best is None or highest_best is None:
                lowest_best = best
                highest_best = best

            if lowest_best[0] > best[0]:
                lowest_best = best
            if highest_best[0] < best[0]:
                highest_best = best

        lowest_best_map[skew] = lowest_best
        highest_best_map[skew] = highest_best

    

    with open(output_path, "w") as output_file:
        output_file.write(f"skew,lower,upper\n")
        for skew in results_keys_ordered:
            lower = lowest_best_map[skew]
            upper = highest_best_map[skew]
            output_file.write(f"{skew},{lower[0]},{upper[0]}\n")

# Requires: `pip install graph-cli`
# There appears to be a bug with this tool such that the labels for the x-y axis are swapped
def generate_graph(files, output):
    commandparts = []
    for label, file in files.items():
        commandparts.append(f"graph '{file}' --marker='' --legend='{label}'")

    command = " --chain | ".join(commandparts)
    command += f" -o '{output}'"

    subprocess.run(["/usr/bin/sh", "-c", command])
    
def topk_histogram_graph(file, output, title):
    g = gnuplot.Gnuplot(terminal='pngcairo',
                        output=f'"{output}"')

    g.cmd("set datafile separator \",\"")
    g.plot(f"\"{file}\" using 1:4 notitle with lines", title=f"\"{title}\"")

def average_results(output_dir, groups, avr_columns):
    print(f"Averaging results in the following columns: {avr_columns}")

    for group, result_paths in groups.items():
        output_f = open(os.path.join(output_dir, group), "w")
        samples = len(result_paths)
        
        columns = []
        output_results = {}
        first_path = result_paths[0]
        with open(first_path, "r") as csv:
            csv = csv.read().splitlines()

            headers = csv[0].split(",")
            columns = headers

            for header in headers:
                output_results[header] = []
                
            row_len = len(csv) - 1
            for row in csv[1:]:
                values = row.split(",")
                for header, value in zip(headers, values):
                    if header in avr_columns:
                        value = float(value)
                    output_results[header].append(value)

        for result_path in result_paths[1:]:
            with open(result_path, "r") as csv:
                csv = csv.read().splitlines()

                headers = csv[0].split(",")

                if headers != columns:
                    raise Exception(f"Headers of {result_path} did not match headers of {first_path} ({columns} vs {headers})")

                for row_i, row in enumerate(csv[1:]):

                    values = row.split(",")
                    for header, value in zip(headers, values):
                        # We only average columns in the avr_columns list
                        if not header in avr_columns:
                            continue
                        output_results[header][row_i] += float(value)
                                
        output_f.write(",".join(columns) + "\n")
        for row_i in range(row_len):
            row = [str(value / samples) if header in avr_columns else value for (header, value) in map(lambda header: (header, output_results[header][row_i]), columns)]

            output_f.write(",".join(row))
            output_f.write("\n")

        output_f.flush()
        output_f.close()

if __name__ == '__main__':
    topk_histogram_graph('results/top-k-100-14-8/averaged/top-100-hash-8-skew-1.1-top_k.csv', "output.png", "Histogram of topK for skew 1.1")
