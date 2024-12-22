# rpi3-baremetal-template

This is a simple template for baremetal C programming for Raspberry Pi 1, 3 and 4.

## 1. Initialization
First, you have to figure out what you want to do with this template.
More specifically, do you need to write code for VideoCore IV's CPU (RPI 1, 3)?
If so, then you'll need the complete initialization.
Otherwise, the basic is better suited.

### 1.1. Complete Initialization
This step takes a rather long time, so it's encouraged to check twice whether you need all submodules.
The worst-case scenario would be that it takes time and space on your computer for unused features, that's it.

If you haven't cloned the repo yet, the best is to download all submodules recursively:
```console
git clone --recursive https://github.com/gberthou/rpi3-baremetal-template
cd vc4-toolchain && ./build-all.sh 2>&1 # cf. vc4-toolchain/README.md
```

Otherwise, you can still manually update the submodules:
```console
git submodule update --init --recursive
cd vc4-toolchain && ./build-all.sh 2>&1 # cf. vc4-toolchain/README.md
```

### 1.2. Basic Initialization
Simply clone this repo without caring about submodules.

## 2. Build
All the `make`-related commands assume Raspberry Pi 3 by default (`RPI=3`).
If you want to build for Raspberry Pi 1 or 4, respectively specify `RPI=1` or `RPI=4` with every `make`-related command.
This even applies to `make build RPI=...` and `make clean RPI=...` as the build directories and generated files are not the same based on which Rasperry Pi you are building an image for.

For the first build, you'll have to prepare the directories with this command:
```console
make build RPI=...
```

Then, simply run:
```console
make RPI=...
```

Additionally, VC4 is absent by default.
In order to activate it, `VC4_SUPPORT` needs to be defined, for instance
`VC4_SUPPORT=1`.
To use VC4 (cf. 1.1. Complete Initialization), run the following:
```
make VC4_SUPPORT=1 RPI=...
```

## 3. Simulation on host (qemu)
Although simulation is not a build operation _per se_, there is a makefile target that selects the appropriate [qemu](https://en.wikipedia.org/wiki/QEMU) invocation based on the `RPI` variable.
It might be possible that the qemu binaries have another name on your filesystem; if so please consider editing the makefile accordingly.

To run qemu and the gdb server:
```
make qemu RPI=...
```

To connect to the gdb server from another terminal (AArch32):
```
arm-none-eabi-gdb <kernel|kernel7>.elf
# Inside the gdb prompt:
target remote :1234
```

To connect to the gdb server from another terminal (AArch64):
```
aarch64-none-elf-gdb kernel8.elf
# Inside the gdb prompt:
target remote :1234
```

## 4. Directories

 - `app-common`: app code, abstracted from the target platform.
 - `rpi<n>`: specific low-level code and constants for Raspberry Pi `<n>`.
 - `rpi<n>/app`: specific app code for Raspberry Pi `<n>`.
 - `aarch<32|64>`: architecture-specific helpers.
 - `core`: interrupt vector and handler definitions.
 - `drivers`: provides a handful of simple yet uncomplete drivers.
 - `include`: useful inline utils to replace stdlib functions with your own assumptions.

## 4. Multi-core (RPI 3 and 4)

Each core can run its own program.
The source code has to provide `main0`, `main1`, `main2`, `main3` functions as shown in `rpi3/app/main.c`.
They will run respectively on cores 0, 1, 2 and 3.

For [AArch64](https://en.wikipedia.org/wiki/AArch64) images, Raspberry Pi's
firmware runs at [EL3](https://developer.arm.com/documentation/102412/0103/Privilege-and-Exception-levels/Exception-levels) and configures [EL2](https://developer.arm.com/documentation/102412/0103/Privilege-and-Exception-levels/Exception-levels) amongst other things.
The code held in `start.s` is run by the firmware at EL2 and is meant to prepare EL1 and jump into `main*` at EL1.
The `main*` functions are thus run with "kernel/OS" privileges and can either continue as is or configure EL0 in order to have a clear Application vs. OS distinction.
Some [exception
handlers](https://developer.arm.com/documentation/102412/0103/Handling-exceptions/Taking-an-exception) at EL3 are currently not managed by Raspberry Pi's firmware as they point to [UDF](https://developer.arm.com/documentation/ddi0602/2022-03/Base-Instructions/UDF--Permanently-Undefined-)
instructions.
Adding a [stub](https://github.com/raspberrypi/tools/tree/master/armstubs) can help overriding the EL3 code.
For EL1 and EL2 exception vectors, running at EL2 gives sufficient privilege to
change [VBAR\_EL1](https://developer.arm.com/documentation/ddi0601/2024-12/AArch64-Registers/VBAR-EL1--Vector-Base-Address-Register--EL1-) and [VBAR\_EL2](https://developer.arm.com/documentation/ddi0601/2024-09/AArch64-Registers/VBAR-EL2--Vector-Base-Address-Register--EL2-) so you can have your own vector handlers hosted in the same code as the rest of your image, without the need for a stub.
This is currently done for EL1 and EL2 at respectively `vbar_el1_ini` and `vbar_el2_ini`: a very simple UART driver is used to print the EL at which the fault has been taken, the offset of the vector and the values of ESR\_ELx as well as FAR\_ELx.
