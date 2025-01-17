# SPDX-FileCopyrightText: 2023 EfficiOS Inc.
#
# SPDX-License-Identifier: MIT

SUBDIRS = utils common unit benchmark regression

.PHONY: short_bench long_bench regtest check-loop

short_bench:
	cd benchmark && $(MAKE) $(AM_MAKEFLAGS) short_bench
long_bench:
	cd benchmark && $(MAKE) $(AM_MAKEFLAGS) long_bench
regtest:
	cd regression && $(MAKE) $(AM_MAKEFLAGS) regtest
	cd benchmark && $(MAKE) $(AM_MAKEFLAGS) regtest

check-loop:
	while [ 0 ]; do \
		$(MAKE) $(AM_MAKEFLAGS) check; \
		if [ "$$?" != "0" ]; then \
			break; \
		fi \
	done
