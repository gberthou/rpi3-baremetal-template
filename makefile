# Possible values: 1, 3
RPI ?= 3
ifeq ($(RPI),1)
  CARCH=-march=armv6z
  ASARCH=-march=armv6z
  BIN=kernel
else ifeq ($(RPI),3)
  CARCH=-mcpu=cortex-a7+nofp
  ASFLAGS=-mcpu=cortex-a7
  BIN=kernel7
else
  $(error Invalid RPI value. Try either of the following: 1, 3)
endif

ARM=arm-none-eabi

OBJDIR=obj-$(RPI)
DISASDIR=$(OBJDIR)/disas-$(RPI)
RPIDIR=rpi$(RPI)
RPIAPPDIR=$(RPIDIR)/app


CFLAGS=-g -Wall -Wextra -Werror -pedantic -fomit-frame-pointer -nostartfiles -ffreestanding $(CARCH) -mthumb-interwork -O3
INCLUDES=-I. -I$(RPIDIR)

ASFLAGS=$(ASARCH)

LDFLAGS=-nostartfiles

DEFINES=-DRPI=$(RPI)

CFILES=$(wildcard core/*.c) $(wildcard drivers/*.c) $(wildcard drivers/bcm2835/*.c) $(wildcard drivers/adc/*.c) $(wildcard drivers/virtual/*.c) $(wildcard app-common/*.c) $(wildcard $(RPIDIR)/*.c) $(wildcard $(RPIAPPDIR)/*.c)
ASFILES=$(wildcard core/*.s) $(wildcard $(RPIDIR)/*.s) resource/console.s

LDSCRIPT=ldscript.ld

OBJS=$(patsubst %.s,$(OBJDIR)/%.o,$(ASFILES))
OBJS+=$(patsubst %.c,$(OBJDIR)/%.o,$(CFILES))

$(OBJDIR)/%.o : %.s
	$(ARM)-as $(ASFLAGS) $< -o $@

$(OBJDIR)/%.o : %.c
	$(ARM)-gcc $(CFLAGS) $(INCLUDES) -c $< -o $@ $(DEFINES)

default: $(LDSCRIPT) $(OBJS)
	$(ARM)-gcc $(LDFLAGS) $(OBJS) -o $(BIN).elf $(LIBDIR) -T $(LDSCRIPT)
	$(ARM)-objcopy $(BIN).elf -O binary $(BIN).img
	$(ARM)-objdump -xd $(BIN).elf > $(DISASDIR)/$(BIN)

build:
	mkdir -p $(OBJDIR) $(OBJDIR)/core $(OBJDIR)/app-common $(OBJDIR)/drivers $(OBJDIR)/drivers/adc $(OBJDIR)/drivers/bcm2835 $(OBJDIR)/drivers/virtual $(OBJDIR)/resource $(DISASDIR) $(OBJDIR)/$(RPIDIR) $(OBJDIR)/$(RPIAPPDIR)

clean:
	rm -f $(OBJS)
