## See sim/Makefile.am
##
## Copyright (C) 2020-2023 Free Software Foundation, Inc.
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
	%D%/eng-le.h \
	%D%/mloop-le.c \
	%D%/stamp-mloop-le \
	%D%/eng-be.h \
	%D%/mloop-be.c \
	%D%/stamp-mloop-be

## This makes sure build tools are available before building the arch-subdirs.
SIM_ALL_RECURSIVE_DEPS += $(%C%_BUILD_OUTPUTS)

%D%/mloop-le.c %D%/eng-le.h: %D%/stamp-mloop-le ; @true
%D%/stamp-mloop-le: $(srccom)/genmloop.sh %D%/mloop.in
	$(AM_V_GEN)$(SHELL) $(srccom)/genmloop.sh -shell $(SHELL) \
		-mono -scache -prefix bpfbf_ebpfle -cpu bpfbf \
		-infile $(srcdir)/%D%/mloop.in \
		-outfile-prefix %D%/ -outfile-suffix -le
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng-le.hin %D%/eng-le.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop-le.cin %D%/mloop-le.c
	$(AM_V_at)touch $@

%D%/mloop-be.c %D%/eng-be.h: %D%/stamp-mloop-be ; @true
%D%/stamp-mloop-be: $(srccom)/genmloop.sh %D%/mloop.in
	$(AM_V_GEN)$(SHELL) $(srccom)/genmloop.sh -shell $(SHELL) \
		-mono -scache -prefix bpfbf_ebpfbe -cpu bpfbf \
		-infile $(srcdir)/%D%/mloop.in \
		-outfile-prefix %D%/ -outfile-suffix -be
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/eng-be.hin %D%/eng-be.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/mloop-be.cin %D%/mloop-be.c
	$(AM_V_at)touch $@

MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)
