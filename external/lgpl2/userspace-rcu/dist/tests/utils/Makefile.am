# SPDX-FileCopyrightText: 2023 EfficiOS Inc.
#
# SPDX-License-Identifier: MIT

AM_CPPFLAGS += -I$(top_srcdir)/src

noinst_LIBRARIES = libtap.a
libtap_a_SOURCES = tap.c tap.h

dist_check_SCRIPTS = \
	tap-driver.sh \
	tap.sh \
	utils.sh
