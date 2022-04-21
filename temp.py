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

    skew_traces = find_real_world_final_traces()

    tasks = []
    for trace in skew_traces:
        mem = 2**20
        tracename = f"{trace.removesuffix('.pcap')}.csv"
        flat_results = os.path.join(output_dir, "original", "flat", tracename)
        traditional_results = os.path.join(output_dir, "original", "traditional", tracename)
        skew_estimation_results = os.path.join(output_dir, "original", "skew_estimates", tracename)
        tasks.append((baseline_performance_fixed_mem_synthetic_task, [trace, flat_results, traditional_results, skew_estimation_results, mem]))

    if should_run_tasks:
        run_tasks(tasks)
