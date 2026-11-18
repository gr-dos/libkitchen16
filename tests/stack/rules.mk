# tests/stack: each program runs with the 6,144-byte stack it paints, in
# the program and in its selective-linking copy.
STACK_EXES := stk_math stk_fpc stk_conv stk_io
$(foreach e,$(STACK_EXES),$(eval $(TEST)/stack/$(e).EXE $(TEST)/plus/stack/$(e).EXE: LDOPTS = option stack=6144))
