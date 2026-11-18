# tests/bench: the speed of each function (tests/bench/bench.c). Built and
# checked by `make test` like the other programs; run only by `make bench`
# (tests/bench/cases.bench, at a fixed speed; docs/DESIGN.md, section 8,
# "Speed").
BENCH_EXE := $(TEST)/bench/bench.EXE
EXTRA_EXES += $(BENCH_EXE)
EXTRA_OBJS += $(BENCH_EXE:.EXE=.obj)
