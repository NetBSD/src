## See sim/Makefile.am
##
## Copyright (C) 2004-2023 Free Software Foundation, Inc.
## Contributed by Axis Communications.
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

## rvdummy is just used for testing -- it runs on the same host as `run`.
## It does nothing if --enable-sim-hardware isn't active.
%C%_rvdummy_SOURCES = %D%/rvdummy.c
%C%_rvdummy_LDADD = $(LIBIBERTY_LIB)

check_PROGRAMS += %D%/rvdummy

%C%_BUILD_OUTPUTS = \
	%D%/engv10.h \
	%D%/mloopv10f.c \
	%D%/stamp-mloop-v10f \
	%D%/engv32.h \
	%D%/mloopv32f.c \
	%D%/stamp-mloop-v32f

## This makes sure build tools are available before building the arch-subdirs.
SIM_ALL_RECURSIVE_DEPS += $(%C%_BUILD_OUTPUTS)

## FIXME: What is mono and what does "Use of `mono' is wip" mean (other
## than the apparent; some "mono" feature is work in progress)?
%D%/mloopv10f.c %D%/engv10.h: %D%/stamp-mloop-v10f ; @true
%D%/stamp-mloop-v10f: $(srccom)/genmloop.sh %D%/mloop.in
	$(AM_V_GEN)$(SHELL) $(srccom)/genmloop.sh -shell $(SHELL) \
		-mono -no-fast -pbb -switch semcrisv10f-switch.c \
		-cpu crisv10f \
		-infile $(srcdir)/%D%/mloop.in -outfile-prefix %D%/ -outfile-suffix -v10f
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng-v10f.hin %D%/engv10.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop-v10f.cin %D%/mloopv10f.c
	$(AM_V_at)touch $@

## FIXME: What is mono and what does "Use of `mono' is wip" mean (other
## than the apparent; some "mono" feature is work in progress)?
%D%/mloopv32f.c %D%/engv32.h: %D%/stamp-mloop-v32f ; @true
%D%/stamp-mloop-v32f: $(srccom)/genmloop.sh %D%/mloop.in
	$(AM_V_GEN)$(SHELL) $(srccom)/genmloop.sh -shell $(SHELL) \
		-mono -no-fast -pbb -switch semcrisv32f-switch.c \
		-cpu crisv32f \
		-infile $(srcdir)/%D%/mloop.in -outfile-prefix %D%/ -outfile-suffix -v32f
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng-v32f.hin %D%/engv32.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop-v32f.cin %D%/mloopv32f.c
	$(AM_V_at)touch $@

MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)
