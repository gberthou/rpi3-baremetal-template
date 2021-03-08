BIN=test
BIN_C_DIR=../../../app/

# Update with your vc4-toolchain installation directory
# https://github.com/itszor/vc4-toolchain
# (don't forget to compile the compiler using the instructions from vc4-toolchain README)
TOOLCHAIN=/home/gautier/github/vc4-toolchain/prefix/bin/vc4-elf-
LDSCRIPT=vc4-sim.ld

CFLAGS=-Wall -Wextra -pedantic -Werror -O3
LFLAGS=-nostartfiles
CFILES=main.c
AFILES=launcher.s
OFILES=$(patsubst %.s,%.o,$(AFILES)) $(patsubst %.c,%.o,$(CFILES))

%.o: %.c
	$(TOOLCHAIN)gcc $(CFLAGS) -o $@ -c $<

%.o: %.s
	$(TOOLCHAIN)as -o $@ -c $<

$(BIN).bin: $(BIN)
	$(TOOLCHAIN)objcopy -O binary $< $@

$(BIN_C_DIR)$(BIN).bin.c: $(BIN).bin
	python3 toc.py $< > $@

$(BIN): $(OFILES)
	$(TOOLCHAIN)gcc -o $@ $^ $(LFLAGS) -T $(LDSCRIPT)

disas: $(BIN)
	$(TOOLCHAIN)objdump -xD $< > $@

clean:
	rm -f $(BIN) $(OFILES) disas $(BIN).bin $(BIN_C_DIR)$(BIN).bin.c

