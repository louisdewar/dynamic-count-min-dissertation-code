#include "Defs.hpp"
#include <map>
#include <unordered_map>

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
    for (auto it = kvm.begin(); it != kvm.end(); ++it) {
      kv.push_back(make_pair(it->first, it->second));
    }
    return kv;
  }

  void update(K key, V value) {

    bool key_in_map = (kvm.find(key) == kvm.end()) ? false : true;

    if (!key_in_map && kvm.size() == k) {
      /*
      V min_val = kvm.begin()->second;
      K min_val_key = kvm.begin()->first;
      for (auto it = kvm.begin(); it != kvm.end(); ++it)
      {
              if (it->second < min_val)
              {
                      min_val = it->second;
                      min_val_key = it->first;
              }
      }
      */

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

    // if (inverse_kvm.size() != kvm.size())
    //{
    //	system("pause");
    // }
  }
};
