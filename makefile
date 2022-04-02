# Possible values: 1, 3
RPI ?= 3
ifeq ($(RPI),1)
  ARM_CARCH=-march=armv6z
  ARM_ASARCH=-march=armv6z
  ARM_BIN=kernel
else ifeq ($(RPI),3)
  ARM_CARCH=-mcpu=cortex-a7+nofp
  ARM_ASARCH=-mcpu=cortex-a7
  ARM_BIN=kernel7
else
  $(error Invalid RPI value. Try either of the following: 1, 3)
endif

ARM=arm-none-eabi-
VC4=vc4-toolchain/prefix/bin/vc4-elf-

OBJDIR=obj-$(RPI)
DISASDIR=$(OBJDIR)/disas-$(RPI)
RPIDIR=rpi$(RPI)
RPIAPPDIR=$(RPIDIR)/app

INCLUDES=-I. -I$(RPIDIR)

ARM_CFLAGS=-std=c2x -g -Wall -Wextra -Werror -pedantic -fomit-frame-pointer -nostartfiles -ffreestanding $(ARM_CARCH) -mthumb-interwork -O3
ARM_ASFLAGS=$(ARM_ASARCH)
ARM_LDFLAGS=-nostartfiles
ARM_DEFINES=-DRPI=$(RPI)
ARM_CFILES=$(wildcard core/*.c) $(wildcard drivers/*.c) $(wildcard drivers/bcm2835/*.c) $(wildcard drivers/adc/*.c) $(wildcard drivers/virtual/*.c) $(wildcard app-common/*.c) $(wildcard $(RPIDIR)/*.c) $(wildcard $(RPIAPPDIR)/*.c)
ARM_ASFILES=$(wildcard core/*.s) $(wildcard $(RPIDIR)/*.s) resource/console.s
ARM_OBJS=$(patsubst %.s,$(OBJDIR)/%.o,$(ARM_ASFILES))
ARM_OBJS+=$(patsubst %.c,$(OBJDIR)/%.o,$(ARM_CFILES))
ARM_LDSCRIPT=ldscript.ld

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

ARM_OBJS+=$(VC4_KERNEL_TO_IMPORT)

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

default: $(ARM_LDSCRIPT) $(ARM_OBJS)
	$(ARM)gcc $(ARM_LDFLAGS) $(ARM_OBJS) -o $(ARM_BIN).elf -T $(ARM_LDSCRIPT)
	$(ARM)objcopy $(ARM_BIN).elf -O binary $(ARM_BIN).img
	$(ARM)objdump -xd $(ARM_BIN).elf > $(DISASDIR)/$(ARM_BIN)

$(OBJDIR)/vc4-kernel.c : $(VC4_BIN)
	python3 tools/raw2c.py $< > $@

$(VC4_BIN): $(VC4_LDSCRIPT) $(VC4_OBJS)
	$(VC4)gcc $(VC4_LDFLAGS) $(VC4_OBJS) -o $@.bin -T $(VC4_LDSCRIPT)
	$(VC4)objcopy -O binary $@.bin $@

build:
	mkdir -p $(OBJDIR) $(OBJDIR)/core $(OBJDIR)/app-common $(OBJDIR)/drivers $(OBJDIR)/drivers/adc $(OBJDIR)/drivers/bcm2835 $(OBJDIR)/drivers/virtual $(OBJDIR)/resource $(DISASDIR) $(OBJDIR)/$(RPIDIR) $(OBJDIR)/$(RPIAPPDIR) $(OBJDIR)/vc4

clean:
	rm -f $(ARM_OBJS) $(ARM_BIN) $(VC4_BIN)
