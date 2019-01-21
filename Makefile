CFLAGS ?= -g
CFLAGS += -Wall -std=c89 -pedantic
LDLIBS += -lm

BUILDDIR ?= ./build

OBJS := ramfuck.o ast.o cli.o eval.o hits.o lex.o line.o mem.o opt.o parse.o ptrace.o search.o symbol.o value.o
OBJS := $(OBJS:%.o=$(BUILDDIR)/obj/%.o)

all: $(BUILDDIR)/ramfuck

$(BUILDDIR)/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/ramfuck: $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	$(RM) -r $(BUILDDIR)

.PHONY: all clean
