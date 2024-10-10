#!/usr/bin/env python3

import fileinput
import os.path
import pathlib
import subprocess

BUILD_DIR = "Build/riscv64"

input = [line for line in list(fileinput.input(encoding="utf-8")) if ':' in line]

backtrace_start = [idx for idx, line in enumerate(input) if "Userspace backtrace:" in line][0] + 1
backtrace_end = [idx for idx, line in enumerate(input) if "Kernel backtrace:" in line][0]


def line_to_backtrace_entry(line):
    _, message = line.split(": ", 1)
    return int(message.split(" ", 1)[0], 16)


raw_backtrace = input[backtrace_start:backtrace_end]

# subtract 1 as backtrace entries contain return addresses
backtrace = [entry - 1 for entry in map(line_to_backtrace_entry, raw_backtrace)]

sepc_line = [line for line in input if "sepc:" in line][0]
sepc = int(sepc_line.split("sepc:")[1].strip(), 16)
backtrace.insert(0, sepc)

# elr_line = [line for line in input if "Exception Link Register:" in line][0]
# elr = int(elr_line.split("Exception Link Register:")[1].strip(), 16)
# backtrace.insert(0, elr)

# pc_line = [line for line in input if "pc=" in line][0]
# pc = int(pc_line.split('pc=')[1].split()[0].split(':')[1], 16)
# backtrace.insert(0, pc)

# print(list(map(hex, backtrace)))

regions_start = [idx for idx, line in enumerate(input) if "Process regions:" in line][0] + 2
regions_end = [idx for idx, line in enumerate(input) if "Kernel regions:" in line][0]


def line_to_region(line):
    _, message = line.split(": ", 1)
    begin_str, end_str = message.split(" -- ")
    name = end_str[45:].strip()
    end_str = end_str.split(" ", 1)[0]

    return int(begin_str, 16), int(end_str, 16), name


raw_regions = input[regions_start:regions_end]
regions = map(line_to_region, raw_regions)

mapped_files = {}
for start, end, name in regions:
    if len(name) > 0 and name[0] == "/":
        file = name.split(": ")[0]
        if "/usr/lib/Loader.so" in name:
            file = "/usr/lib/Loader.so"

        mapped_files.setdefault(file, {"start": 2**64 - 1, "end": 0})
        mapped_files[file]["start"] = min(mapped_files[file]["start"], start)
        mapped_files[file]["end"] = max(mapped_files[file]["end"], end)

# for k, v in mapped_files.items():
#     print(f'{v["start"]:#x} - {v["end"]:#x}: {k}')
#     # print(f'{start:#x} - {end:#x}: {name}')

for addr in backtrace:
    file, region_start = next((
        (file, region["start"])
        for file, region in mapped_files.items()
        if region["start"] <= addr and region["end"] >= addr
    ), (None, None))

    if file is None or region_start is None:
        print(f"{addr:#x} in ??")
        continue

    file_basename = os.path.basename(file)
    if file_basename == 'libgcc_s.so.1':
        file = 'Toolchain/Build/riscv64/gcc/gcc/libgcc_s.so.1'
    else:
        file = next(
            path
            for path in pathlib.Path(BUILD_DIR).rglob(file_basename)
            if os.path.isfile(path) and os.access(path, os.X_OK)
        )

    addr2line_output = (
        subprocess.run(
            ["addr2line", "-Cfie", file, hex(addr - region_start)], capture_output=True
        )
        .stdout.decode("utf-8")
        .splitlines()
    )

    print(f"{addr:#x} in binary {file_basename}+{addr - region_start:#x}:")

    if len(addr2line_output) == 0:
        print("  ??")
        continue

    print(f"  {addr2line_output[0]} ({addr2line_output[1]})")

    for fn, src_file in zip(addr2line_output[2::2], addr2line_output[3::2]):
        print(f"  inlined by {fn} ({src_file})")
