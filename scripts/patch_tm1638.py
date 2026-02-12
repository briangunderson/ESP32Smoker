"""Pre-build script: patch TM1638 library min() type mismatch for ESP32-S3.

The upstream tm1638-library uses min(7, intensity) which fails C++ template
deduction on ESP32-S3 (int vs byte). This patches it to min((byte)7, intensity).
"""
import os

def patch_tm1638():
    for root, dirs, files in os.walk(os.path.join(".pio", "libdeps")):
        if "TM16XX.cpp" in files:
            filepath = os.path.join(root, "TM16XX.cpp")
            with open(filepath, "r") as f:
                content = f.read()
            if "min(7, intensity)" in content:
                content = content.replace("min(7, intensity)", "min((byte)7, intensity)")
                with open(filepath, "w") as f:
                    f.write(content)
                print(f"[PATCH] Fixed min() type mismatch in {filepath}")
            else:
                print(f"[PATCH] {filepath} already patched")

patch_tm1638()
