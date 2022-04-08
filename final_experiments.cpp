#include "final_experiments.hpp"

/*
 * Experiments used in the final dissertation
 */

// struct flat_sketch {
//   CountMinFlat sketch;
//   FILE *output;
// };
//
// struct traditional_sketch {
//   CountMinFlat sketch;
//   FILE *output;
// };
// Tests the performance of a number of different sketches of both the regular
// and flat count min sketches with varying numbers of hash functions at a fixed
// memory on a single trace.
void baseline_performance_fixed_mem_synthetic(int mem, char *trace_path,
                                              FILE *flat_results,
                                              FILE *traditional_results,
                                              FILE *skew_estimation) {
  const int k = 100;
  PacketCounter *counter = new PacketCounter(1 << 28);
  vector<CountMinFlat *> flat_variants{};
  vector<CountMinTopK *> traditional_variants{};

  vector<long long> flat_sum_sq_err{};
  vector<long long> traditional_sum_sq_err{};

  const double e = exp(1.0);

  flat_variants.reserve(12);
  traditional_variants.reserve(12);

  for (int i = 1; i < 10; i++) {
    CountMinFlat *flat = new CountMinFlat(k);
    flat->initialize(mem, i, 10);
    flat_variants.push_back(flat);
    flat_sum_sq_err.push_back(0);

    CountMinTopK *regular = new CountMinTopK(k);
    regular->initialize(mem / i, i, 10);
    traditional_variants.push_back(regular);
    traditional_sum_sq_err.push_back(0);
  }

  ZipfReader *reader = new ZipfReader(trace_path);

  long long sum_sq_err = 0;
  long total = 0;
  long next_estimation_index = 1;

  fprintf(skew_estimation,
          "variant,hash functions,packets read,skew estimate\n");

  assert(flat_variants.size() == traditional_variants.size());
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

    for (int i = 0; i < flat_variants.size(); i++) {
      auto sketch = flat_variants[i];
      sketch->increment(dest);
      int estimate = sketch->query(dest);
      flat_sum_sq_err[i] +=
          (long long)(estimate - actual) * (estimate - actual);

      if (total == next_estimation_index) {
        double skew_estimate = sketch->estimate_skew();
        fprintf(skew_estimation, "flat,%d,%ld,%f\n", sketch->hash_count, total,
                skew_estimate);
      }
    }

    for (int i = 0; i < traditional_variants.size(); i++) {
      auto sketch = traditional_variants[i];
      sketch->increment(dest);
      int estimate = sketch->query(dest);
      traditional_sum_sq_err[i] +=
          (long long)(estimate - actual) * (estimate - actual);

      if (total == next_estimation_index) {
        double skew_estimate = sketch->estimate_skew();
        fprintf(skew_estimation, "traditional,%d,%ld,%f\n", sketch->height,
                total, skew_estimate);
      }
    }
    if (total == next_estimation_index) {
      next_estimation_index *= 2;
    }
  }

  printf("calculating error stats for trace %s\n", trace_path);

  fprintf(flat_results,
          "hash functions,normalized error,heavy hitter error,sketch error "
          "(e),sketch error (2e),sketch error (4e),sketch error (8e)\n");
  fprintf(traditional_results,
          "hash functions,normalized error,heavy hitter error,sketch error "
          "(e),sketch error (2e),sketch error (4e),sketch error (8e)\n");

  // set phi=0.1%
  int heavy_hitter_threshold = (int)(0.001 * (double)total);

  char *packet = new char[FT_SIZE];
  packet[12] = (char)255;
  for (int i = 0; i < flat_variants.size(); i++) {
    auto sketch = flat_variants[i];
    int hash_functions = sketch->hash_count;
    double normalized_error = sqrt((double)flat_sum_sq_err[i] / (double)total);

    long double heavy_hitter_sq_sum_err = 0.0;
    int less_than_threshold = 0;
    int packet_index = 1;
    // After seeing 10 entries less than the threshold we assume there won't be
    // any more (the loop is in order of decreasing expected frequency).
    while (less_than_threshold < 10) {
      int *packetCast = (int *)packet;

      packetCast[0] = packet_index;
      packetCast[1] = packet_index;
      packetCast[2] = packet_index;

      int actual = counter->query_index(packet_index);
      packet_index++;

      if (actual < heavy_hitter_threshold) {
        less_than_threshold++;
        continue;
      } else if (less_than_threshold > 0) {
        printf("Less than threshold: %d, but current frequency %d is greater "
               "than limit %d\n",
               less_than_threshold, actual, heavy_hitter_threshold);
      }

      int estimate = sketch->query(packet);

      long double err = (long double)actual - (long double)estimate;
      heavy_hitter_sq_sum_err += err * err;
    }

    double sketch_error_e = sketch->sketch_error(e, total, mem);
    double sketch_error_2e = sketch->sketch_error(2.0 * e, total, mem);
    double sketch_error_4e = sketch->sketch_error(4.0 * e, total, mem);
    double sketch_error_8e = sketch->sketch_error(8.0 * e, total, mem);

    double heavy_hitter_err = sqrt(heavy_hitter_sq_sum_err);

    fprintf(flat_results, "%d,%f,%f,%f,%f,%f,%f\n", hash_functions,
            normalized_error, heavy_hitter_err, sketch_error_e, sketch_error_2e,
            sketch_error_4e, sketch_error_8e);
  }

  for (int i = 0; i < traditional_variants.size(); i++) {
    auto sketch = traditional_variants[i];
    int hash_functions = sketch->height;
    double normalized_error =
        sqrt((double)traditional_sum_sq_err[i] / (double)total);

    long double heavy_hitter_sq_sum_err = 0.0;
    int less_than_threshold = 0;
    int packet_index = 1;
    // After seeing 10 entries less than the threshold we assume there won't be
    // any more (the loop is in order of decreasing expected frequency).
    while (less_than_threshold < 10) {
      int *packetCast = (int *)packet;

      packetCast[0] = packet_index;
      packetCast[1] = packet_index;
      packetCast[2] = packet_index;

      int actual = counter->query_index(packet_index);
      packet_index++;

      if (actual < heavy_hitter_threshold) {
        less_than_threshold++;
        continue;
      } else if (less_than_threshold > 0) {
        printf("Less than threshold: %d, but current frequency %d is greater "
               "than limit %d\n",
               less_than_threshold, actual, heavy_hitter_threshold);
      }

      int estimate = sketch->query(packet);

      long double err = (long double)actual - (long double)estimate;
      heavy_hitter_sq_sum_err += err * err;
    }

    double sketch_error_e = sketch->sketch_error(e, total, mem);
    double sketch_error_2e = sketch->sketch_error(2.0 * e, total, mem);
    double sketch_error_4e = sketch->sketch_error(4.0 * e, total, mem);
    double sketch_error_8e = sketch->sketch_error(8.0 * e, total, mem);

    double heavy_hitter_err = sqrt(heavy_hitter_sq_sum_err);
    // fprintf(traditional_results, "%d,%f,%f\n", hash_functions,
    // normalized_error,
    //         heavy_hitter_err);
    fprintf(traditional_results, "%d,%f,%f,%f,%f,%f,%f\n", hash_functions,
            normalized_error, heavy_hitter_err, sketch_error_e, sketch_error_2e,
            sketch_error_4e, sketch_error_8e);
  }

  fclose(flat_results);
  fclose(traditional_results);

  delete counter;
  delete reader;
}
