import subprocess
import re

print("Sweeping alpha_y and alpha_z on 1e16:")
for az in [1.0, 1.1, 1.2, 1.3, 1.5, 2.0]:
    for ay in [12.0, 14.0, 16.0, 18.0, 20.0, 22.0]:
        cmd = ["./primecount_kim/build/primecount", "1e16", f"--alpha-y={ay}", f"--alpha-z={az}", "--time"]
        res = subprocess.run(cmd, stdout=subprocess.PIPE, text=True)
        m = re.search(r"Seconds:\s+([\d\.]+)", res.stdout)
        if m:
            print(f"alpha_z={az:<4} alpha_y={ay:<5} -> {float(m.group(1)):.3f}s")
