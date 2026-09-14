import subprocess
import re

print("Fine-tuning alpha_z and alpha_y on 1e16:")
best_t = 999.0
best_params = ()
for az in [1.8, 2.0, 2.2, 2.5, 2.8, 3.0, 3.5]:
    for ay in [10.0, 12.0, 13.0, 14.0, 15.0, 16.0]:
        cmd = ["./primecount_kim/build/primecount", "1e16", f"--alpha-y={ay}", f"--alpha-z={az}", "--time"]
        res = subprocess.run(cmd, stdout=subprocess.PIPE, text=True)
        m = re.search(r"Seconds:\s+([\d\.]+)", res.stdout)
        if m:
            t = float(m.group(1))
            if t < best_t:
                best_t = t
                best_params = (az, ay)
            print(f"alpha_z={az:<4} alpha_y={ay:<5} -> {t:.3f}s")
print(f"\nBEST: {best_t:.3f}s with alpha_z={best_params[0]} alpha_y={best_params[1]}")
