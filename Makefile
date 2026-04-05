SRCDIR = src
OBJDIR = obj
INCLUDEDIR = inc
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)%.c=$(OBJDIR)%.o)
INCLUDES = $(SOURCES:$(SRCDIR)%.c=$(INCLUDEDIR)%.h)
UNIDEPS =
XFLAGS =
CFLAGS = -I$(INCLUDEDIR) -fopenmp $(XFLAGS) -m64
DEBUGFLAGS = -ggdb -DDEBUG -Wall -Wextra
RELEASEFLAGS = -O3
CC = gcc
TARGET = lefoux
PERFDATA = perfdata.csv
PERFARGS = --magic

.PHONY: all
all: $(TARGET)

# Using TOTALTHREADS is better when there are many memory lookups such as
# when evaluating positions with mostly bishops, queens, and rooks while
# CPUCORES is better when there is mostly computation and few memory lookups
# such positions with many knights and pawns. In general, CPUCORES is probably
# a better default but is subject to tweaking
.PHONY: autothreads
autothreads: clean
	CPUCORES=$$( cat /proc/cpuinfo | gawk 'match($$0, /cpu cores\s: (.*$$)/, groups) {print groups[1]}' | tail -1); \
	TOTALTHREADS=$$( cat /proc/cpuinfo | gawk 'match($$0, /processor\s:(.*$$)/, groups) {print groups[1]}' | tail -1); TOTALTHREADS=$$(echo "$$TOTALTHREADS + 1" | bc);\
	$(MAKE) XFLAGS="-DNUM_THREADS=$$CPUCORES" release;

.PHONY: single
single: clean
	$(MAKE) XFLAGS="-DNUM_THREADS=1";

.PHONY: check
check: debug
	./$(TARGET) --test

.PHONY: debug
debug: CFLAGS += $(DEBUGFLAGS)
debug: clean all

.PHONY: release
release: CFLAGS += $(RELEASEFLAGS)
release: clean all

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) -o $(TARGET)

# Override for files to ignore specific warnings
$(OBJDIR)/tests.o: $(SRCDIR)/tests.c $(INCLUDEDIR)/tests.h $(UNIDEPS)
	@mkdir -p $(OBJDIR)
	$(CC) -c $< $(CFLAGS) -o $@ -Wno-int-to-pointer-cast -Wno-unused-parameter

$(OBJDIR)/uci.o: $(SRCDIR)/uci.c $(INCLUDEDIR)/uci.h $(UNIDEPS)
	@mkdir -p $(OBJDIR)
	$(CC) -c $< $(CFLAGS) -o $@ -Wno-unused-parameter

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCLUDEDIR)/%.h $(UNIDEPS)
	@mkdir -p $(OBJDIR)
	$(CC) -c $< $(CFLAGS) -o $@

$(INCLUDES):

$(UNIDEPS):
	@touch $@

.PHONY: clean
clean:
	rm -rf $(OBJDIR)/*.o $(TARGET)
