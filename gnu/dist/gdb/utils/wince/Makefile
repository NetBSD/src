# Generated automatically from Makefile.in by configure.
#Copyright 1999 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

prefix = /usr/local
exec_prefix = ${prefix}

host_alias = sun4
target_alias = sun4
program_transform_name = s,x,x,
bindir = $(exec_prefix)/bin
libdir = $(exec_prefix)/lib
tooldir = $(libdir)/$(target_alias)

datadir = $(prefix)/share
mandir = $(prefix)/man
man1dir = $(mandir)/man1
man2dir = $(mandir)/man2
man3dir = $(mandir)/man3
man4dir = $(mandir)/man4
man5dir = $(mandir)/man5
man6dir = $(mandir)/man6
man7dir = $(mandir)/man7
man8dir = $(mandir)/man8
man9dir = $(mandir)/man9
infodir = $(prefix)/info
includedir = $(prefix)/include

SHELL = /bin/sh

INSTALL = /bin/sh /sourceware/snapshot-tmp/gdbadmin-tmp/gdb_5_3-branch/5.3/gdb/src/install-sh -c
INSTALL_PROGRAM = /bin/sh /sourceware/snapshot-tmp/gdbadmin-tmp/gdb_5_3-branch/5.3/gdb/src/install-sh -c 
INSTALL_DATA = /bin/sh /sourceware/snapshot-tmp/gdbadmin-tmp/gdb_5_3-branch/5.3/gdb/src/install-sh -c -m 644

CC=gcc
EXEEXT = 

# Directory containing source files.
srcdir = .././wince
VPATH = .././wince

# Where is the "-liberty" library?  Typically in ../libiberty.
LIBIBERTY = ../../libiberty/libiberty.a

# All the includes used for CFLAGS and for lint.
# -I. for config files.
# -I${srcdir} possibly for regex.h also.
INCLUDE_CFLAGS = -I. -I${srcdir} -I$(srcdir)/../../include

# CFLAGS is specifically reserved for setting from the command line
# when running make.  I.E.  "make USER_CFLAGS=-Wmissing-prototypes".
CFLAGS = -g

# INTERNAL_CFLAGS is the aggregate of all other *CFLAGS macros.
INTERNAL_CFLAGS = $(CFLAGS) $(INCLUDE_CFLAGS) $(USER_CFLAGS)  
# None of the things in CFLAGS will do any harm, and on some systems
#  (e.g. SunOS4) it is important to use the MH_CFLAGS.
LDFLAGS = $(CFLAGS)

# Libraries and corresponding dependencies for compiling gdb.
CLIBS = ${LIBIBERTY}
CDEPS = ${LIBIBERTY}

# Prevent Sun make from putting in the machine type.  Setting
# TARGET_ARCH to nothing works for SunOS 3, 4.0, but not for 4.1.
.c.o:
	${CC} -c ${INTERNAL_CFLAGS} $<

PROGS = cesetup$(EXEEXT)

.PHONY: all info install install-info

all: $(PROGS)

info:

install: $(PROGS)
	if [ "$(PROGS)" != "" ]; then \
		for i in $(PROGS); do \
			$(INSTALL_PROGRAM) $$i $(bindir)/$$i; \
		done; \
	fi

install-info:

cesetup$(EXEEXT):	cesetup.o
	$(CC) $(LDFLAGS) -o cesetup$(EXEEXT) cesetup.o $(CLIBS)

cesetup.o: cesetup.c

mostlyclean:

clean:		mostlyclean
	rm -f *.o $(PROGS)

distclean:	clean
	rm -f config.log config.cache

maintainer-clean realclean:	distclean

Makefile: Makefile.in config.status
	$(SHELL) config.status

config.status: configure
	$(SHELL) config.status --recheck

