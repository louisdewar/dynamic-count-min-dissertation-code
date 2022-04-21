#include "topK.hpp"

TopK::TopK(int k) { this->k = k; }

vector<pair<std::array<char, FT_SIZE>, uint32_t>> TopK::items() {
  vector<pair<std::array<char, FT_SIZE>, uint32_t>> kv;
  for (auto it = inverse_kvm.begin(); it != inverse_kvm.end(); ++it) {
    kv.push_back(make_pair(it->first.second, it->first.first));
  }
  return kv;
}

void TopK::update(const char *packet, uint32_t value) {
  std::array<char, FT_SIZE> key;
  memcpy(&key, packet, FT_SIZE);

  bool key_in_map = (kvm.find(key) == kvm.end()) ? false : true;

  if (!key_in_map && kvm.size() == k) {
    uint32_t min_val = (inverse_kvm.begin()->first).first;
    std::array<char, FT_SIZE> min_val_key = (inverse_kvm.begin()->first).second;

    if (value > min_val) {
      kvm.erase(min_val_key);
      inverse_kvm.erase(make_pair(min_val, min_val_key));
      kvm[key] = value;
      inverse_kvm[make_pair(value, key)] = key;
    }
  } else if (key_in_map || kvm.size() < k) {
    if (key_in_map) {
      inverse_kvm.erase(
          pair<uint32_t, std::array<char, FT_SIZE>>(kvm[key], key));
    }
    inverse_kvm[pair<uint32_t, std::array<char, FT_SIZE>>(value, key)] = key;
    kvm[key] = value;
  }
}
