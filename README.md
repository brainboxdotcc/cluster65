# Cluster65

Cluster65 is an emulator for a fictional multi-core 6502 computer.

Each core has its own private 64K address space, there will be no shared RAM and cores will communicate through firmware rather than shared memory.

Instead of emulating one 6502 at a time, this executes multiple independent 6502 cores in parallel where possible. This is the unique part of this project, and it is intended for AVX2 x86_64 host machines only.

Right now, this is purely a windows visual studio project because my Linux machines don't support AVX2. Linux support may follow at a later date.

## Current

Right now this can:

- Execute four independent 6502 cores simultaneously using AVX2
- Support divergent execution between cores
- Run a small subset of the 6502 instruction set

It isn't intended to be useful yet, at the moment it's just proving this is possible.

## Planned

Eventually Cluster65 will become a complete fictional computer, including:

- A complete 6502 implementation
- Firmware for inter-core communication
- A monitor, assembler and debugger

This is still under construction, so expect breakage!
