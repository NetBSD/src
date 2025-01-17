# SPDX-FileCopyrightText: 2023 EfficiOS Inc.
#
# SPDX-License-Identifier: MIT

ACLOCAL_AMFLAGS=-I m4

SUBDIRS = include src doc tests extras

dist_doc_DATA = \
	LICENSE.md \
	README.md

EXTRA_DIST = \
	CodingStyle.md \
	lgpl-relicensing.md \
	LICENSES/Autoconf-exception-2.0.txt \
	LICENSES/BSD-2-Clause.txt \
	LICENSES/CC0-1.0.txt \
	LICENSES/CC-BY-4.0.txt \
	LICENSES/FSFAP.txt \
	LICENSES/GPL-2.0-only.txt \
	LICENSES/GPL-2.0-or-later.txt \
	LICENSES/LGPL-2.1-only.txt \
	LICENSES/LGPL-2.1-or-later.txt \
	LICENSES/LicenseRef-Autoconf-exception-macro.txt \
	LICENSES/LicenseRef-Boehm-GC.txt \
	LICENSES/MIT.txt

.PHONY: short_bench long_bench regtest check-loop
short_bench:
	cd tests && $(MAKE) $(AM_MAKEFLAGS) short_bench
long_bench:
	cd tests && $(MAKE) $(AM_MAKEFLAGS) long_bench
regtest:
	cd tests && $(MAKE) $(AM_MAKEFLAGS) regtest
check-loop:
	cd tests && $(MAKE) $(AM_MAKEFLAGS) check-loop
