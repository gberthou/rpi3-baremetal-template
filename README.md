# rpi3-baremetal-template

This is a simple template for baremetal C programming for Raspberry Pi 3.

## Multi-core

Each core can run its own program.
The source code has to provide `main0`, `main1`, `main2`, `main3` functions as shown in `apps/main.c`.
They will run respectively on cores 0, 1, 2 and 3.

## Directories

 - `app`: contains the app code.
 - `core`: contains some assembly code, starting code and interrupt vector.
 - `drivers`: provides a handful of simple but uncomplete drivers.
