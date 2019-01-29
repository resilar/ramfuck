BUILDDIR ?= build

CFLAGS ?= -g
CFLAGS += -Wall -std=c89 -pedantic
LDLIBS += -lm

INCS += -I$(BUILDDIR)/include

OBJS := ramfuck.o ast.o cli.o config.o eval.o hits.o lex.o line.o opt.o parse.o ptrace.o search.o symbol.o target.o value.o
OBJS := $(OBJS:%.o=$(BUILDDIR)/obj/%.o)

all: $(BUILDDIR)/ramfuck

$(BUILDDIR)/include/defines.h: defines.h
	@mkdir -p $(dir $@)
	cp $< $@

$(BUILDDIR)/obj/%.o: src/%.c $(BUILDDIR)/include/defines.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(BUILDDIR)/ramfuck: $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	$(RM) -r $(BUILDDIR)

.PHONY: all clean
