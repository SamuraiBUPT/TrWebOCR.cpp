# FastTrWeb

High throughput & inference engine for [Tr](https://github.com/myhub/tr) with cplusplus extensions, updated for environments like ubuntu 20.04+.

The web service depends on:

+ [httplib](https://github.com/yhirose/cpp-httplib)
+ [nlohmann/json](https://github.com/nlohmann/json)

The two repository adopt `one header file only` way, which is really convenient for web service developing.

Let's just keep it simple, and fast.



**Note: this project is still in early development and not ready for production use. May finish in several days.**



# Environment and settings

As the release files from [Tr](https://github.com/myhub/tr) has only `.so` files, we can never figure out how it was implemented, but following the tutorials and examples from the scripts it offered, we may **infer** that:

+ The `tr_run` function can be executed in multi-threads environment, which is of vital importance to accelerate the inference latency and throughput.
+ The input of `tr_run` function can be a path of local image, or the pointer of ndarray, for easy-development reasons I adopt `local image path` (const char*) as input. Another kind of input will try later.



## TODO
+ [x] support inference
+ [ ] GPU support
+ [ ] Image rotation C++ implement.

# Benchmark

Inference 100 requests, the concurrency is 20, CPU mode:

![benchmark1](docs/time_comparison.png)