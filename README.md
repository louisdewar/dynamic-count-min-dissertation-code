# Dynamic count-min dissertation code

This repo contains the code for my 2022 dynamic count-min dissertation supervised by Dr Ran Ben Basat at University College London.
Details for how to use it, including building the source code can be found in the appendix of the main dissertation in the section called "System Manual".

## Overview of C++ files

- `main.cpp` the entrypoint, uses the CLI args to decide which experiment to run and with what parameters.
- `CMS.cpp` / `CMS.hpp`, contains all the sketches used by this project including an implementation of the final dynamic sketch. The baseline sketch was originally from SALSA, but it was adapted in several different ways for this project.
- `topK.cpp` / `topK.hpp`, a top-k data structure slightly adapted from SALSA in order to be more convenient to work with.
- `final_experiments.cpp` / `final_experiments.hpp` the functions implementing experiments that were used for the final dissertation.
- `experiment.hpp` some experiments that were used throughout the project, although `final_experiments` should be preferred.
- `genzipf.h` code to generate zipf traces, it is untouched from SALSA where it was used from another project.
- `skew_estimation.cpp` / `skew_estimation.hpp` code for estimation of skews. It includes the final estimation technique along with some debugging methods.
- `optimal_paramaters.cpp` / `optimal_paramaters.hpp` contains code for finding the pre-determined optimal parameters based on estimated skew and error metric.
- `Counter.cpp` / `Counter.hpp` contains code for getting the true count of packets, one used an array which is suitable for synthetic sketches due to their domain, and another uses a hash map which is less efficient but can work with real-world traces.
- `BobHash.cpp` / `BobHash.hpp` from SALSA, used for calcuating hashes.
- `xxhash.cpp` / `xxhash.h` external library for hashing, used solely for the hash packet counter.
- `zipf_stats.hpp` some utility functions for calculating things like the Zipfian harmonic number.
