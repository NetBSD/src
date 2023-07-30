## See sim/Makefile.am
##
## Copyright (C) 2005-2023 Free Software Foundation, Inc.
## Contributed by Red Hat, Inc.
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

%C%_run_SOURCES =
%C%_run_LDADD = \
	%D%/main.o \
	%D%/libsim.a \
	$(SIM_COMMON_LIBS)

noinst_PROGRAMS += %D%/run

## Helper targets for running make from the top-level due to run's main.o.
%D%/%.o: %D%/%.c | %D%/libsim.a $(SIM_ALL_RECURSIVE_DEPS)
	$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

%C%_BUILD_OUTPUTS = \
	%D%/opc2c$(EXEEXT) \
	%D%/m32c.c \
	%D%/r8c.c

## This makes sure build tools are available before building the arch-subdirs.
SIM_ALL_RECURSIVE_DEPS += $(%C%_BUILD_OUTPUTS)

%C%_opc2c_SOURCES = %D%/opc2c.c

# These rules are copied from automake, but tweaked to use FOR_BUILD variables.
%D%/opc2c$(EXEEXT): $(%C%_opc2c_OBJECTS) $(%C%_opc2c_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_opc2c_OBJECTS) $(%C%_opc2c_LDADD)

# opc2c is a build-time only tool.  Override the default rules for it.
%D%/opc2c.o: %D%/opc2c.c
	$(AM_V_CC)$(COMPILE_FOR_BUILD) -c $< -o $@

# opc2c leaks memory, and therefore makes AddressSanitizer unhappy.  Disable
# leak detection while running it.
%C%_OPC2C_RUN = ASAN_OPTIONS=detect_leaks=0 %D%/opc2c$(EXEEXT)

%D%/m32c.c: %D%/m32c.opc %D%/opc2c$(EXEEXT)
	$(AM_V_GEN)$(%C%_OPC2C_RUN) -l $@.log $< > $@.tmp
	$(AM_V_at)mv $@.tmp $@

%D%/r8c.c: %D%/r8c.opc %D%/opc2c$(EXEEXT)
	$(AM_V_GEN)$(%C%_OPC2C_RUN) -l $@.log $< > $@.tmp
	$(AM_V_at)mv $@.tmp $@

EXTRA_PROGRAMS += %D%/opc2c
MOSTLYCLEANFILES += \
	$(%C%_BUILD_OUTPUTS) \
	%D%/m32c.c.log \
	%D%/r8c.c.log
