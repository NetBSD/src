## See sim/Makefile.am
##
## Copyright (C) 1996-2023 Free Software Foundation, Inc.
## Written by Cygnus Support.
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

%C%_BUILT_SRC_FROM_IGEN = \
	%D%/icache.h \
	%D%/icache.c \
	%D%/idecode.h \
	%D%/idecode.c \
	%D%/semantics.h \
	%D%/semantics.c \
	%D%/model.h \
	%D%/model.c \
	%D%/support.h \
	%D%/support.c \
	%D%/itable.h \
	%D%/itable.c \
	%D%/engine.h \
	%D%/engine.c \
	%D%/irun.c
%C%_BUILD_OUTPUTS = \
	$(%C%_BUILT_SRC_FROM_IGEN) \
	%D%/stamp-igen

## This makes sure build tools are available before building the arch-subdirs.
SIM_ALL_RECURSIVE_DEPS += $(%C%_BUILD_OUTPUTS)

$(%C%_BUILT_SRC_FROM_IGEN): %D%/stamp-igen

%C%_IGEN_TRACE = # -G omit-line-numbers # -G trace-rule-selection -G trace-rule-rejection -G trace-entries
%C%_IGEN_INSN = $(srcdir)/%D%/mn10300.igen
%C%_IGEN_INSN_INC = %D%/am33.igen %D%/am33-2.igen
%C%_IGEN_DC = $(srcdir)/%D%/mn10300.dc
%D%/stamp-igen: $(%C%_IGEN_INSN) $(%C%_IGEN_INSN_INC) $(%C%_IGEN_DC) $(IGEN)
	$(AM_V_GEN)$(IGEN_RUN) \
		$(%C%_IGEN_TRACE) \
		-G gen-direct-access \
		-M mn10300,am33 -G gen-multi-sim=am33 \
		-M am33_2 \
		-I $(srcdir)/%D% \
		-i $(%C%_IGEN_INSN) \
		-o $(%C%_IGEN_DC) \
		-x \
		-n icache.h    -hc %D%/tmp-icache.h \
		-n icache.c    -c  %D%/tmp-icache.c \
		-n semantics.h -hs %D%/tmp-semantics.h \
		-n semantics.c -s  %D%/tmp-semantics.c \
		-n idecode.h   -hd %D%/tmp-idecode.h \
		-n idecode.c   -d  %D%/tmp-idecode.c \
		-n model.h     -hm %D%/tmp-model.h \
		-n model.c     -m  %D%/tmp-model.c \
		-n support.h   -hf %D%/tmp-support.h \
		-n support.c   -f  %D%/tmp-support.c \
		-n itable.h    -ht %D%/tmp-itable.h \
		-n itable.c    -t  %D%/tmp-itable.c \
		-n engine.h    -he %D%/tmp-engine.h \
		-n engine.c    -e  %D%/tmp-engine.c \
		-n irun.c      -r  %D%/tmp-irun.c
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-icache.h %D%/icache.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-icache.c %D%/icache.c
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-idecode.h %D%/idecode.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-idecode.c %D%/idecode.c
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-semantics.h %D%/semantics.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-semantics.c %D%/semantics.c
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-model.h %D%/model.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-model.c %D%/model.c
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-support.h %D%/support.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-support.c %D%/support.c
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-itable.h %D%/itable.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-itable.c %D%/itable.c
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-engine.h %D%/engine.h
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-engine.c %D%/engine.c
	$(AM_V_at)$(SHELL) $(srcroot)/move-if-change %D%/tmp-irun.c %D%/irun.c
	$(AM_V_at)touch $@

MOSTLYCLEANFILES += $(%C%_BUILD_OUTPUTS)
