#!/usr/bin/env python3

from tqdm import tqdm
from typing import Optional

import bisect
import orjson  # the normal json module is too slow
import operator
import os

CLOCK_FREQUENCY = 1_000_000_000  # Hz

KERNEL_MAP_FILE_PATH = 'Build/aarch64clang/Kernel/kernel.map'

symbol_map = []
lowest_symbol_address = 0
highest_symbol_address = 0

with open(KERNEL_MAP_FILE_PATH) as kernel_map_file:
    _ = kernel_map_file.readline()
    for line in kernel_map_file:
        start_address, _, symbol = line.split(maxsplit=2)
        start_address = int(start_address, 16)
        symbol = symbol.strip()

        symbol_map.append((start_address, symbol))

    lowest_symbol_address = symbol_map[0][0]
    highest_symbol_address = symbol_map[-1][0]


def symbol_for_address(address: int) -> Optional[str]:
    if address < lowest_symbol_address or address > highest_symbol_address:
        return None

    symbol_index = bisect.bisect_right(symbol_map, address, key=operator.itemgetter(0)) - 1
    return symbol_map[symbol_index][1]


with open('trace.bin', 'rb') as trace_file, open('chrome-trace.json', 'wb') as chrome_trace_file:
    trace_file_size = trace_file.seek(0, os.SEEK_END)
    trace_file.seek(0)

    t = tqdm(total=(trace_file_size // (3 * 4)), mininterval=0.1)

    events = []

    # for i in list(range(50)) + [1000]:
    #     events.append({
    #         'name': 'process_name',
    #         'ph': 'M',
    #         'pid': i,
    #         'args': {
    #             'name': f'P{i}',
    #         },
    #     })
    #
    #     events.append({
    #         'name': 'thread_name',
    #         'ph': 'M',
    #         'pid': i,
    #         'tid': i,
    #         'args': {
    #             'name': f'T{i}',
    #         },
    #     })

    while True:
        function_address_bytes = trace_file.read(4)
        if len(function_address_bytes) < 4:
            break

        function_address = int.from_bytes(function_address_bytes, 'little')

        is_function_entry = (function_address & (1 << 31)) != 0
        function_address &= ~(1 << 31)

        timestamp_bytes = trace_file.read(8)
        if len(timestamp_bytes) < 8:
            break

        timestamp = int.from_bytes(timestamp_bytes, 'little')

        tid = timestamp >> 56
        timestamp &= ((1 << 56) - 1)

        event = {
            'name': symbol_for_address(function_address) or f'<{function_address:#x}>',
            'ph': 'B' if is_function_entry else 'E',
            'pid': tid,
            'tid': tid,
            'ts': timestamp * 1_000_000 / CLOCK_FREQUENCY,
        }
        events.append(event)

        t.update()

    t.close()

    print('Writing JSON...')
    chrome_trace_file.write(orjson.dumps({'traceEvents': events}))
    print('Done')
