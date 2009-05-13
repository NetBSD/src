## Process this file with automake to produce Makefile.in
# Makefile for GNU CVS program.
#
# Copyright (C) 1986-2005 The Free Software Foundation, Inc.
#
# Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
#                                  and others.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

SHELL = /bin/sh

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib \
              -I$(top_srcdir)/diff $(ZLIB_CPPFLAGS)

bin_PROGRAMS = cvs
bin_SCRIPTS = cvsbug

# The cvs executable
cvs_SOURCES = \
	add.c \
	admin.c \
	annotate.c \
	buffer.c \
	checkin.c \
	checkout.c \
	classify.c \
	client.c \
	commit.c \
	create_adm.c \
	cvsrc.c \
	diff.c \
	edit.c \
	entries.c \
	error.c \
	exithandle.c \
	expand_path.c \
	fileattr.c \
	filesubr.c \
	find_names.c \
	hardlink.c \
	hash.c \
	history.c \
	ignore.c \
	import.c \
	lock.c \
	log.c \
	log-buffer.c log-buffer.h \
	login.c \
	logmsg.c \
	ls.c \
	main.c \
	mkmodules.c \
	modules.c \
	ms-buffer.c ms-buffer.h \
	myndbm.c \
	no_diff.c \
	parseinfo.c parseinfo.h \
	patch.c \
	rcs.c \
	rcscmds.c \
	recurse.c \
	release.c \
	remove.c \
	repos.c \
	root.c \
	rsh-client.c rsh-client.h \
	run.c \
	scramble.c \
	server.c \
	stack.c stack.h \
	status.c \
	subr.c subr.h \
	tag.c \
	update.c \
	version.c \
	vers_ts.c \
	watch.c \
	wrapper.c \
	zlib.c \
	buffer.h \
	client.h \
	cvs.h \
	edit.h \
	fileattr.h \
	hardlink.h \
	hash.h \
	history.h \
	myndbm.h \
	rcs.h \
	root.h \
	server.h \
	update.h \
	watch.h

EXTRA_cvs_SOURCES = gssapi-client.c gssapi-client.h \
                    kerberos4-client.c kerberos4-client.h \
		    socket-client.c socket-client.h

cvs_DEPENDENCIES = $(cvs_client_objects) \
	../diff/libdiff.a \
	../lib/libcvs.a \
	$(ZLIB_LIBS)
cvs_LDADD = $(cvs_client_objects) \
	../diff/libdiff.a \
	../lib/libcvs.a \
	$(ZLIB_LIBS) \
	$(LIB_CLOCK_GETTIME) \
	$(LIB_NANOSLEEP) \
	$(LIBINTL)

# General
EXTRA_DIST = \
	.cvsignore \
	ChangeLog-9194 \
	ChangeLog-9395 \
	ChangeLog-96 \
	ChangeLog-97 \
	build_src.com \
	sanity.sh

check-local: localcheck remotecheck proxycheck

.PHONY: localcheck
localcheck: sanity.config.sh
	$(SHELL) $(srcdir)/sanity.sh `pwd`/cvs$(EXEEXT)

.PHONY: remotecheck
remotecheck: all sanity.config.sh
	$(SHELL) $(srcdir)/sanity.sh -r `pwd`/cvs$(EXEEXT)

.PHONY: proxycheck
proxycheck: all sanity.config.sh
	$(SHELL) $(srcdir)/sanity.sh -p `pwd`/cvs$(EXEEXT)

# Our distclean targets
distclean-local:
	rm -f check.log check.plog check.plog~

## MAINTAINER Targets

# for backwards compatibility with the old makefiles
.PHONY: realclean
realclean: maintainer-clean
