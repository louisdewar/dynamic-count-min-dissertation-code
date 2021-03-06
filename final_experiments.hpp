#include "CMS.hpp"
#include "Counter.hpp"
#include "TraceReader.hpp"

using namespace std;

void baseline_performance_fixed_mem_synthetic(int mem, char *trace_path,
                                              FILE *flat_results,
                                              FILE *traditional_results,
                                              FILE *skew_estimation);

void dynamic_performance_fixed_mem_synthetic(int mem, char *trace_path,
                                             FILE *results,
                                             FILE *skew_estimation);

void baseline_performance_fixed_mem_real_world(int mem, char *trace_path,
                                               FILE *flat_results,
                                               FILE *traditional_results,
                                               FILE *skew_estimation);

void dynamic_performance_fixed_mem_real_world(int mem, char *trace_path,
                                              FILE *results,
                                              FILE *skew_estimation);
