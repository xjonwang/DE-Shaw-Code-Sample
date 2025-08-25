import matplotlib.pyplot as plt
import statistics
import numpy as np

if __name__ == "__main__":
    schedulers = ["CREDIT", "CREDIT", "ROUND_ROBIN"]
    priorities = [(16, 64), (16, 16), (16, 16)]
    low_prio_csts = [-1] * 3 
    high_prio_csts = [-1] * 3
    for i, (scheduler, priority) in enumerate(zip(schedulers, priorities)):
        with open(f"output/credit/benchmark_{scheduler.lower()}_{','.join(map(str, priority))}_0.txt") as f:
            lines = f.readlines()
            csts = [int(line) for line in lines]
            low_prio_csts[i] = statistics.mean(csts)
        with open(f"output/credit/benchmark_{scheduler.lower()}_{','.join(map(str, priority))}_1.txt") as f:
            lines = f.readlines()
            csts = [int(line) for line in lines]
            high_prio_csts[i] = statistics.mean(csts)

    fig, ax = plt.subplots()
    bar_width = 0.35
    x = np.arange(3)
    low_prio_bars = ax.bar(x - bar_width / 2, low_prio_csts, bar_width, label="Client 1 CST")
    high_prio_bars = ax.bar(x + bar_width / 2, high_prio_csts, bar_width, label="Client 2 CST")
    ax.set_xlabel("Scheduling Policy")
    ax.set_ylabel("CST (us)")
    ax.set_title(f"CST vs Scheduling Policy")
    ax.set_xticks(x)
    ax.set_xticklabels(["Credit 16/64", "Credit Equal", "Round Robin"])
    ax.legend()
    plt.savefig(f"output/charts/benchmark_credit.png", dpi=600)
    plt.show()
