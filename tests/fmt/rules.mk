# tests/fmt: the stack test runs with the 2,048-byte stack the spec names.
$(TEST)/fmt/stack.EXE: LDOPTS = option stack=2048
