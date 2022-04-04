#pragma once

#include "Defs.hpp"
#include <assert.h>
#include <map>
#include <stdint.h>
#include <unordered_map>
#include <vector>

using namespace std;

template <class K, class V> class orderedMapTopK {
  unordered_map<K, V> kvm;
  // map<pair<K, V>, K, cmpByStringLength<K, V>> inverse_kvm;
  map<pair<V, K>, K> inverse_kvm;

  int k;

public:
  orderedMapTopK<K, V>(int k) { this->k = k; }

  vector<pair<K, V>> items() {
    vector<pair<K, V>> kv;
    for (auto it = inverse_kvm.begin(); it != inverse_kvm.end(); ++it) {
      kv.push_back(make_pair(it->first.second, it->first.first));
    }
    return kv;
  }

  void update(K key, V value) {

    bool key_in_map = (kvm.find(key) == kvm.end()) ? false : true;

    if (!key_in_map && kvm.size() == k) {
      V min_val = (inverse_kvm.begin()->first).first;
      K min_val_key = (inverse_kvm.begin()->first).second;

      if (value > min_val) {
        kvm.erase(min_val_key);
        inverse_kvm.erase(make_pair(min_val, min_val_key));
        kvm[key] = value;
        inverse_kvm[make_pair(value, key)] = key;
      }
    } else if (key_in_map || kvm.size() < k) {
      if (key_in_map) {
        inverse_kvm.erase(pair<V, K>(kvm[key], key));
      }
      inverse_kvm[pair<V, K>(value, key)] = key;
      kvm[key] = value;
    }
  }
};

