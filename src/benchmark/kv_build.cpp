// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/*
 * Build and save alex to file
 *
 * Examples:
    ./kv_build --keys_file=../resources/fb_1M_uint64 --keys_file_type=sosd --total_num_keys=1000000 --db_path=tmp/alex_whole/fb_1M_uint64
    ./kv_build --keys_file=../resources/gmm_k10_1M_uint64 --keys_file_type=sosd --total_num_keys=1000000 --db_path=tmp/alex_whole/gmm_k10_1M_uint64
 */

#include "../core/alex.h"

#include <iomanip>

#include "flags.h"
#include "utils.h"

#ifndef __has_include
  static_assert(false, "__has_include not supported");
#else
#  if __cplusplus >= 201703L && __has_include(<filesystem>)
#    include <filesystem>
     namespace fs = std::filesystem;
#  elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
     namespace fs = std::experimental::filesystem;
#  elif __has_include(<boost/filesystem.hpp>)
#    include <boost/filesystem.hpp>
     namespace fs = boost::filesystem;
#  endif
#endif

// Modify these if running your own workload
#define KEY_TYPE uint64_t
#define PAYLOAD_TYPE uint64_t  // to store rank

void print_stat(alex::Alex<KEY_TYPE, PAYLOAD_TYPE>& index) {
  auto stats = index.get_stats();
  std::cout << "Alex stats" << std::endl;
  std::cout << "\tnum_keys: " << stats.num_keys << std::endl;
  std::cout << "\tnum_model_nodes: " << stats.num_model_nodes << std::endl;
  std::cout << "\tnum_data_nodes: " << stats.num_data_nodes << std::endl;
  std::cout << "\tnum_expand_and_scales: " << stats.num_expand_and_scales << std::endl;
  std::cout << "\tnum_expand_and_retrains: " << stats.num_expand_and_retrains << std::endl;
  std::cout << "\tnum_downward_splits: " << stats.num_downward_splits << std::endl;
  std::cout << "\tnum_sideways_splits: " << stats.num_sideways_splits << std::endl;
  std::cout << "\tnum_model_node_expansions: " << stats.num_model_node_expansions << std::endl;
  std::cout << "\tnum_model_node_splits: " << stats.num_model_node_splits << std::endl;
  std::cout << "\tnum_downward_split_keys: " << stats.num_downward_split_keys << std::endl;
  std::cout << "\tnum_sideways_split_keys: " << stats.num_sideways_split_keys << std::endl;
  std::cout << "\tnum_model_node_expansion_pointers: " << stats.num_model_node_expansion_pointers << std::endl;
  std::cout << "\tnum_model_node_split_pointers: " << stats.num_model_node_split_pointers << std::endl;
  std::cout << "\tnum_node_lookups: " << stats.num_node_lookups << std::endl;
  std::cout << "\tnum_lookups: " << stats.num_lookups << std::endl;
  std::cout << "\tnum_inserts: " << stats.num_inserts << std::endl;
  std::cout << "\tsplitting_time: " << stats.splitting_time << std::endl;
  std::cout << "\tcost_computation_time: " << stats.cost_computation_time << std::endl;
}

/*
 * Required flags:
 * --keys_file              path to the file that contains keys
 * --keys_file_type         file type of keys_file (options: binary | text | sosd)
 * --total_num_keys         total number of keys in the keys file
 * --db_path                path to save built alex
 */
int main(int argc, char* argv[]) {
  auto flags = parse_flags(argc, argv);
  std::string keys_file_path = get_required(flags, "keys_file");
  std::string keys_file_type = get_required(flags, "keys_file_type");
  auto total_num_keys = stoi(get_required(flags, "total_num_keys"));
  std::string db_path = get_required(flags, "db_path");

  // Prepare directory
  if (!fs::is_directory(db_path) || !fs::exists(db_path)) {
    fs::path db_path_p(db_path);
    fs::create_directories(db_path_p.parent_path());
    std::cout << "Created directory " << db_path_p.parent_path() << std::endl;
  }

  // Read keys from file
  auto keys = new KEY_TYPE[total_num_keys];
  if (keys_file_type == "binary") {
    load_binary_data(keys, total_num_keys, keys_file_path);
  } else if (keys_file_type == "text") {
    load_text_data(keys, total_num_keys, keys_file_path);
  } else if (keys_file_type == "sosd") {
    load_sosd_data(keys, total_num_keys, keys_file_path);
  } else {
    std::cerr << "--keys_file_type must be either 'binary' or 'text'"
              << std::endl;
    return 1;
  }

  // Combine bulk loaded keys with their ranks
  auto values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[total_num_keys];
  for (int i = 0; i < total_num_keys; i++) {
    if (i % (total_num_keys / 10) == 0) {
      std::cout << "idx= " << i << ": key= " << keys[i] << std::endl;
    }
    values[i].first = keys[i];
    values[i].second = i;
  }
  std::sort(values, values + total_num_keys,
            [](auto const& a, auto const& b) { return a.first < b.first; });
  delete[] keys;
  std::cout << "Loaded dataset of size " << total_num_keys << std::endl;

  // Create ALEX and bulk load
  auto bulk_load_start_time = std::chrono::high_resolution_clock::now();
  alex::Alex<KEY_TYPE, PAYLOAD_TYPE> index;
  index.bulk_load(values, total_num_keys);
  auto bulk_load_end_time = std::chrono::high_resolution_clock::now();
  auto bulk_load_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          bulk_load_end_time - bulk_load_start_time)
                          .count();
  std::cout << "Bulk load completed in " << bulk_load_time / 1e9 << " s" << std::endl;
  print_stat(index);

  // Serialize and save to file
  {
    std::ofstream ofs(db_path);
    boost::archive::binary_oarchive oa(ofs);
    oa << index;
    std::cout << "Saved to " << db_path << std::endl;
  }

  // Test load alex from file
  {
    alex::Alex<KEY_TYPE, PAYLOAD_TYPE> new_index;
    std::ifstream ifs(db_path);
    boost::archive::binary_iarchive ia(ifs);
    ia >> new_index;
    std::cout << "Tested loaded from " << db_path << std::endl;
  }

  delete[] values;
}
