#!/usr/bin/env python3
"""Generate include/web_content.h from data/www/ source files.

Reads:
  data/www/index.html  -> raw C string literal (PROGMEM)
  data/www/style.css   -> gzip -> C uint8_t array (PROGMEM)
  data/www/script.js   -> gzip -> C uint8_t array (PROGMEM)

Outputs:
  include/web_content.h

Usage:
  python scripts/generate_web_content.py
  (run from project root)
"""

import gzip
import os
import sys


def find_project_root():
    """Find the project root by looking for platformio.ini."""
    # Try script's parent directory first
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(script_dir)
    if os.path.exists(os.path.join(root, "platformio.ini")):
        return root
    # Try current working directory
    if os.path.exists("platformio.ini"):
        return os.getcwd()
    print("ERROR: Cannot find project root (no platformio.ini found)")
    print("       Run from project root or from scripts/ directory")
    sys.exit(1)


def gzip_to_c_array(data, varname):
    """Compress data with gzip and format as a C uint8_t array."""
    compressed = gzip.compress(data, compresslevel=9)

    lines = []
    for i in range(0, len(compressed), 16):
        chunk = compressed[i : i + 16]
        hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {hex_str}")

    body = ",\n".join(lines)
    result = (
        f"const uint8_t {varname}[] PROGMEM = {{\n"
        f"{body}\n"
        f"}};\n\n"
        f"const size_t {varname}_len = sizeof({varname});"
    )
    return result, len(data), len(compressed)


def main():
    root = find_project_root()

    src_html = os.path.join(root, "data", "www", "index.html")
    src_css = os.path.join(root, "data", "www", "style.css")
    src_js = os.path.join(root, "data", "www", "script.js")
    out_file = os.path.join(root, "include", "web_content.h")

    # Verify source files exist
    missing = []
    for f in [src_html, src_css, src_js]:
        if not os.path.exists(f):
            missing.append(f)
    if missing:
        print("ERROR: Missing source files:")
        for f in missing:
            print(f"  {f}")
        sys.exit(1)

    # Read HTML as text
    with open(src_html, "r", encoding="utf-8") as f:
        html = f.read()

    # Safety check: raw string literal delimiter must not appear in HTML
    if ')rawliteral"' in html:
        print("ERROR: index.html contains ')rawliteral\"' which would break the C raw string literal")
        sys.exit(1)

    # Read and gzip CSS and JS
    with open(src_css, "rb") as f:
        css_data = f.read()
    with open(src_js, "rb") as f:
        js_data = f.read()

    css_array, css_orig, css_gz = gzip_to_c_array(css_data, "web_style_css_gz")
    js_array, js_orig, js_gz = gzip_to_c_array(js_data, "web_script_js_gz")

    # Write web_content.h
    with open(out_file, "w", encoding="utf-8", newline="\n") as out:
        out.write("#ifndef WEB_CONTENT_H\n")
        out.write("#define WEB_CONTENT_H\n\n")
        out.write("#include <Arduino.h>\n\n")

        # HTML as raw string literal
        out.write('const char web_index_html[] PROGMEM = R"rawliteral(\n')
        out.write(html)
        if not html.endswith("\n"):
            out.write("\n")
        out.write(')rawliteral";\n\n')

        # CSS as gzipped byte array
        out.write(css_array)
        out.write("\n\n")

        # JS as gzipped byte array
        out.write(js_array)
        out.write("\n\n")

        out.write("#endif // WEB_CONTENT_H\n")

    # Summary
    html_size = len(html.encode("utf-8"))
    total_progmem = html_size + css_gz + js_gz
    print(f"Generated {out_file}")
    print(f"  HTML:  {html_size:>7,} bytes (raw string)")
    print(f"  CSS:   {css_orig:>7,} -> {css_gz:>6,} bytes (gzip, {100 - css_gz * 100 // css_orig}% reduction)")
    print(f"  JS:    {js_orig:>7,} -> {js_gz:>6,} bytes (gzip, {100 - js_gz * 100 // js_orig}% reduction)")
    print(f"  Total PROGMEM: {total_progmem:,} bytes ({total_progmem / 1024:.1f} KB)")


if __name__ == "__main__":
    main()
