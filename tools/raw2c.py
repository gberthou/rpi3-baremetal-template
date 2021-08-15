import sys
import os.path

def is_nice(c):
    o = ord(c)
    return (o >= ord('a') and o <= ord('z')) or (o >= ord('A') and o <= ord('Z') ) or (o >= ord('0') and o <= ord('9'))

def nicify(s):
    return "".join(i if is_nice(i) else '_' for i in s)

if __name__ == "__main__":
    with open(sys.argv[1], "rb") as f:
        varname = nicify(os.path.split(sys.argv[1])[-1]) + "_contents"
        b = f.read()

        print("#include <stdint.h>")
        print("uint8_t __attribute__((aligned(4))) %s[] = {" % varname)
        print("    " + ", ".join("0x%02x" % i for i in b))
        print("};")

