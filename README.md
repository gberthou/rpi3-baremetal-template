# rpi3-baremetal-template

This is a simple template for baremetal C programming for Raspberry Pi 3.
It is also compatibl with Raspberry Pi 1 to some extent.

## 1. Initialization
First, you have to figure out what you want to do with this template.
More specifically, do you need to write code for VideoCore IV's CPU?
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

Otherwise, you can still manuall update the submodules:
```console
git submodule update --init --recursive
cd vc4-toolchain && ./build-all.sh 2>&1 # cf. vc4-toolchain/README.md
```

### 1.2. Basic Initialization
Simply clone this repo without caring about submodules.

### 2. Build
All the `make`-related commands assume Raspberry Pi 3 by default (`RPI=3`).
If you want to build for Raspberry Pi 1, specify `RPI=1` with every `make`-related command.

For the first build, you'll have to prepare the directories with this command:
```console
make build
```

Then, simply run:
```console
make
```

## 3. Directories

 - `app-common`: app code, abstracted from the target platform.
 - `rpi1`: specific low-level code and constants for Raspberry Pi 1.
 - `rpi1/app`: specific app code for Raspberry Pi 1.
 - `rpi3`: specific low-level code and constants for Raspberry Pi 3.
 - `rpi3/app`: specific app code for Raspberry Pi 3.
 - `core`: interrupt vector and handler definitions.
 - `drivers`: provides a handful of simple yet uncomplete drivers.

## 4. Multi-core (RPI 3 only)

Each core can run its own program.
The source code has to provide `main0`, `main1`, `main2`, `main3` functions as shown in `rpi3/app/main.c`.
They will run respectively on cores 0, 1, 2 and 3.

