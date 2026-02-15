VERSION_FILE = ChangeLog

DIRS    = src data filter icons man menu po scripts

.PHONY: all
all:
	@for d in $(DIRS); do $(ECHO) -e "\x1b[1;33m** processing $$d\x1b[0m"; \
	   $(MAKE) -C $$d || exit 1; \
	done

.PHONY: install
install:
	@for d in $(DIRS); do $(ECHO) -e "\x1b[1;32m** installing $$d\x1b[0m"; \
	   $(MAKE) -C $$d install || exit 1; \
	done
	install -d "$(DOC_DIR)"; \
	install -m 644 $(VERSION_FILE) "$(DOC_DIR)"

.PHONY: uninstall
uninstall:
	@for d in $(DIRS); do $(ECHO) -e "\x1b[1;32m** uninstalling $$d\x1b[0m"; \
	   $(MAKE) -C $$d uninstall || exit 1; \
	done

.PHONY: clean
clean:
	@touch src/.depend
	@for d in $(DIRS); do $(ECHO) -e "\x1b[1;31m** cleanup $$d\x1b[0m"; \
	   $(MAKE) -C $$d clean; \
	done

.PHONY: distclean
distclean:
	@$(MAKE) clean
	@rm -f config.mak

.PHONY: clean-all
clean-all:
	@$(MAKE) distclean
	@$(ECHO) -e "\x1b[1;31m** cleanup windows\x1b[0m"
	@rm -rf windows/build
	@$(ECHO) -e "\x1b[1;31m** cleanup macos\x1b[0m"
	@rm -rf macos/build
	@$(ECHO) -e "\x1b[1;31m** cleanup android\x1b[0m"
	@if [ -x android/gradlew ]; then cd android && ./gradlew clean -q; fi
	@$(ECHO) -e "\x1b[1;31m** cleanup shared/tests\x1b[0m"
	@$(MAKE) -C shared/tests clean 2>/dev/null || true

.PHONY: clang-format
clang-format:
	./scripts/format_code.py

.PHONY: check test
check test:
	@$(ECHO) -e "\x1b[1;33m** running tests\x1b[0m"
	@$(MAKE) -C tests run-tests
