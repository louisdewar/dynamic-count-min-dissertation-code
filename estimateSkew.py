# import numpy as np
# 
# def harmonic_number(domain, skew):
#     return sum([1 / n**skew for n in range(1, domain)])
# 
# def topk_frequencies(k, domain, skew):
#     harmonic_n = harmonic_number(domain, skew)
# 
#     return [1 / (harmonic_n * i**skew) for i in range(1, k + 1)]
# 
# # The estimates are actually kH (where k is 1/total elements)
# def harmonic_number_estimates(topk, skew):
#     return [1 / (val * i**skew) for i, val in enumerate(topk, start=1)]
# 
# DOMAIN = 2 ** 18;
# 
# skew = 0.6
# frequencies = topk_frequencies(100, DOMAIN, skew)
# sampled = [np.random.binomial(1000000, frequency) for frequency in frequencies]
# 
# for i in range(2, 13):
#     skew = i/10
#     estimates = harmonic_number_estimates(sampled, skew)
# 
#     print(skew, np.var(estimates))


import numpy as np
def estimate_skew():
    #a_range = [0.6,1.3]
    a_vals = [i * 0.01 for i in range(30,140, 5)]
    for a_est in a_vals:
        top_k_harmonic_est = [1/(i+1)**a_est / f for i,f in enumerate(top_k_freq)]
        print (np.std(top_k_harmonic_est),a_est, "mean =", np.mean(top_k_harmonic_est))
        
        
k = 10
ds = 1 << 20
a = 0.6

n = 100000000

harmonic = sum([1./i**a for i in range(1,ds+1)])

#top_k_freq = [1./i**a / harmonic for i in range(1,k+1)]
top_k_freq = [np.random.binomial(n, 1./i**a / harmonic) for i in range(1,k+1)]

print("True skew", a)
estimate_skew()

print(harmonic)
