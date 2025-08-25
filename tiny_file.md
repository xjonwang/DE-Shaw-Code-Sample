# Tiny File

## Tiny File Service
### Building
```
$ cd tiny_file/service
$ mkdir build
$ cd build
$ cmake ..
$ make
$ ./tiny_file_service <args> (e.g. ./tiny_file_service --n_sms 1 --sms_size 4096 --scheduler CREDIT)
```
This will build the service binary `tiny_file_service` and the unit test binary `tests/tests`.
### Running
Parameters:
- `--n_sms`        Number of shared memory segments [required]
- `--sms_size`     Shared memory segment size in bytes [required]
- `--scheduler`    Set the scheduler algorithm to CREDIT or ROUND_ROBIN

## Tiny File Application
### Building
```
$ cd tiny_file/client
$ mkdir build
$ cd build
$ cmake ..
$ make
$ cp -r ../../../benchmark/input/ .
$ ./app/app <args> (e.g. ./app/app --state SYNC --files input/allfiles.txt)
```
This will build the client library and the sample application binary `app/app`. The paths in `allfiles.txt` and `benchmark.txt` are configured such that `input` directory must be in the same directory as the cwd when you run `app/app`.
### Running
Parameters:
- `--state`         Set the operation state to SYNC or ASYNC [required]
- `--file`          Specify the file path to be compressed
- `--files`         Specify the file containing the list of files to be compressed (one file per line)
    - One of `--file` or `--files` must be specified
- `--priority`      Specify the credit priority
- `--log`           Specify the log file path
NOTE that the client has some delay before exiting because the worker threads wait on a condition variable in the client side scheduler for 1 second before polling again. This gives it a chance to exit instead of waiting on the condition variable indefinitely. However, the time interval is set to 1 second to prevent frequent polling.

## Benchmarking
All results in the report were obtained from the benchmarks in `benchmarks/`. After building the service and client, you can run the benchmarks as follows (assuming you have `pyenv` and `pyenv-virtualenv` installed):
```
$ cd benchmarks
$ source scripts/env.sh pyenv
$ python <benchmark_name>_benchmark.py
$ python <benchmark_name>_visualize.py
```
