SRCDIR = src
OBJDIR = obj
INCLUDEDIR = inc
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)%.c=$(OBJDIR)%.o)
INCLUDES = $(SOURCES:$(SRCDIR)%.c=$(INCLUDEDIR)%.h)
UNIDEPS =
XFLAGS =
CFLAGS = -I$(INCLUDEDIR) -O2 -fopenmp $(XFLAGS)
CC = gcc
TARGET = lefoux
PERFDATA = perfdata.csv
PERFARGS = --magic

.PHONY: all
all: $(TARGET)

.PHONY: autothreads
autothreads:
	CPUCORES=$$( cat /proc/cpuinfo | gawk 'match($$0, /cpu cores\s: (.*$$)/, groups) {print groups[1]}' | tail -1); \
	$(MAKE) XFLAGS="-DNUM_THREADS=$$CPUCORES";

.PHONY: debug
debug: CFLAGS += -g -DDEBUG -Wall -Wextra # -Wno-unused-parameter
debug: clean all
debug: $(TARGET)

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
