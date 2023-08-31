# Serenity RISC-V Port

## Required extensions

The RISC-V (64-bit, little endian) port requires the following extensions:

- `G`, which is an abbreviation for the following extensions:
    - `I`: Base Integer Instruction Set
    - `M`: Standard Extension for Integer Multiplication and Division
    - `A`: Standard Extension for Atomic Instructions
    - `F`: Standard Extension for Single-Precision Floating-Point
    - `D`: Standard Extension for Double-Precision Floating-Point
    - `Zicsr`: Control and Status Register (CSR) Instructions
    - `Zifencei`: Instruction-Fetch Fence
- `C`: Standard Extension for Compressed Instructions
- `Ss`: Supervisor Architecture
- `Sv39`: Page-based Virtual Memory Extension, 39-bit

RISC-V Profiles extensions:
- `Sstvala`: `stval` provides all needed values
- `Sstvecd`: `stvec` supports Direct mode
- `Ssu64xl`: UXLEN=64 must be supported
- `Svbare`: Bare mode virtual-memory translation supported
- `Zic64b`: Cache block size is 64 bytes
