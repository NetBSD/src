## See sim/Makefile.am.
##
## Copyright (C) 1997-2023 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.

## Parts of the common/ sim code that have been unified.
## Most still lives in common/Make-common.in.

AM_CPPFLAGS += \
	-I$(srcdir)/%D% \
	-DSIM_COMMON_BUILD
AM_CPPFLAGS_FOR_BUILD += -I$(srcdir)/%D%

## This makes sure common parts are available before building the arch-subdirs
## which will refer to these.
SIM_ALL_RECURSIVE_DEPS += \
	%D%/libcommon.a

## NB: libcommon.a isn't used directly by ports.  We need a target for common
## objects to be a part of, and ports use the individual objects directly.
SIM_COMMON_LIB = %D%/libcommon.a
noinst_LIBRARIES += $(SIM_COMMON_LIB)
%C%_libcommon_a_SOURCES = \
	%D%/callback.c \
	%D%/portability.c \
	%D%/sim-load.c \
	%D%/syscall.c \
	%D%/target-newlib-errno.c \
	%D%/target-newlib-open.c \
	%D%/target-newlib-signal.c \
	%D%/target-newlib-syscall.c \
	%D%/version.c

%D%/version.c: %D%/version.c-stamp ; @true
%D%/version.c-stamp: $(srcroot)/gdb/version.in $(srcroot)/bfd/version.h $(srcdir)/%D%/create-version.sh
	$(AM_V_GEN)$(SHELL) $(srcdir)/%D%/create-version.sh $(srcroot)/gdb $@.tmp
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change $@.tmp $(@:-stamp=)
	$(AM_V_at)touch $@

CLEANFILES += \
	%D%/version.c %D%/version.c-stamp

##
## For subdirs.
##

LIBIBERTY_LIB = ../libiberty/libiberty.a
BFD_LIB = ../bfd/libbfd.la
OPCODES_LIB = ../opcodes/libopcodes.la

SIM_COMMON_LIBS = \
	$(SIM_COMMON_LIB) \
	$(BFD_LIB) \
	$(OPCODES_LIB) \
	$(LIBIBERTY_LIB) \
	$(LIBGNU) \
	$(LIBGNU_EXTRA_LIBS)
