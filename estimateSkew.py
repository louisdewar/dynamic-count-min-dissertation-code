import numpy as np

def harmonic_number(domain, skew):
    return sum([1 / n**skew for n in range(1, domain)])

def topk_frequencies(k, domain, skew):
    harmonic_n = harmonic_number(domain, skew)

    return [1 / (harmonic_n * i**skew) for i in range(1, k + 1)]

# The estimates are actually kH (where k is 1/total elements)
def harmonic_number_estimates(topk, skew):
    return [1 / ((val + 1) * i**skew) for i, val in enumerate(topk, start=1)]

DOMAIN = 2 ** 18;

skew = 0.9
frequencies = topk_frequencies(10, DOMAIN, skew)
sampled = [np.random.binomial(100, frequency) for frequency in frequencies]

for i in range(6, 13):
    skew = i/10
    estimates = harmonic_number_estimates(sampled, skew)

    print(skew, np.var(estimates), estimates)

