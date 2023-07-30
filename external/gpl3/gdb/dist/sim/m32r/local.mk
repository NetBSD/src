## See sim/Makefile.am
##
## Copyright (C) 1996-2023 Free Software Foundation, Inc.
## Contributed by Cygnus Support.
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
	%D%/eng.h \
	%D%/mloop.c \
	%D%/stamp-mloop \
	%D%/engx.h \
	%D%/mloopx.c \
	%D%/stamp-mloop-x \
	%D%/eng2.h \
	%D%/mloop2.c \
	%D%/stamp-mloop-2

## This makes sure build tools are available before building the arch-subdirs.
SIM_ALL_RECURSIVE_DEPS += $(%C%_BUILD_OUTPUTS)

## FIXME: Use of `mono' is wip.
%D%/mloop.c %D%/eng.h: %D%/stamp-mloop ; @true
%D%/stamp-mloop: $(srccom)/genmloop.sh %D%/mloop.in
	$(AM_V_GEN)$(SHELL) $(srccom)/genmloop.sh -shell $(SHELL) \
		-mono -fast -pbb -switch sem-switch.c \
		-cpu m32rbf \
		-infile $(srcdir)/%D%/mloop.in -outfile-prefix %D%/
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng.hin %D%/eng.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop.cin %D%/mloop.c
	$(AM_V_at)touch $@

## FIXME: Use of `mono' is wip.
%D%/mloopx.c %D%/engx.h: %D%/stamp-mloop ; @true
%D%/stamp-mloop-x: $(srccom)/genmloop.sh %D%/mloop.in
	$(AM_V_GEN)$(SHELL) $(srccom)/genmloop.sh -shell $(SHELL) \
		-mono -no-fast -pbb -parallel-write -switch semx-switch.c \
		-cpu m32rxf \
		-infile $(srcdir)/%D%/mloopx.in -outfile-prefix %D%/ -outfile-suffix x
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/engx.hin %D%/engx.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloopx.cin %D%/mloopx.c
	$(AM_V_at)touch $@

## FIXME: Use of `mono' is wip.
%D%/mloop2.c %D%/eng2.h: %D%/stamp-mloop ; @true
%D%/stamp-mloop-2: $(srccom)/genmloop.sh %D%/mloop.in
	$(AM_V_GEN)$(SHELL) $(srccom)/genmloop.sh -shell $(SHELL) \
		-mono -no-fast -pbb -parallel-write -switch sem2-switch.c \
		-cpu m32r2f \
		-infile $(srcdir)/%D%/mloop2.in -outfile-prefix %D%/ -outfile-suffix 2
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng2.hin %D%/eng2.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop2.cin %D%/mloop2.c
	$(AM_V_at)touch $@

MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)
