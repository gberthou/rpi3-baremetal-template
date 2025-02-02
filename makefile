# Possible values: 1, 3, 4, 5
RPI ?= 5
AA64 ?= 1

ifeq ($(RPI),1)
  # RPI1 does not support AArch64
  override AA64=0
  ARM_CARCH=-march=armv6z
  ARM_ASARCH=-march=armv6z
else ifeq ($(RPI),3)
  ARM_CARCH=-mcpu=cortex-a53+nofp
  ARM_ASARCH=-mcpu=cortex-a53
else ifeq ($(RPI),4)
  ARM_CARCH=-mcpu=cortex-a72
  ARM_ASARCH=-mcpu=cortex-a72
else ifeq ($(RPI),5)
  override AA64=1
  ARM_CARCH=-mcpu=cortex-a76
  ARM_ASARCH=-mcpu=cortex-a76
else
  $(error Invalid RPI value. Try either of the following: 1, 3, 4, 5)
endif

# Kernel names are documented in that table:
# https://www.raspberrypi.com/documentation/computers/linux_kernel.html
ifeq ($(AA64),1)
ifeq ($(RPI),5)
  ARM_BIN=kernel_2712
else
  ARM_BIN=kernel8
endif
  ARM=aarch64-none-elf-
  ARM_CFLAGS=
  ARM_ASFILES=$(wildcard aarch64/*.s)
  ARM_CFILES=$(wildcard aarch64/*.c)
  INCLUDES=-Iaarch64/include
  START_ADDRESS=0x80000
  OBJDIR=obj-$(RPI)-aa64
else
ifeq ($(RPI),3)
  ARM_BIN=kernel7
else ifeq ($(RPI),4)
  ARM_BIN=kernel7l
else # Default to RPI1
  ARM_BIN=kernel
endif
  ARM=arm-none-eabi-
  ARM_CFLAGS=-mthumb-interwork
  ARM_ASFILES=$(wildcard aarch32/*.s)
  ARM_CFILES=$(wildcard aarch32/*.c)
  INCLUDES=-Iaarch32/include
  START_ADDRESS=0x8000
  OBJDIR=obj-$(RPI)-aa32
endif
VC4=vc4-toolchain/prefix/bin/vc4-elf-

DISASDIR=$(OBJDIR)/disas-$(RPI)
RPIDIR=rpi$(RPI)
RPIAPPDIR=$(RPIDIR)/app

INCLUDES+=-I. -Iinclude -I$(RPIDIR)

ARM_CFLAGS+=-std=c2x -g -Wall -Wextra -Werror -pedantic -fomit-frame-pointer -nostartfiles -ffreestanding -mgeneral-regs-only $(ARM_CARCH) -O3
ARM_ASFLAGS=$(ARM_ASARCH) --defsym RPI=$(RPI)
ARM_LDFLAGS=-nostartfiles
ARM_DEFINES=-DRPI=$(RPI)
ARM_CFILES+=$(wildcard core/*.c) $(wildcard drivers/*.c) $(wildcard drivers/bcm2835/*.c) $(wildcard drivers/adc/*.c) $(wildcard drivers/virtual/*.c) $(wildcard app-common/*.c) $(wildcard $(RPIDIR)/*.c) $(wildcard $(RPIAPPDIR)/*.c)
ifeq ($(RPI), 5)
  ARM_CFILES:=$(filter-out drivers/bcm2835/gpio.c,$(ARM_CFILES))
  ARM_CFILES:=$(filter-out drivers/bcm2835/clock.c,$(ARM_CFILES))
  ARM_CFILES+=$(wildcard drivers/rp1/*.c)
endif

ARM_ASFILES+=$(wildcard core/*.s) $(wildcard $(RPIDIR)/*.s) resource/console.s
ARM_OBJS=$(patsubst %.s,$(OBJDIR)/%.o,$(ARM_ASFILES))
ARM_OBJS+=$(patsubst %.c,$(OBJDIR)/%.o,$(ARM_CFILES))

VC4_BIN=$(OBJDIR)/vc4/vc4.bin
VC4_CFLAGS=-Wall -Wextra -pedantic -Werror -O3
VC4_ASFLAGS=
VC4_LDFLAGS=-nostartfiles
VC4_DEFINES=
VC4_CFILES=$(wildcard vc4/*.c)
VC4_ASFILES=$(wildcard vc4/*.s)
VC4_OBJS=$(patsubst %.s,$(OBJDIR)/%.o,$(VC4_ASFILES))
VC4_OBJS+=$(patsubst %.c,$(OBJDIR)/%.o,$(VC4_CFILES))
VC4_LDSCRIPT=./vc4-toolchain/prefix/vc4-elf/lib/vc4-sim.ld
VC4_KERNEL_TO_IMPORT=$(OBJDIR)/vc4-kernel.o

ifdef VC4_SUPPORT
ARM_DEFINES+=-DVC4_SUPPORT
ARM_OBJS+=$(VC4_KERNEL_TO_IMPORT)
endif

$(OBJDIR)/vc4/%.o: vc4/%.s
	$(VC4)as $(VC4_ASLAGS) $< -o $@

$(OBJDIR)/vc4/%.o: vc4/%.c
	$(VC4)gcc $(VC4_CFLAGS) $(INCLUDES) -c $< -o $@ $(VC4_DEFINES)

$(OBJDIR)/%.o : %.s
	$(ARM)as $(ARM_ASFLAGS) $< -o $@

$(OBJDIR)/%.o : %.c
	$(ARM)gcc $(ARM_CFLAGS) $(INCLUDES) -c $< -o $@ $(ARM_DEFINES)
$(OBJDIR)/%.o : $(OBJDIR)/%.c
	$(ARM)gcc $(ARM_CFLAGS) $(INCLUDES) -c $< -o $@ $(ARM_DEFINES)

default: $(ARM_OBJS)
	$(ARM)gcc $(ARM_LDFLAGS) $(ARM_OBJS) -o $(ARM_BIN).elf -Wl,-Ttext,$(START_ADDRESS) -Wl,--section-start=.stack=0x800 -T ldscript.ld
	$(ARM)objdump -xds $(ARM_BIN).elf > $(DISASDIR)/$(ARM_BIN)
	$(ARM)objcopy $(ARM_BIN).elf -O binary $(ARM_BIN).img

ifeq ($(AA64),0)
ifneq ($(RPI),1)
	# qemu for RPI2 loads the kernel at 0x10000 whereas the real hardware loads
	# it at 0x8000. Here, a qemu-specific elf/img pair is generated to
	# accomodate to that.
	$(ARM)gcc $(ARM_LDFLAGS) $(ARM_OBJS) -o $(ARM_BIN)-qemu.elf -Wl,-Ttext,0x10000 -Wl,--section-start=.stack=0x800 -T ldscript.ld
	$(ARM)objcopy $(ARM_BIN)-qemu.elf -O binary $(ARM_BIN)-qemu.img
endif
endif

$(OBJDIR)/vc4-kernel.c : $(VC4_BIN)
	python3 tools/raw2c.py $< > $@

$(VC4_BIN): $(VC4_LDSCRIPT) $(VC4_OBJS)
	$(VC4)gcc $(VC4_LDFLAGS) $(VC4_OBJS) -o $@.bin -T $(VC4_LDSCRIPT)
	$(VC4)objcopy -O binary $@.bin $@

build:
	mkdir -p $(OBJDIR) $(OBJDIR)/aarch32 $(OBJDIR)/aarch64 $(OBJDIR)/core $(OBJDIR)/app-common $(OBJDIR)/drivers $(OBJDIR)/drivers/adc $(OBJDIR)/drivers/bcm2835 $(OBJDIR)/drivers/rp1 $(OBJDIR)/drivers/virtual $(OBJDIR)/resource $(DISASDIR) $(OBJDIR)/$(RPIDIR) $(OBJDIR)/$(RPIAPPDIR) $(OBJDIR)/vc4

qemu:
ifeq ($(RPI),1)
	qemu-system-arm -s -S -serial mon:stdio -M raspi1ap -smp 1 -m 512M -cpu arm1176 -bios $(ARM_BIN).img -device loader,addr=$(START_ADDRESS),cpu-num=0
else ifeq ($(RPI),3)
# qemu for RPI3 only supports AArch64 so RPI2 platform will be used for AArch32
# kernels.
ifeq ($(AA64),0)
	qemu-system-aarch64 -s -S -serial mon:stdio -M raspi2b -smp 4 -m 1G -cpu cortex-a53 -kernel $(ARM_BIN)-qemu.img
else
	qemu-system-aarch64 -s -S -serial mon:stdio -M raspi3b -smp 4 -m 1G -cpu cortex-a53 -kernel $(ARM_BIN).img
endif
else ifeq ($(RPI),4)
# qemu for RPI4 only supports AArch64 so RPI2 platform will be used for AArch32
# kernels.
ifeq ($(AA64),0)
	qemu-system-aarch64 -s -S -serial mon:stdio -M raspi2b -smp 4 -m 1G -cpu cortex-a72 -kernel $(ARM_BIN)-qemu.img
else
	qemu-system-aarch64 -s -S -serial mon:stdio -M raspi4b -smp 4 -m 2G -cpu cortex-a72 -kernel $(ARM_BIN).img
endif
else ifeq ($(RPI),5)
	echo "No support yet"
endif

clean:
	rm -f $(ARM_OBJS) $(ARM_BIN).elf $(ARM_BIN).img $(ARM_BIN)-qemu.elf $(ARM_BIN)-qemu.img $(VC4_BIN) $(VC4_KERNEL_TO_IMPORT)
