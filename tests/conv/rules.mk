# What is particular to tests/conv (the Makefile includes this file).
#
# The programs that read a data file link the helper objects of tests/conv/lib;
# roundtrp only needs the hex printer, purity and longstr nothing.

CONV_DIR  := $(TEST)/conv
CONV_DATA := $(foreach p,atof conv errno,$(CONV_DIR)/$(p).EXE $(TEST)/plus/conv/$(p).EXE)
CONV_HEX  := $(foreach p,roundtrp,$(CONV_DIR)/$(p).EXE $(TEST)/plus/conv/$(p).EXE)

$(CONV_DATA): $(CONV_DIR)/lib/data.obj $(CONV_DIR)/lib/hex.obj
$(CONV_HEX): $(CONV_DIR)/lib/hex.obj

EXTRA_OBJS += $(CONV_DIR)/lib/data.obj $(CONV_DIR)/lib/hex.obj

# longstr runs with a stack of 1,024 bytes: the spec allows strtod 700 bytes of
# stack, and the program needs a few dozen more for itself and for the
# interrupts that run on its stack. (The option goes into wlink's `option`
# list.)
$(CONV_DIR)/longstr.EXE $(TEST)/plus/conv/longstr.EXE: LDOPTS += , stack=1024
