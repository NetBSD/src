## Process this file with automake to produce Makefile.in

if TRE_AGREP
agrep_dirs = src doc
else
agrep_dirs =
endif

SUBDIRS = lib $(agrep_dirs) tests utils po m4

EXTRA_DIST = \
	LICENSE \
	win32/tre-config.h win32/config.h \
	win32/tre.vcproj \
	win32/tre.sln \
	win32/retest.vcproj \
	python/tre-python.c \
	python/setup.py \
	python/example.py

ACLOCAL_AMFLAGS = -I m4
AC_CONFIG_AUX_DIR = utils

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = tre.pc
