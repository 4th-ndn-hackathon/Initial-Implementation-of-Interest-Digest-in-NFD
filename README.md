# Initial Implementation of Interest Digest in NFD

Project members:

* [Junxiao Shi](https://www.cs.arizona.edu/~shijunxiao/) (The University of Arizona)
* [Lei Pi](https://www.linkedin.com/in/leipi1) (University of Memphis)
* [Qi Zhao](https://irl.cs.ucla.edu/~qzhao/) (University of California, Los Angeles)
* [Chengyu Fan](https://www.linkedin.com/in/chengyu-fan-5a56b227) (Colorado State University)

[4th NDN Hackathon presentation](https://www.slideshare.net/yoursunny/nfd-interestdigest)

## Goal

Implement [Interest digest](https://redmine.named-data.net/issues/3333) in
NFD and benchmark its benefit on forwarding performance.

## Codebase

ndn-cxx-baseline and NFD-baseline branches contain the code we started with.
ndn-cxx and NFD branches contain commits that implement Interest digest.

## Benchmark

Benchmarking method is documented in `benchmark.md`.

With 4-component names like `/N/P1/ping/8888`:

* baseline: router CPU usage 93-96%, RTT 3.83ms
* Interest digest: router CPU usage 91-94%, RTT 2.44ms

With 11-component names like `/A/B/C/D/E/F/G/H/P1/ping/8888`:

* baseline: router CPU usage 95-98%, RTT 13.19ms
* Interest digest: router CPU usage 95-97%, RTT 13.12ms

