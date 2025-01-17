# SPDX-FileCopyrightText: 2023 EfficiOS Inc.
#
# SPDX-License-Identifier: MIT

AM_CPPFLAGS += -I$(top_srcdir)/src

#Add the -version-info directly here since we are only building
# library that use the version-info
AM_LDFLAGS=-version-info $(URCU_LIBRARY_VERSION) $(LT_NO_UNDEFINED)

dist_noinst_HEADERS = urcu-die.h urcu-wait.h compat-getcpu.h \
	urcu-utils.h compat-smp.h

COMPAT = compat_arch.c compat_futex.c

RCULFHASH = rculfhash.c rculfhash-mm-order.c rculfhash-mm-chunk.c \
		rculfhash-mm-mmap.c

lib_LTLIBRARIES = liburcu-common.la \
		liburcu.la liburcu-qsbr.la \
		liburcu-mb.la liburcu-bp.la \
		liburcu-memb.la liburcu-cds.la

#
# liburcu-common contains wait-free queues (needed by call_rcu) as well
# as futex fallbacks.
#
liburcu_common_la_SOURCES = wfqueue.c wfcqueue.c wfstack.c $(COMPAT)

liburcu_la_SOURCES = urcu.c urcu-pointer.c $(COMPAT)
liburcu_la_CFLAGS = -DRCU_MEMBARRIER $(AM_CFLAGS)
liburcu_la_LIBADD = liburcu-common.la

liburcu_memb_la_SOURCES = urcu.c urcu-pointer.c $(COMPAT)
liburcu_memb_la_CFLAGS = -DRCU_MEMBARRIER $(AM_CFLAGS)
liburcu_memb_la_LIBADD = liburcu-common.la

liburcu_qsbr_la_SOURCES = urcu-qsbr.c urcu-pointer.c $(COMPAT)
liburcu_qsbr_la_CFLAGS = -DRCU_QSBR $(AM_CFLAGS)
liburcu_qsbr_la_LIBADD = liburcu-common.la

liburcu_mb_la_SOURCES = urcu.c urcu-pointer.c $(COMPAT)
liburcu_mb_la_CFLAGS = -DRCU_MB $(AM_CFLAGS)
liburcu_mb_la_LIBADD = liburcu-common.la

liburcu_bp_la_SOURCES = urcu-bp.c urcu-pointer.c $(COMPAT)
liburcu_bp_la_LIBADD = liburcu-common.la

liburcu_cds_la_SOURCES = rculfqueue.c rculfstack.c lfstack.c \
	workqueue.c workqueue.h $(RCULFHASH) $(COMPAT)
liburcu_cds_la_LIBADD = liburcu-common.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liburcu-cds.pc liburcu.pc liburcu-bp.pc liburcu-qsbr.pc \
	liburcu-mb.pc liburcu-memb.pc

EXTRA_DIST = \
	urcu-call-rcu-impl.h \
	urcu-defer-impl.h \
	urcu-poll-impl.h \
	rculfhash-internal.h
