#include "final_experiments.hpp"

/*
 * Experiments used in the final dissertation
 */

enum Variant { Flat, Traditional };

class SketchEvaluation {
public:
  EvaluatableSketch *sketch;
  Variant variant;
  string variant_name;
  double sum_sq_err;

  SketchEvaluation(EvaluatableSketch *sketch, Variant variant) {
    this->sketch = sketch;
    this->variant = variant;
    this->sum_sq_err = 0.0;

    if (variant == Flat) {
      variant_name = "flat";
    } else {
      variant_name = "traditional";
    }
  }

  void handle_packet(char *packet, int actual, double seen_packets) {
    this->sketch->increment(packet);
    int estimate = sketch->query(packet);
    double diff = estimate - actual;
    this->sum_sq_err += diff * diff;
  }

  int get_hash_function_count() {
    return this->sketch->get_hash_function_count();
  }

  double normalized_error(double total) {
    double inv_n = 1.0 / total;
    double error = sqrt(this->sum_sq_err / total) / total;

    return error;
  }

  double heavy_hitter_err_real_world(TopK trueTopK, int threshold, long total) {
    auto items = trueTopK.items();
    long double heavy_hitter_sq_sum_err = 0.0;
    int heavy_hitters = 0;
    for (auto it = --items.end(); it >= items.begin(); --it) {
      uint32_t actual = it->second;
      char *packet = it->first.data();

      if (actual < threshold) {
        break;
      }

      int estimate = sketch->query(packet);
      long double err =
          ((long double)actual - (long double)estimate) / (long double)total;

      heavy_hitters++;
      heavy_hitter_sq_sum_err += err * err;
    }

    if (heavy_hitters == 0) {
      return 0.0;
    }

    return sqrt(heavy_hitter_sq_sum_err / (long double)heavy_hitters);
  }

  double heavy_hitter_err(PacketCounter *counter, int threshold, long total) {
    char *packet = new char[FT_SIZE];
    packet[12] = (char)255;
    long double heavy_hitter_sq_sum_err = 0.0;
    int less_than_threshold = 0;
    int packet_index = 1;
    int heavy_hitters = 0;
    // After seeing 10 entries less than the threshold we assume there won't be
    // any more (the loop is in order of decreasing expected frequency).
    while (less_than_threshold < 10) {
      int *packetCast = (int *)packet;

      packetCast[0] = packet_index;
      packetCast[1] = packet_index;
      packetCast[2] = packet_index;

      int actual = counter->query_index(packet_index);
      packet_index++;

      if (actual < threshold) {
        less_than_threshold++;
        continue;
      } else if (less_than_threshold > 0) {
        printf("Less than threshold: %d, but current frequency %d is greater "
               "than limit %d\n",
               less_than_threshold, actual, threshold);
      }

      int estimate = sketch->query(packet);

      long double err =
          ((long double)actual - (long double)estimate) / (long double)total;
      heavy_hitters++;
      heavy_hitter_sq_sum_err += err * err;
    }

    if (heavy_hitters == 0) {
      return 0.0;
    }

    return sqrt(heavy_hitter_sq_sum_err / (long double)heavy_hitters);
  }
};

void baseline_performance_fixed_mem_synthetic(int mem, char *trace_path,
                                              FILE *flat_results,
                                              FILE *traditional_results,
                                              FILE *skew_estimation) {
  const int k = 100;
  PacketCounter *counter = new PacketCounter(1 << 28);
  vector<SketchEvaluation *> variants{};

  const double e = exp(1.0);

  variants.reserve(24);

  for (int i = 1; i < 10; i++) {
    CountMinFlat *flat = new CountMinFlat(k);
    flat->initialize(mem, i, 10);
    variants.push_back(new SketchEvaluation(flat, Flat));

    CountMinTopK *regular = new CountMinTopK(k);
    regular->initialize(mem / i, i, 10);
    variants.push_back(new SketchEvaluation(regular, Traditional));
  }

  ZipfReader *reader = new ZipfReader(trace_path);

  long long sum_sq_err = 0;
  long total = 0;
  long next_estimation_index = 1;

  fprintf(skew_estimation,
          "variant,hash functions,packets read,skew estimate\n");

  while (true) {
    char dest[FT_SIZE] = {};
    int ret = reader->read_next_packet(dest);

    // Reached end
    if (ret < FT_SIZE) {
      break;
    }

    if (ret != FT_SIZE) {
      throw std::runtime_error(
          "Failed sanity check - read incorrect number of bytes");
    }
    total++;

    int actual = counter->increment(dest);

    for (auto variant : variants) {
      variant->handle_packet(dest, actual, total);
      if (total == next_estimation_index) {
        double skew_estimate = variant->sketch->estimate_skew();
        fprintf(skew_estimation, "%s,%d,%ld,%f\n",
                variant->variant_name.c_str(),
                variant->get_hash_function_count(), total, skew_estimate);
      }
    }

    if (total == next_estimation_index) {
      next_estimation_index *= 2;
    }
  }

  printf("calculating error stats for trace %s\n", trace_path);

  fprintf(flat_results,
          "hash functions,normalized error,heavy hitter error,sketch error "
          "e,sketch error 2e,sketch error 4e,sketch error 8e\n");
  fprintf(traditional_results,
          "hash functions,normalized error,heavy hitter error,sketch error "
          "e,sketch error 2e,sketch error 4e,sketch error 8e\n");

  // set phi=0.1%
  int heavy_hitter_threshold = (int)(0.001 * (double)total);

  char *packet = new char[FT_SIZE];
  packet[12] = (char)255;
  for (auto variant : variants) {
    double normalized_error = variant->normalized_error(total);

    double heavy_hitter_err =
        variant->heavy_hitter_err(counter, heavy_hitter_threshold, total);

    auto sketch = variant->sketch;
    double sketch_error_e = sketch->sketch_error(e, total, mem);
    double sketch_error_2e = sketch->sketch_error(2.0 * e, total, mem);
    double sketch_error_4e = sketch->sketch_error(4.0 * e, total, mem);
    double sketch_error_8e = sketch->sketch_error(8.0 * e, total, mem);

    int hash_functions = sketch->get_hash_function_count();

    FILE *results_output = flat_results;
    if (variant->variant == Traditional) {
      results_output = traditional_results;
    }

    fprintf(results_output, "%d,%E,%E,%E,%E,%E,%E\n", hash_functions,
            normalized_error, heavy_hitter_err, sketch_error_e, sketch_error_2e,
            sketch_error_4e, sketch_error_8e);
  }

  fclose(flat_results);
  fclose(traditional_results);
}

void baseline_performance_fixed_mem_real_world(int mem, char *trace_path,
                                               FILE *flat_results,
                                               FILE *traditional_results,
                                               FILE *skew_estimation) {
  const int k = 100;
  HashPacketCounter *counter = new HashPacketCounter(1 << 28);
  vector<SketchEvaluation *> variants{};

  TopK trueTopK = TopK(2000);

  const double e = exp(1.0);

  variants.reserve(24);

  for (int i = 1; i < 10; i++) {
    CountMinFlat *flat = new CountMinFlat(k);
    flat->initialize(mem, i, 10);
    variants.push_back(new SketchEvaluation(flat, Flat));

    CountMinTopK *regular = new CountMinTopK(k);
    regular->initialize(mem / i, i, 10);
    variants.push_back(new SketchEvaluation(regular, Traditional));
  }

  ZipfReader *reader = new ZipfReader(trace_path);

  long long sum_sq_err = 0;
  long total = 0;

  long next_estimation_index = 1;

  fprintf(skew_estimation,
          "variant,hash functions,packets read,skew estimate\n");

  while (true) {
    char dest[FT_SIZE] = {};
    int ret = reader->read_next_packet(dest);

    // Reached end
    if (ret < FT_SIZE) {
      break;
    }

    if (ret != FT_SIZE) {
      throw std::runtime_error(
          "Failed sanity check - read incorrect number of bytes");
    }
    total++;

    int actual = counter->increment(dest);
    trueTopK.update(dest, actual);

    for (auto variant : variants) {
      variant->handle_packet(dest, actual, total);
      if (total == next_estimation_index) {
        double skew_estimate = variant->sketch->estimate_skew();
        fprintf(skew_estimation, "%s,%d,%ld,%f\n",
                variant->variant_name.c_str(),
                variant->get_hash_function_count(), total, skew_estimate);
      }
    }

    if (total == next_estimation_index) {
      next_estimation_index *= 2;
    }
  }

  printf("calculating error stats for trace %s\n", trace_path);

  fprintf(flat_results,
          "hash functions,normalized error,heavy hitter error,sketch error "
          "e,sketch error 2e,sketch error 4e,sketch error 8e\n");
  fprintf(traditional_results,
          "hash functions,normalized error,heavy hitter error,sketch error "
          "e,sketch error 2e,sketch error 4e,sketch error 8e\n");

  // set phi=0.1%
  int heavy_hitter_threshold = (int)(0.001 * (double)total);

  char *packet = new char[FT_SIZE];
  packet[12] = (char)255;
  for (auto variant : variants) {
    double normalized_error = variant->normalized_error(total);

    double heavy_hitter_err = variant->heavy_hitter_err_real_world(
        trueTopK, heavy_hitter_threshold, total);

    auto sketch = variant->sketch;
    double sketch_error_e = sketch->sketch_error(e, total, mem);
    double sketch_error_2e = sketch->sketch_error(2.0 * e, total, mem);
    double sketch_error_4e = sketch->sketch_error(4.0 * e, total, mem);
    double sketch_error_8e = sketch->sketch_error(8.0 * e, total, mem);

    int hash_functions = sketch->get_hash_function_count();

    FILE *results_output = flat_results;
    if (variant->variant == Traditional) {
      results_output = traditional_results;
    }

    fprintf(results_output, "%d,%E,%E,%E,%E,%E,%E\n", hash_functions,
            normalized_error, heavy_hitter_err, sketch_error_e, sketch_error_2e,
            sketch_error_4e, sketch_error_8e);
  }

  fclose(flat_results);
  fclose(traditional_results);
}

// Tests the performance of the dynamic sketches
void dynamic_performance_fixed_mem_synthetic(int mem, char *trace_path,
                                             FILE *results,
                                             FILE *skew_estimation) {
  const int k = 100;
  PacketCounter *counter = new PacketCounter(1 << 28);
  vector<SketchEvaluation *> variants{};

  const double e = exp(1.0);

  variants.reserve(10);
  const int start_hash_functions = 4;

  for (int i = 0; i <= 1; i++) {
    // Error metrics are numbered 0 to 1 inclusive.
    ErrorMetric metric = (ErrorMetric)i;

    DynamicCountMin *flat_bounds = new DynamicCountMin(k, metric, true);
    flat_bounds->initialize(mem, start_hash_functions, 10);
    SketchEvaluation *evaluation_bounds =
        new SketchEvaluation(flat_bounds, Flat);
    evaluation_bounds->variant_name = error_metric_name(metric) + "-bounds";
    variants.push_back(evaluation_bounds);

    DynamicCountMin *flat_lowest = new DynamicCountMin(k, metric, false);
    flat_lowest->initialize(mem, start_hash_functions, 10);
    SketchEvaluation *evaluation_lowest =
        new SketchEvaluation(flat_lowest, Flat);
    evaluation_lowest->variant_name = error_metric_name(metric) + "-lowest";
    variants.push_back(evaluation_lowest);
  }

  ZipfReader *reader = new ZipfReader(trace_path);

  long long sum_sq_err = 0;
  long total = 0;
  long next_estimation_index = 1;

  fprintf(skew_estimation,
          "variant,hash functions,packets read,skew estimate\n");

  while (true) {
    char dest[FT_SIZE] = {};
    int ret = reader->read_next_packet(dest);

    // Reached end
    if (ret < FT_SIZE) {
      break;
    }

    if (ret != FT_SIZE) {
      throw std::runtime_error(
          "Failed sanity check - read incorrect number of bytes");
    }
    total++;

    int actual = counter->increment(dest);

    for (auto variant : variants) {
      variant->handle_packet(dest, actual, total);
      if (total == next_estimation_index) {
        double skew_estimate = variant->sketch->estimate_skew();
        fprintf(skew_estimation, "%s,%d,%ld,%f\n",
                variant->variant_name.c_str(),
                variant->get_hash_function_count(), total, skew_estimate);
      }
    }

    if (total == next_estimation_index) {
      next_estimation_index *= 2;
    }
  }

  printf("calculating error stats for trace %s\n", trace_path);

  fprintf(results, "variant,normalized error,heavy hitter error,sketch error "
                   "e,sketch error 2e,sketch error 4e,sketch error 8e\n");

  // set phi=0.1%
  int heavy_hitter_threshold = (int)(0.001 * (double)total);

  char *packet = new char[FT_SIZE];
  packet[12] = (char)255;
  for (auto variant : variants) {
    double normalized_error = variant->normalized_error(total);

    double heavy_hitter_err =
        variant->heavy_hitter_err(counter, heavy_hitter_threshold, total);

    auto sketch = variant->sketch;
    double sketch_error_e = sketch->sketch_error(e, total, mem);
    double sketch_error_2e = sketch->sketch_error(2.0 * e, total, mem);
    double sketch_error_4e = sketch->sketch_error(4.0 * e, total, mem);
    double sketch_error_8e = sketch->sketch_error(8.0 * e, total, mem);

    int hash_functions = sketch->get_hash_function_count();

    fprintf(results, "%s,%E,%E,%E,%E,%E,%E\n", variant->variant_name.c_str(),
            normalized_error, heavy_hitter_err, sketch_error_e, sketch_error_2e,
            sketch_error_4e, sketch_error_8e);
  }

  fclose(results);
}

void dynamic_performance_fixed_mem_real_world(int mem, char *trace_path,
                                              FILE *results,
                                              FILE *skew_estimation) {

  const int k = 100;
  HashPacketCounter *counter = new HashPacketCounter(1 << 28);
  TopK trueTopK = TopK(2000);

  // PacketCounter *counter = new PacketCounter(1 << 28);
  vector<SketchEvaluation *> variants{};

  const double e = exp(1.0);

  variants.reserve(10);
  const int start_hash_functions = 4;

  for (int i = 0; i <= 1; i++) {
    // Error metrics are numbered 0 to 1 inclusive.
    ErrorMetric metric = (ErrorMetric)i;

    DynamicCountMin *flat_bounds = new DynamicCountMin(k, metric, true);
    flat_bounds->initialize(mem, start_hash_functions, 10);
    SketchEvaluation *evaluation_bounds =
        new SketchEvaluation(flat_bounds, Flat);
    evaluation_bounds->variant_name = error_metric_name(metric) + "-bounds";
    variants.push_back(evaluation_bounds);

    DynamicCountMin *flat_lowest = new DynamicCountMin(k, metric, false);
    flat_lowest->initialize(mem, start_hash_functions, 10);
    SketchEvaluation *evaluation_lowest =
        new SketchEvaluation(flat_lowest, Flat);
    evaluation_lowest->variant_name = error_metric_name(metric) + "-lowest";
    variants.push_back(evaluation_lowest);
  }

  ZipfReader *reader = new ZipfReader(trace_path);

  long long sum_sq_err = 0;
  long total = 0;
  long next_estimation_index = 1;

  fprintf(skew_estimation,
          "variant,hash functions,packets read,skew estimate\n");

  while (true) {
    char dest[FT_SIZE] = {};
    int ret = reader->read_next_packet(dest);

    // Reached end
    if (ret < FT_SIZE) {
      break;
    }

    if (ret != FT_SIZE) {
      throw std::runtime_error(
          "Failed sanity check - read incorrect number of bytes");
    }
    total++;

    int actual = counter->increment(dest);
    trueTopK.update(dest, actual);

    for (auto variant : variants) {
      variant->handle_packet(dest, actual, total);
      if (total == next_estimation_index) {
        double skew_estimate = variant->sketch->estimate_skew();
        fprintf(skew_estimation, "%s,%d,%ld,%f\n",
                variant->variant_name.c_str(),
                variant->get_hash_function_count(), total, skew_estimate);
      }
    }

    if (total == next_estimation_index) {
      next_estimation_index *= 2;
    }
  }

  printf("calculating error stats for trace %s\n", trace_path);

  fprintf(results, "variant,normalized error,heavy hitter error,sketch error "
                   "e,sketch error 2e,sketch error 4e,sketch error 8e\n");

  // set phi=0.1%
  int heavy_hitter_threshold = (int)(0.001 * (double)total);

  char *packet = new char[FT_SIZE];
  packet[12] = (char)255;
  for (auto variant : variants) {
    double normalized_error = variant->normalized_error(total);

    double heavy_hitter_err = variant->heavy_hitter_err_real_world(
        trueTopK, heavy_hitter_threshold, total);

    auto sketch = variant->sketch;
    double sketch_error_e = sketch->sketch_error(e, total, mem);
    double sketch_error_2e = sketch->sketch_error(2.0 * e, total, mem);
    double sketch_error_4e = sketch->sketch_error(4.0 * e, total, mem);
    double sketch_error_8e = sketch->sketch_error(8.0 * e, total, mem);

    int hash_functions = sketch->get_hash_function_count();

    fprintf(results, "%s,%E,%E,%E,%E,%E,%E\n", variant->variant_name.c_str(),
            normalized_error, heavy_hitter_err, sketch_error_e, sketch_error_2e,
            sketch_error_4e, sketch_error_8e);
  }

  fclose(results);
}
