import subprocess
import re

best_t = 999.0
best_params = ()
for az in [1.0, 1.5, 2.0, 2.5, 3.0]:
    for ay in [10.0, 12.0, 14.0, 16.0, 18.0]:
        cmd = ["./primecount_kim/build/primecount", "1e15", f"--alpha-y={ay}", f"--alpha-z={az}", "--time"]
        res = subprocess.run(cmd, stdout=subprocess.PIPE, text=True)
        m = re.search(r"Seconds:\s+([\d\.]+)", res.stdout)
        if m:
            t = float(m.group(1))
            if t < best_t:
                best_t = t
                best_params = (az, ay)
print(f"1e15 BEST: {best_t:.3f}s with alpha_z={best_params[0]} alpha_y={best_params[1]}")
