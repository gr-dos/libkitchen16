# Rules particular to tests/math (see the Makefile).

# The programs include lib/mathio.h, which the Makefile does not know.
MATH_PROGS := $(patsubst tests/math/%.c,$(TEST)/math/%.obj,$(wildcard tests/math/*.c))
$(MATH_PROGS): tests/math/lib/mathio.h

# Selective linking of single functions: tests/math/check-link looks at
# the members of the l_NAME programs (README.txt), before the programs run.
MATH_LINK := fabs sqrt sin pow

math-link: $(foreach f,$(MATH_LINK),$(TEST)/math/l_$(f).EXE)
	tests/math/check-link $(TEST)/math $(MATH_LINK)

test: math-link

.PHONY: math-link
