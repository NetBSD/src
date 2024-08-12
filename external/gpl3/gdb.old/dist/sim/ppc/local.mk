## See sim/Makefile.am
##
## Copyright (C) 1994-2023 Free Software Foundation, Inc.
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

%D%/psim$(EXEEXT): %D%/run$(EXEEXT)
	$(AM_V_GEN)ln $< $@ 2>/dev/null || $(LN_S) $< $@ 2>/dev/null || cp -p $< $@

## Helper targets for running make from the top-level due to run's sis.o.
%D%/%.o: %D%/%.c | %D%/libsim.a $(SIM_ALL_RECURSIVE_DEPS)
	$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

noinst_PROGRAMS += %D%/run %D%/psim

%C%docdir = $(docdir)/%C%
%C%doc_DATA = %D%/BUGS %D%/INSTALL %D%/README %D%/RUN
