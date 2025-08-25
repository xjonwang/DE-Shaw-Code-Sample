import matplotlib.pyplot as plt
import statistics
import numpy as np

if __name__ == "__main__":
    num_segments = [1, 3, 5]
    sms_sizes = [32, 64, 128, 256, 512, 1024, 2048, 4096, 8192]
    for n_sms in num_segments:
        async_csts = [-1] * len(sms_sizes)
        sync_csts = [-1] * len(sms_sizes)
        for i, sms_size in enumerate(sms_sizes):
            with open(f"output/async/benchmark_{n_sms}_{sms_size}.txt") as f:
                lines = f.readlines()
                csts = [int(line) for line in lines]
                avg_cst = statistics.mean(csts)
                async_csts[i] = avg_cst
            with open(f"output/sync/benchmark_{n_sms}_{sms_size}.txt") as f:
                lines = f.readlines()
                csts = [int(line) for line in lines]
                avg_cst = statistics.mean(csts)
                sync_csts[i] = avg_cst

        bar_width = 0.35
        fig, ax = plt.subplots()
        x = np.arange(len(sms_sizes))
        async_bars = ax.bar(x - bar_width / 2, async_csts, bar_width, label="Async CST")
        sync_bars = ax.bar(x + bar_width / 2, sync_csts, bar_width, label="Sync CST")
        ax.set_xlabel("SMS Size")
        ax.set_ylabel("CST (us)")
        ax.set_title(f"CST vs SMS Size with {n_sms} Segments")
        ax.set_xticks(x)
        ax.set_xticklabels(sms_sizes)
        ax.legend()
        plt.savefig(f"output/charts/benchmark_{n_sms}.png", dpi=600)
        plt.show()
