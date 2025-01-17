# SPDX-FileCopyrightText: 2023 EfficiOS Inc.
#
# SPDX-License-Identifier: MIT

AM_CPPFLAGS += -I$(top_srcdir)/src -I$(top_srcdir)/tests/common

noinst_HEADERS = \
	api.h \
	compat-rand.h \
	thread-id.h

noinst_LTLIBRARIES = libdebug-yield.la

libdebug_yield_la_SOURCES = debug-yield.c debug-yield.h
