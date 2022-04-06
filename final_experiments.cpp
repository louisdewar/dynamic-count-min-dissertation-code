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
                                              FILE *traditional_results) {
  PacketCounter *counter = new PacketCounter(1 << 28);
  vector<CountMinFlat *> flat_variants{};
  vector<CountMinBaselineFlexibleWidth *> traditional_variants{};

  vector<long long> flat_sum_sq_err{};
  vector<long long> traditional_sum_sq_err{};

  flat_variants.reserve(12);
  traditional_variants.reserve(12);

  for (int i = 1; i < 10; i++) {
    CountMinFlat *flat = new CountMinFlat(100);
    flat->initialize(mem, i, 10);
    flat_variants.push_back(flat);
    flat_sum_sq_err.push_back(0);

    CountMinBaselineFlexibleWidth *regular =
        new CountMinBaselineFlexibleWidth();
    regular->initialize(mem / i, i, 10);
    traditional_variants.push_back(regular);
    traditional_sum_sq_err.push_back(0);
  }

  ZipfReader *reader = new ZipfReader(trace_path);

  long long sum_sq_err = 0;
  long long total = 0;

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

    int actual = counter->increment(dest);

    for (int i = 0; i < flat_variants.size(); i++) {
      auto flat = flat_variants[i];
      flat->increment(dest);
      int estimate = flat->query(dest);
      flat_sum_sq_err[i] +=
          (long long)(estimate - actual) * (estimate - actual);
    }

    for (int i = 0; i < traditional_variants.size(); i++) {
      auto traditional = traditional_variants[i];
      traditional->increment(dest);
      int estimate = traditional->query(dest);
      traditional_sum_sq_err[i] +=
          (long long)(estimate - actual) * (estimate - actual);
    }
    // true_top_k.update(*(int *)&dest, actual);

    total++;
  }

  printf("calculating error stats for trace %s\n", trace_path);

  fprintf(flat_results, "hash functions,normalized error,heavy hitter error\n");
  fprintf(traditional_results,
          "hash functions,normalized error,heavy hitter error\n");

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

    double heavy_hitter_err = sqrt(heavy_hitter_sq_sum_err);

    fprintf(flat_results, "%d,%f,%f\n", hash_functions, normalized_error,
            heavy_hitter_err);
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
    while (heavy_hitter_threshold < 10) {
      int *packetCast = (int *)packet;

      packetCast[0] = packet_index;
      packetCast[1] = packet_index;
      packetCast[2] = packet_index;

      int actual = counter->query_index(i);
      int estimate = sketch->query(packet);

      long double err = (long double)actual - (long double)estimate;
      heavy_hitter_sq_sum_err += err * err;
      packet_index++;
    }

    double heavy_hitter_err = sqrt(heavy_hitter_sq_sum_err);
    fprintf(traditional_results, "%d,%f,%f\n", hash_functions, normalized_error,
            heavy_hitter_err);
  }

  fclose(flat_results);
  fclose(traditional_results);

  delete counter;
  delete reader;
}
