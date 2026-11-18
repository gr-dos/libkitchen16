# tests/fpc: regs.c calls the helpers through lib/regscall.asm, which the
# Makefile assembles for the model (the pattern rule for tests/%.asm) and
# which is linked into regs only.

$(TEST)/fpc/regs.EXE: $(TEST)/fpc/lib/regscall.obj
$(TEST)/plus/fpc/regs.EXE: $(TEST)/fpc/lib/regscall.obj

EXTRA_OBJS += $(TEST)/fpc/lib/regscall.obj
