## See sim/Makefile.am
##
## Copyright (C) 2008-2023 Free Software Foundation, Inc.
## Contributed by M Ranga Swami Reddy <MR.Swami.Reddy@nsc.com>
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
	%D%/nrun.o \
	%D%/libsim.a \
	$(SIM_COMMON_LIBS)

noinst_PROGRAMS += %D%/run

%C%_BUILD_OUTPUTS = \
	%D%/gencode$(EXEEXT) \
	%D%/simops.h \
	%D%/table.c

## This makes sure build tools are available before building the arch-subdirs.
SIM_ALL_RECURSIVE_DEPS += $(%C%_BUILD_OUTPUTS)

%C%_gencode_SOURCES = %D%/gencode.c
%C%_gencode_LDADD = %D%/cr16-opc.o

# These rules are copied from automake, but tweaked to use FOR_BUILD variables.
%D%/gencode$(EXEEXT): $(%C%_gencode_OBJECTS) $(%C%_gencode_DEPENDENCIES) %D%/$(am__dirstamp)
	$(AM_V_CCLD)$(LINK_FOR_BUILD) $(%C%_gencode_OBJECTS) $(%C%_gencode_LDADD)

# gencode is a build-time only tool.  Override the default rules for it.
%D%/gencode.o: %D%/gencode.c
	$(AM_V_CC)$(COMPILE_FOR_BUILD) -c $< -o $@
%D%/cr16-opc.o: ../opcodes/cr16-opc.c
	$(AM_V_CC)$(COMPILE_FOR_BUILD) -c $< -o $@

%D%/simops.h: %D%/gencode$(EXEEXT)
	$(AM_V_GEN)$< -h >$@

%D%/table.c: %D%/gencode$(EXEEXT)
	$(AM_V_GEN)$< >$@

EXTRA_PROGRAMS += %D%/gencode
MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)
