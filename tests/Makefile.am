# HIME Unit Tests Makefile

CFLAGS += -std=gnu17 -Wall $(GTKINC) -I../src

# Test programs
PROGS = test-utf8 test-util

# Build all test programs
all: $(PROGS)

# test-utf8: UTF-8 function tests
test-utf8: test-utf8.c test-framework.h ../src/locale.o ../src/util.o
	$(CC) $(CFLAGS) -o $@ test-utf8.c ../src/locale.o ../src/util.o $(GTKLDFLAGS)

# test-util: Utility function tests
test-util: test-util.c test-framework.h ../src/util.o ../src/locale.o
	$(CC) $(CFLAGS) -o $@ test-util.c ../src/util.o ../src/locale.o $(GTKLDFLAGS)

# Run all tests
.PHONY: run-tests check test
run-tests check test: $(PROGS)
	@echo ""
	@failed=0; passed=0; \
	for t in $(PROGS); do \
		echo "Running $$t..."; \
		if ./$$t; then \
			passed=$$((passed + 1)); \
		else \
			failed=$$((failed + 1)); \
		fi; \
		echo ""; \
	done; \
	echo "========================================"; \
	echo "Test Suites: $$passed passed, $$failed failed"; \
	echo "========================================"; \
	if [ $$failed -gt 0 ]; then exit 1; fi

# Clean
.PHONY: clean
clean:
	rm -f $(PROGS) *.o

# Install/uninstall (no-op for tests)
.PHONY: install uninstall
install:
uninstall:
