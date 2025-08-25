import os
import signal
import time
from util import fork_and_exec

def run_benchmark(scheduler: str, priority: tuple[int, int]) -> None:
    server_pid = fork_and_exec("../tiny_file/service/build/tiny_file_service", "--n_sms", "1", "--sms_size", "8192", "--scheduler", scheduler)
    time.sleep(2)
    os.remove(f"output/credit/benchmark_{scheduler.lower()}_{','.join(map(str, priority))}_0.txt")
    os.remove(f"output/credit/benchmark_{scheduler.lower()}_{','.join(map(str, priority))}_1.txt")
    client_1_pid = fork_and_exec("../tiny_file/client/build/app/app", "--state", "ASYNC", "--files", "bin/input/benchmark.txt", "--priority", str(priority[0]), "--log", f"output/credit/benchmark_{scheduler.lower()}_{','.join(map(str, priority))}_0.txt")
    client_2_pid = fork_and_exec("../tiny_file/client/build/app/app", "--state", "ASYNC", "--files", "bin/input/benchmark.txt", "--priority", str(priority[1]), "--log", f"output/credit/benchmark_{scheduler.lower()}_{','.join(map(str, priority))}_1.txt")
    os.waitpid(client_1_pid, 0)
    os.waitpid(client_2_pid, 0)
    os.kill(server_pid, signal.SIGTERM)

if __name__ == '__main__':
    schedulers = ["CREDIT", "CREDIT", "ROUND_ROBIN"]
    priorities = [(16, 64), (16, 16), (16, 16)]
    for scheduler, priority in zip(schedulers, priorities):
        run_benchmark(scheduler=scheduler, priority=priority)
