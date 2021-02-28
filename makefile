ARM=arm-none-eabi
BIN=kernel7

OBJDIR=obj
DISASDIR=disas

CFLAGS=-g -Wall -Wextra -Werror -pedantic -fomit-frame-pointer -fno-stack-limit -mno-apcs-frame -nostartfiles -ffreestanding -mcpu=cortex-a7+nofp -mthumb-interwork -O2
INCLUDES=-I.

ASFLAGS=-mcpu=cortex-a7

LDFLAGS=-nostartfiles

DEFINES=

CFILES=$(wildcard core/*.c) $(wildcard drivers/bcm2835/*.c) $(wildcard drivers/adc/*.c) $(wildcard app/*.c)
ASFILES=$(wildcard core/*.s)

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
	mkdir -p $(OBJDIR) $(OBJDIR)/core $(OBJDIR)/app $(OBJDIR)/drivers $(OBJDIR)/drivers/adc $(OBJDIR)/drivers/bcm2835 $(DISASDIR)

clean:
	rm -f $(OBJS)
