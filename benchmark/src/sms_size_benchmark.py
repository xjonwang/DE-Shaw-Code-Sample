import os
import signal
import time
from util import fork_and_exec

def start_server(program: str, *args) -> int:
    return fork_and_exec(program, *args)

def start_client(server_pid: int, program: str, *args) -> int:
    client_pid = fork_and_exec(program, *args)
    os.waitpid(client_pid, 0)
    os.kill(server_pid, signal.SIGTERM)

def run_benchmark(state: str, n_sms: int, sms_size: int) -> None:
    server_pid = start_server("../tiny_file/service/build/tiny_file_service", "--n_sms", str(n_sms), "--sms_size", str(sms_size))
    time.sleep(2)
    os.remove(f"output/{state.lower()}/benchmark_{n_sms}_{sms_size}.txt")
    start_client(server_pid, "../tiny_file/client/build/app/app", "--state", state, "--files", "bin/input/benchmark.txt", "--log", f"output/{state.lower()}/benchmark_{n_sms}_{sms_size}.txt")

if __name__ == '__main__':
    states = ["SYNC", "ASYNC"]
    num_segments = [1, 3, 5]
    sms_sizes: list[int] = [32, 64, 128, 256, 512, 1024, 2048, 4096, 8192]
    for state in states:
        for n_sms in num_segments:
            for sms_size in sms_sizes:
                run_benchmark(state, n_sms, sms_size)
