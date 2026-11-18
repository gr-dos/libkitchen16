# tests/size: tests/size/size.c compiled once per profile, -DSZ_PROFILE,
# and linked like every test program (so the selective-linking and map
# checks of `make test` cover them too). `make size` reports what each adds
# over `base` (tests/size/report; docs/DESIGN.md, section 8, "Size").
SIZE_PROFILES := base add arith snprintf sprintf printf strtod sqrt sin exp log pow atan2 perl all
SIZE_EXES := $(patsubst %,$(TEST)/size/%.EXE,$(SIZE_PROFILES))
EXTRA_EXES += $(SIZE_EXES)
EXTRA_OBJS += $(SIZE_EXES:.EXE=.obj)

$(TEST)/size/%.obj: tests/size/size.c $(HEADERS) $(TOOLS) $(LIBC16_DEP)
	mkdir -p $(@D)
	. ./$(TOOLS) && PATH=$$WATCOM/binl64:$$PATH && wcc $(TCFLAGS) -DSZ_$* -fo=$@ $<
