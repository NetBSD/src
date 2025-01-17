# SPDX-FileCopyrightText: 2023 EfficiOS Inc.
#
# SPDX-License-Identifier: MIT

AM_CPPFLAGS += -I$(top_srcdir)/src -I$(top_srcdir)/tests/utils -I$(top_srcdir)/tests/common

LOG_DRIVER_FLAGS = --merge --comments
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' \
	URCU_TESTS_SRCDIR='$(abs_top_srcdir)/tests' \
	URCU_TESTS_BUILDDIR='$(abs_top_builddir)/tests' \
	$(SHELL) $(top_srcdir)/tests/utils/tap-driver.sh

noinst_PROGRAMS = \
	get_cpu_mask_from_sysfs \
	get_cpu_mask_from_sysfs_cxx \
	get_max_cpuid_from_sysfs \
	get_max_cpuid_from_sysfs_cxx \
	test_arch \
	test_arch_cxx \
	test_get_max_cpuid_from_mask \
	test_get_max_cpuid_from_mask_cxx \
	test_get_possible_cpus_array_len \
	test_get_possible_cpus_array_len_cxx \
	test_uatomic \
	test_uatomic_cxx \
	test_urcu_multiflavor \
	test_urcu_multiflavor_cxx \
	test_urcu_multiflavor_dynlink \
	test_urcu_multiflavor_dynlink_cxx \
	test_urcu_multiflavor_single_unit \
	test_urcu_multiflavor_single_unit_cxx \
	test_urcu_multiflavor_single_unit_dynlink \
	test_urcu_multiflavor_single_unit_dynlink_cxx \
	test_build \
	test_build_cxx \
	test_build_dynlink \
	test_build_dynlink_cxx

dist_noinst_SCRIPTS = \
	test_get_cpu_mask_from_sysfs \
	test_get_cpu_mask_from_sysfs_cxx \
	test_get_max_cpuid_from_sysfs \
	test_get_max_cpuid_from_sysfs_cxx

TESTS = \
	test_arch \
	test_arch_cxx \
	test_get_cpu_mask_from_sysfs \
	test_get_cpu_mask_from_sysfs_cxx \
	test_get_max_cpuid_from_mask \
	test_get_max_cpuid_from_mask_cxx \
	test_get_max_cpuid_from_sysfs \
	test_get_max_cpuid_from_sysfs_cxx \
	test_get_possible_cpus_array_len \
	test_get_possible_cpus_array_len_cxx \
	test_uatomic \
	test_uatomic_cxx \
	test_urcu_multiflavor \
	test_urcu_multiflavor_cxx \
	test_urcu_multiflavor_dynlink \
	test_urcu_multiflavor_dynlink_cxx \
	test_urcu_multiflavor_single_unit \
	test_urcu_multiflavor_single_unit_cxx \
	test_urcu_multiflavor_single_unit_dynlink \
	test_urcu_multiflavor_single_unit_dynlink_cxx \
	test_build \
	test_build_cxx \
	test_build_dynlink \
	test_build_dynlink_cxx

noinst_HEADERS = test_urcu_multiflavor.h

URCU_COMMON_LIB=$(top_builddir)/src/liburcu-common.la
URCU_LIB=$(top_builddir)/src/liburcu.la
URCU_QSBR_LIB=$(top_builddir)/src/liburcu-qsbr.la
URCU_MB_LIB=$(top_builddir)/src/liburcu-mb.la
URCU_BP_LIB=$(top_builddir)/src/liburcu-bp.la
URCU_CDS_LIB=$(top_builddir)/src/liburcu-cds.la
TAP_LIB=$(top_builddir)/tests/utils/libtap.a

get_cpu_mask_from_sysfs_cxx_SOURCES = get_cpu_mask_from_sysfs_cxx.cpp
get_cpu_mask_from_sysfs_SOURCES = get_cpu_mask_from_sysfs.c
get_max_cpuid_from_sysfs_cxx_SOURCES = get_max_cpuid_from_sysfs_cxx.cpp
get_max_cpuid_from_sysfs_SOURCES = get_max_cpuid_from_sysfs.c

test_arch_SOURCES = test_arch.c
test_arch_LDADD = $(URCU_COMMON_LIB) $(TAP_LIB)

test_arch_cxx_SOURCES = test_arch_cxx.cpp
test_arch_cxx_LDADD = $(URCU_COMMO_LIB) $(TAP_LIB)

test_get_max_cpuid_from_mask_SOURCES = test_get_max_cpuid_from_mask.c
test_get_max_cpuid_from_mask_LDADD = $(TAP_LIB)

test_get_max_cpuid_from_mask_cxx_SOURCES = test_get_max_cpuid_from_mask_cxx.cpp
test_get_max_cpuid_from_mask_cxx_LDADD = $(TAP_LIB)

test_get_possible_cpus_array_len_SOURCES = test_get_possible_cpus_array_len.c
test_get_possible_cpus_array_len_LDADD = $(TAP_LIB)

test_get_possible_cpus_array_len_cxx_SOURCES = test_get_possible_cpus_array_len_cxx.cpp
test_get_possible_cpus_array_len_cxx_LDADD = $(TAP_LIB)

test_uatomic_SOURCES = test_uatomic.c
test_uatomic_LDADD = $(URCU_COMMON_LIB) $(TAP_LIB)

test_uatomic_cxx_SOURCES = test_uatomic_cxx.cpp
test_uatomic_cxx_LDADD = $(URCU_COMMON_LIB) $(TAP_LIB)

test_urcu_multiflavor_SOURCES = test_urcu_multiflavor.c \
	test_urcu_multiflavor-memb.c \
	test_urcu_multiflavor-mb.c \
	test_urcu_multiflavor-qsbr.c \
	test_urcu_multiflavor-bp.c
test_urcu_multiflavor_LDADD = $(URCU_LIB) $(URCU_MB_LIB) \
	$(URCU_QSBR_LIB) $(URCU_BP_LIB) $(TAP_LIB)

test_urcu_multiflavor_cxx_SOURCES = test_urcu_multiflavor_cxx.cpp \
	test_urcu_multiflavor-memb_cxx.cpp \
	test_urcu_multiflavor-mb_cxx.cpp \
	test_urcu_multiflavor-qsbr_cxx.cpp \
	test_urcu_multiflavor-bp_cxx.cpp
test_urcu_multiflavor_cxx_LDADD = $(URCU_LIB) $(URCU_MB_LIB) \
	$(URCU_QSBR_LIB) $(URCU_BP_LIB) $(TAP_LIB)

test_urcu_multiflavor_dynlink_SOURCES = test_urcu_multiflavor.c \
	test_urcu_multiflavor-memb.c \
	test_urcu_multiflavor-mb.c \
	test_urcu_multiflavor-qsbr.c \
	test_urcu_multiflavor-bp.c
test_urcu_multiflavor_dynlink_CFLAGS = -DDYNAMIC_LINK_TEST $(AM_CFLAGS)
test_urcu_multiflavor_dynlink_LDADD = $(URCU_LIB) $(URCU_MB_LIB) \
	$(URCU_QSBR_LIB) $(URCU_BP_LIB) $(TAP_LIB)

test_urcu_multiflavor_dynlink_cxx_SOURCES = test_urcu_multiflavor_cxx.cpp \
	test_urcu_multiflavor-memb_cxx.cpp \
	test_urcu_multiflavor-mb_cxx.cpp \
	test_urcu_multiflavor-qsbr_cxx.cpp \
	test_urcu_multiflavor-bp_cxx.cpp
test_urcu_multiflavor_dynlink_cxx_CXXFLAGS = -DDYNAMIC_LINK_TEST $(AM_CXXFLAGS)
test_urcu_multiflavor_dynlink_cxx_LDADD = $(URCU_LIB) $(URCU_MB_LIB) \
	$(URCU_QSBR_LIB) $(URCU_BP_LIB) $(TAP_LIB)

test_urcu_multiflavor_single_unit_SOURCES = test_urcu_multiflavor_single_unit.c
test_urcu_multiflavor_single_unit_LDADD = $(URCU_LIB) $(URCU_MB_LIB) \
	$(URCU_QSBR_LIB) $(URCU_BP_LIB) $(TAP_LIB)

test_urcu_multiflavor_single_unit_cxx_SOURCES = test_urcu_multiflavor_single_unit_cxx.cpp
test_urcu_multiflavor_single_unit_cxx_LDADD = $(URCU_LIB) $(URCU_MB_LIB) \
	$(URCU_QSBR_LIB) $(URCU_BP_LIB) $(TAP_LIB)

test_urcu_multiflavor_single_unit_dynlink_SOURCES = test_urcu_multiflavor_single_unit.c
test_urcu_multiflavor_single_unit_dynlink_CFLAGS = -DDYNAMIC_LINK_TEST $(AM_CFLAGS)
test_urcu_multiflavor_single_unit_dynlink_LDADD = $(URCU_LIB) $(URCU_MB_LIB) \
	$(URCU_QSBR_LIB) $(URCU_BP_LIB) $(TAP_LIB)

test_urcu_multiflavor_single_unit_dynlink_cxx_SOURCES = test_urcu_multiflavor_single_unit_cxx.cpp
test_urcu_multiflavor_single_unit_dynlink_cxx_CXXFLAGS = -DDYNAMIC_LINK_TEST $(AM_CXXFLAGS)
test_urcu_multiflavor_single_unit_dynlink_cxx_LDADD = $(URCU_LIB) $(URCU_MB_LIB) \
	$(URCU_QSBR_LIB) $(URCU_BP_LIB) $(TAP_LIB)

test_build_SOURCES = \
	test_build.c
test_build_LDADD = $(URCU_COMMON_LIB) $(URCU_CDS_LIB) $(TAP_LIB)

test_build_cxx_SOURCES = \
	test_build_cxx.cpp
test_build_cxx_LDADD = $(URCU_COMMON_LIB) $(URCU_CDS_LIB) $(TAP_LIB)

test_build_dynlink_SOURCES = \
	test_build.c
test_build_dynlink_CFLAGS = -DDYNAMIC_LINK_TEST $(AM_CFLAGS)
test_build_dynlink_LDADD = $(URCU_LIB) $(URCU_COMMON_LIB) $(URCU_CDS_LIB) $(TAP_LIB)

test_build_dynlink_cxx_SOURCES = \
	test_build_cxx.cpp
test_build_dynlink_cxx_CXXFLAGS = -DDYNAMIC_LINK_TEST $(AM_CXXFLAGS)
test_build_dynlink_cxx_LDADD = $(URCU_LIB) $(URCU_COMMON_LIB) $(URCU_CDS_LIB) $(TAP_LIB)
