# Possible values: 1, 3, 4
RPI ?= 3
AA64 ?= 0
ifeq ($(RPI),1)
  ARM_CARCH=-march=armv6z
  ARM_ASARCH=-march=armv6z
  ARM_BIN=kernel
else ifeq ($(RPI),3)
  ARM_CARCH=-mcpu=cortex-a53+nofp
  ARM_ASARCH=-mcpu=cortex-a53
  ARM_BIN=kernel7
else ifeq ($(RPI),4)
  ARM_CARCH=-mcpu=cortex-a72
  ARM_ASARCH=-mcpu=cortex-a72
  ARM_BIN=kernel8
  AA64=1
else
  $(error Invalid RPI value. Try either of the following: 1, 3, 4)
endif

ifeq ($(AA64),1)
  ARM=aarch64-none-elf-
  ARM_CFLAGS=
  INCLUDES=-Iaarch64/include
  START_ADDRESS=0x80000
else
  ARM=arm-none-eabi-
  ARM_CFLAGS=-mthumb-interwork
  INCLUDES=-Iaarch32/include
  START_ADDRESS=0x8000
endif
VC4=vc4-toolchain/prefix/bin/vc4-elf-

OBJDIR=obj-$(RPI)
DISASDIR=$(OBJDIR)/disas-$(RPI)
RPIDIR=rpi$(RPI)
RPIAPPDIR=$(RPIDIR)/app

INCLUDES+=-I. -Iinclude -I$(RPIDIR)

ARM_CFLAGS+=-std=c2x -g -Wall -Wextra -Werror -pedantic -fomit-frame-pointer -nostartfiles -ffreestanding -mgeneral-regs-only $(ARM_CARCH) -O3
ARM_ASFLAGS=$(ARM_ASARCH)
ARM_LDFLAGS=-nostartfiles
ARM_DEFINES=-DRPI=$(RPI)
ARM_CFILES=$(wildcard core/*.c) $(wildcard drivers/*.c) $(wildcard drivers/bcm2835/*.c) $(wildcard drivers/adc/*.c) $(wildcard drivers/virtual/*.c) $(wildcard app-common/*.c) $(wildcard $(RPIDIR)/*.c) $(wildcard $(RPIAPPDIR)/*.c)
ARM_ASFILES=$(wildcard core/*.s) $(wildcard $(RPIDIR)/*.s) resource/console.s
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
	$(ARM)objcopy $(ARM_BIN).elf -O binary $(ARM_BIN).img
	$(ARM)objdump -xds $(ARM_BIN).elf > $(DISASDIR)/$(ARM_BIN)

$(OBJDIR)/vc4-kernel.c : $(VC4_BIN)
	python3 tools/raw2c.py $< > $@

$(VC4_BIN): $(VC4_LDSCRIPT) $(VC4_OBJS)
	$(VC4)gcc $(VC4_LDFLAGS) $(VC4_OBJS) -o $@.bin -T $(VC4_LDSCRIPT)
	$(VC4)objcopy -O binary $@.bin $@

build:
	mkdir -p $(OBJDIR) $(OBJDIR)/aarch64 $(OBJDIR)/core $(OBJDIR)/app-common $(OBJDIR)/drivers $(OBJDIR)/drivers/adc $(OBJDIR)/drivers/bcm2835 $(OBJDIR)/drivers/virtual $(OBJDIR)/resource $(DISASDIR) $(OBJDIR)/$(RPIDIR) $(OBJDIR)/$(RPIAPPDIR) $(OBJDIR)/vc4

qemu:
ifeq ($(RPI),1)
	qemu-system-arm -s -S -serial mon:stdio -M raspi1ap -smp 1 -m 512M -cpu arm1176 -bios kernel.img -device loader,addr=$(START_ADDRESS),cpu-num=0
else ifeq ($(RPI),3)
	qemu-system-aarch64 -s -S -serial mon:stdio -M raspi2b -smp 4 -m 1G -cpu cortex-a53 -bios kernel7.img -device loader,addr=$(START_ADDRESS),cpu-num=0
else ifeq ($(RPI),4)
	qemu-system-aarch64 -s -S -serial mon:stdio -M raspi4b -smp 4 -m 2G -cpu cortex-a72 -kernel kernel8.img
endif

clean:
	rm -f $(ARM_OBJS) $(ARM_BIN) $(VC4_BIN) $(VC4_KERNEL_TO_IMPORT)
