# libkitchen16 Linux build.
#
#   make          build one static library per memory model (DESIGN.md,
#                 decision 7): build/s/LIBK16.LIB (small), build/c/LIBK16C.LIB
#                 (compact) and build/l/LIBK16L.LIB (large), and stage them,
#                 with the headers, in build/stage (the release layout)
#   make test     build the test programs against each library and run the
#                 checks in tests/ (ABI, link maps, 8086 instructions,
#                 selective linking) and the programs under DOSBox-X
#   make audit    compare each library's code with Open Watcom's runtime
#                 libraries for the same model and print only scores
#                 (decision 6)
#   make dist     build/dist/libkitchen16-VERSION.zip and its .sha256, from
#                 the staged tree and the audit, and check the package; and
#                 make src
#   make src      build/dist/libkitchen16-VERSION-src.tar.gz and its .sha256,
#                 the committed tree of HEAD (not the working tree), and
#                 check it
#   make install PREFIX=DIR
#                 copy the staged tree to DIR
#   make clean    remove build/s, build/c, build/l, build/stage and
#                 build/dist (fetched tools in build/tools are kept)
#
# MODEL=s, c or l limits lib, test and audit to one model, for example
# `make MODEL=c test`.
#
# libkitchen16 builds and tests against libc16 (decision 2): scripts/fetch-libc16
# downloads the release it pins into build/libc16. LIBC16_ROOT names another
# libc16 tree instead, an unpacked release or a checkout built with make, for
# example `make LIBC16_ROOT=../libc16`.
#
# Needs Perl 5.40 or later as `perl` on PATH, a C compiler and make (to build
# JWasm), git, curl, unzip and xz, and dosbox-x for the tests. JWasm and Open
# Watcom are fetched into build/tools.

VERSION := $(shell cat VERSION)

BUILD  := build
TOOLS  := $(BUILD)/tools.env
MODELS := s c l

# The library of each model, its name in a release's lib/, and the macro
# that selects the model in the assembly members (src/model.inc), the same
# one wcc predefines. LC16 names libc16's library of the model.
LIBNAME_s := LIBK16.LIB
LIBNAME_c := LIBK16C.LIB
LIBNAME_l := LIBK16L.LIB
LC16NAME_s := LIBC16.LIB
LC16NAME_c := LIBC16C.LIB
LC16NAME_l := LIBC16L.LIB
ASMDEF_s  := __SMALL__
ASMDEF_c  := __COMPACT__
ASMDEF_l  := __LARGE__
LIBS      := $(foreach m,$(MODELS),$(BUILD)/$(m)/$(LIBNAME_$(m)))

# libc16: the tree the build uses. A checkout's build/stage has the release
# layout, so it is preferred when there is one (libc16's docs/CONSUMERS.md,
# "Using libc16").
LIBC16_ROOT ?= $(BUILD)/libc16
LIBC16_DIR  := $(if $(wildcard $(LIBC16_ROOT)/build/stage),$(LIBC16_ROOT)/build/stage,$(LIBC16_ROOT))
LIBC16_DEP  := $(if $(filter $(BUILD)/libc16,$(LIBC16_ROOT)),$(BUILD)/libc16/.zip-sha256)

# Library members are compiled for the 8086, with no stack checks, with
# software floating point (-fpc: docs/ABI.md), and see no headers or
# libraries of Open Watcom's: -x ignores INCLUDE and WATCOM, and -zl leaves
# out default-library records. Members see libkitchen16's headers and
# libc16's, so that public functions are defined __cdecl as they are
# declared. Each model adds -mMODEL (wcc) or -D its macro (JWasm).
WCFLAGS_ALL := -q -zq -zl -zld -x -0 -os -s -fpc -bt=dos -wx -we -Iinclude -I$(LIBC16_DIR)/include
JWFLAGS_ALL := -q -Isrc

ifndef MODEL

all: lib stage

lib test audit:
	set -e; for m in $(MODELS); do $(MAKE) --no-print-directory MODEL=$$m $@; done

$(LIBS): FORCE
	$(MAKE) --no-print-directory MODEL=$(word 2,$(subst /, ,$@)) lib

else

ifeq ($(filter $(MODEL),$(MODELS)),)
$(error MODEL must be one of $(MODELS))
endif

MB  := $(BUILD)/$(MODEL)
OBJ := $(MB)/obj
LIB := $(MB)/$(LIBNAME_$(MODEL))
LC16 := $(LIBC16_DIR)/lib/$(LC16NAME_$(MODEL))

WCFLAGS := $(WCFLAGS_ALL) -m$(MODEL)
JWFLAGS := $(JWFLAGS_ALL) -D$(ASMDEF_$(MODEL))

SOURCES := $(wildcard src/*.asm src/*.c)
MEMBERS := $(patsubst %,$(OBJ)/%.obj,$(basename $(notdir $(SOURCES))))

lib: $(LIB)

# libc16's library: fetched if it is the default tree, otherwise it must exist.
$(LC16): $(LIBC16_DEP)

$(LIB): $(MEMBERS)
	rm -f $@
	. ./$(TOOLS) && PATH=$$WATCOM/binl64:$$PATH && wlib -q -n -b $@ $(addprefix +,$^)

$(OBJ)/%.obj: src/%.asm src/model.inc $(TOOLS)
	mkdir -p $(@D)
	. ./$(TOOLS) && "$$JWASM" $(JWFLAGS) -Fo$@ $<

$(OBJ)/%.obj: src/%.c $(wildcard src/*.h include/*.h) $(TOOLS) $(LIBC16_DEP)
	mkdir -p $(@D)
	. ./$(TOOLS) && PATH=$$WATCOM/binl64:$$PATH && wcc $(WCFLAGS) -fo=$@ $<

# Test programs are built as a consumer builds (decision 2): the default
# register convention, the library's model, -fpc, libkitchen16's and libc16's
# headers only, and no Open Watcom library. Each directory tests/SUITE with a
# cases file is a suite for tests/run-dos; every tests/SUITE/NAME.c is a
# program, linked with tests/support/testio.c. A suite's rules.mk, if it has
# one, adds what is particular to it (more objects for a program, more
# programs, flags).
TEST      := $(MB)/tests
TCFLAGS   := $(WCFLAGS) -Itests/support
HEADERS   := $(wildcard include/*.h) tests/support/testio.h tests/support/check.h
SUITES    := $(patsubst tests/%/cases,%,$(wildcard tests/*/cases))
SUITE_SRCS := $(wildcard $(patsubst %,tests/%/*.c,$(SUITES)))
SUITE_EXES := $(patsubst tests/%.c,$(TEST)/%.EXE,$(SUITE_SRCS))
EXTRA_EXES :=
EXTRA_OBJS :=

$(TEST)/%.obj: tests/%.c $(HEADERS) $(TOOLS) $(LIBC16_DEP)
	mkdir -p $(@D)
	. ./$(TOOLS) && PATH=$$WATCOM/binl64:$$PATH && wcc $(TCFLAGS) -fo=$@ $<

$(TEST)/%.obj: tests/%.asm src/model.inc $(TOOLS)
	mkdir -p $(@D)
	. ./$(TOOLS) && "$$JWASM" $(JWFLAGS) -Fo$@ $<

-include $(wildcard tests/*/rules.mk)

# Selective linking (DESIGN.md, section 5): each program is linked again in
# TEST/plus against a copy of the library with an extra member that nothing
# references, tests/plus/unused.c, and must come out identical.
PLUS      := $(TEST)/plus
PLUS_LIB  := $(PLUS)/$(LIBNAME_$(MODEL))
PLUS_EXES := $(patsubst $(TEST)/%.EXE,$(PLUS)/%.EXE,$(SUITE_EXES) $(EXTRA_EXES))

$(PLUS)/%.obj: tests/plus/%.c $(TOOLS) $(LIBC16_DEP)
	mkdir -p $(@D)
	. ./$(TOOLS) && PATH=$$WATCOM/binl64:$$PATH && wcc $(WCFLAGS) -fo=$@ $<

$(PLUS_LIB): $(LIB) $(PLUS)/unused.obj
	cp $(LIB) $@
	. ./$(TOOLS) && PATH=$$WATCOM/binl64:$$PATH && wlib -q -b $@ +$(PLUS)/unused.obj

# Every program's objects: its own, the test support, and a suite's extras.
# No default libraries: the map must show only the objects, libkitchen16 and
# libc16 (scripts/check-map), and X.MEMBERS lists the library members linked.
OBJECTS_OF = $(filter %.obj,$^)
$(SUITE_EXES) $(EXTRA_EXES): $(TEST)/%.EXE: $(TEST)/%.obj $(TEST)/support/testio.obj scripts/check-map $(LIB) $(LC16)
	mkdir -p $(@D)
	. ./$(TOOLS) && PATH=$$WATCOM/binl64:$$PATH && wlink option quiet, nodefaultlibs, \
	  map=$(@:.EXE=.MAP) $(LDOPTS) format dos name $@ $(foreach o,$(OBJECTS_OF),file $(o)) \
	  library $(LIB) library $(LC16)
	scripts/check-map $(@:.EXE=.MAP) $(LIB) $(LC16) $(OBJECTS_OF) > $(@:.EXE=.MEMBERS)

$(PLUS_EXES): $(PLUS)/%.EXE: $(TEST)/%.obj $(TEST)/support/testio.obj scripts/check-map $(PLUS_LIB) $(LC16)
	mkdir -p $(@D)
	. ./$(TOOLS) && PATH=$$WATCOM/binl64:$$PATH && wlink option quiet, nodefaultlibs, \
	  map=$(@:.EXE=.MAP) $(LDOPTS) format dos name $@ $(foreach o,$(OBJECTS_OF),file $(o)) \
	  library $(PLUS_LIB) library $(LC16)
	scripts/check-map $(@:.EXE=.MAP) $(PLUS_LIB) $(LC16) $(OBJECTS_OF) > $(@:.EXE=.MEMBERS)

# A suite's members file may have a variant for one model, MEMBERS.MODEL.
members = $(firstword $(wildcard tests/$(1)/members.$(MODEL)) tests/$(1)/members)

test: $(SUITE_EXES) $(EXTRA_EXES) $(PLUS_EXES)
	@echo '=== model $(MODEL)'
	. ./$(TOOLS) && scripts/check-8086 $(MEMBERS) $(SUITE_EXES:.EXE=.obj) $(EXTRA_OBJS) $(TEST)/support/testio.obj
	. ./$(TOOLS) && PATH=$$WATCOM/binl64:$$PATH && tests/check-abi $(MODEL) $(TEST)/abi $(TCFLAGS)
	tests/check-selective $(TEST) $(PLUS)
	set -e; for s in $(SUITES); do \
	  m=$$(ls tests/$$s/members.$(MODEL) tests/$$s/members 2>/dev/null | head -1); \
	  if [ -n "$$m" ]; then tests/check-members $(TEST)/$$s $$m; fi; \
	  tests/run-dos $(TEST)/$$s tests/$$s/cases; \
	done

# The similarity audit (decision 6) against the DOS runtime library, the
# math libraries (software floating point, the 8087 libraries and the
# emulator) of the same model in the pinned Open Watcom snapshot, and its
# loose DOS objects. Only scores are printed.
AUDIT := $(MB)/audit.txt

audit: $(LIB) scripts/similarity-audit $(TOOLS)
	. ./$(TOOLS) && scripts/similarity-audit $(LIB) $$WATCOM/lib286/dos/clib$(MODEL).lib \
	  $$WATCOM/lib286/math$(MODEL).lib $$WATCOM/lib286/math87$(MODEL).lib $$WATCOM/lib286/emu87.lib \
	  $$WATCOM/lib286/dos/*.obj > $(AUDIT).tmp \
	  || { cat $(AUDIT).tmp; exit 1; }
	mv $(AUDIT).tmp $(AUDIT)
	cat $(AUDIT)

endif

# libc16, fetched at the version scripts/fetch-libc16 pins.
$(BUILD)/libc16/.zip-sha256: scripts/fetch-libc16
	scripts/fetch-libc16 $(BUILD)/libc16

# Paths of the fetched tools, as shell assignments.
$(TOOLS): scripts/fetch-jwasm scripts/fetch-watcom $(wildcard patches/jwasm/*.patch)
	mkdir -p $(@D)
	jwasm=$$(scripts/fetch-jwasm) && watcom=$$(scripts/fetch-watcom) \
	  && printf 'JWASM=%s\nWATCOM=%s\nexport WATCOM\n' "$$jwasm" "$$watcom" > $@

# The release layout (README.md, "Using a release"), the same for a
# release ZIP and a checkout: consumers point LIBKITCHEN16_ROOT at either.
STAGE := $(BUILD)/stage
DOCS  := docs/ABI.md docs/CONSUMERS.md docs/DESIGN.md docs/LICENSE-PROVENANCE.md \
	$(wildcard docs/spec/*.md)
HDRS  := $(wildcard include/*.h)

stage: $(LIBS) $(HDRS) $(DOCS) VERSION LICENSE README.md scripts/fetch-watcom \
		scripts/build-info scripts/fetch-jwasm scripts/fetch-libc16 Makefile
	rm -rf $(STAGE).tmp
	mkdir -p $(STAGE).tmp/lib $(STAGE).tmp/tools
	cp VERSION LICENSE README.md $(STAGE).tmp/
	cp $(LIBS) $(STAGE).tmp/lib/
	cp -R include $(STAGE).tmp/
	mkdir -p $(STAGE).tmp/docs/spec
	for f in $(DOCS); do cp $$f $(STAGE).tmp/$$f; done
	cp scripts/fetch-watcom $(STAGE).tmp/tools/
	scripts/build-info '$(WCFLAGS_ALL)' '$(JWFLAGS_ALL)' > $(STAGE).tmp/BUILD.txt
	rm -rf $(STAGE)
	mv $(STAGE).tmp $(STAGE)

install: stage
	@test -n "$(PREFIX)" || { echo 'usage: make install PREFIX=DIR' >&2; exit 1; }
	mkdir -p $(PREFIX)
	cp -R $(STAGE)/. $(PREFIX)/

DIST := $(BUILD)/dist
ZIP  := $(DIST)/libkitchen16-$(VERSION).zip

SRC  := $(DIST)/libkitchen16-$(VERSION)-src.tar.gz

dist: stage audit scripts/make-dist scripts/check-dist src
	mkdir -p $(DIST)
	scripts/make-dist $(STAGE) $(foreach m,$(MODELS),$(BUILD)/$(m)/audit.txt) libkitchen16-$(VERSION) $(ZIP)
	cd $(DIST) && sha256sum libkitchen16-$(VERSION).zip > libkitchen16-$(VERSION).zip.sha256
	scripts/check-dist $(ZIP) $(VERSION)

# The corresponding source (README.md, "Source archive"): git archive of
# HEAD, so uncommitted changes are not in it.
src: scripts/make-src scripts/check-src
	mkdir -p $(DIST)
	scripts/make-src HEAD $(VERSION) $(SRC)
	scripts/check-src $(SRC) $(VERSION) HEAD

clean:
	rm -rf $(foreach m,$(MODELS),$(BUILD)/$(m)) $(STAGE) $(DIST)

FORCE:

.PHONY: all lib test audit stage install dist src clean FORCE
.DELETE_ON_ERROR:
