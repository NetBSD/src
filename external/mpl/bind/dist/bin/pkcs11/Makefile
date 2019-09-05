# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

srcdir =	.

top_srcdir =	../..

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# Search for machine-generated header files in the build tree,
# and for normal headers in the source tree (${top_srcdir}).
# We only need to look in OS-specific subdirectories for the
# latter case, because there are no machine-generated OS-specific
# headers.

ISC_INCLUDES = -I/p/netbsd/cvsroot/src/external/mpl/bind/import/lib/isc/include \
	-I${top_srcdir}/lib/isc \
	-I${top_srcdir}/lib/isc/include \
	-I${top_srcdir}/lib/isc/unix/include \
	-I${top_srcdir}/lib/isc/pthreads/include

ISCCC_INCLUDES = -I/p/netbsd/cvsroot/src/external/mpl/bind/import/lib/isccc/include \
       -I${top_srcdir}/lib/isccc/include

ISCCFG_INCLUDES = -I/p/netbsd/cvsroot/src/external/mpl/bind/import/lib/isccfg/include \
       -I${top_srcdir}/lib/isccfg/include

DNS_INCLUDES = -I/p/netbsd/cvsroot/src/external/mpl/bind/import/lib/dns/include \
	-I${top_srcdir}/lib/dns/include

NS_INCLUDES = -I/p/netbsd/cvsroot/src/external/mpl/bind/import/lib/ns/include \
	-I${top_srcdir}/lib/ns/include

IRS_INCLUDES = -I/p/netbsd/cvsroot/src/external/mpl/bind/import/lib/irs/include \
	-I${top_srcdir}/lib/irs/include

BIND9_INCLUDES = -I/p/netbsd/cvsroot/src/external/mpl/bind/import/lib/bind9/include \
	-I${top_srcdir}/lib/bind9/include

TEST_INCLUDES = \
	-I${top_srcdir}/lib/tests/include

CINCLUDES =	${ISC_INCLUDES}

CDEFINES =

ISCLIBS =	../../lib/isc/libisc.a -L/usr/lib -lcrypto

ISCDEPLIBS =	../../lib/isc/libisc.a

DEPLIBS =	${ISCDEPLIBS}

# if FORCE_STATIC_PROVIDER: LIBS += ${PROVIDER}
LIBS =		${ISCLIBS}   -Wl,-R/usr/pkg/lib -L/usr/pkg/lib -lxml2 -L/usr/lib -lz -L/usr/lib -llzma -L/usr/lib -lm 

SUBDIRS =	benchmarks

TARGETS =	pkcs11-list pkcs11-destroy \
		pkcs11-keygen pkcs11-tokens
SRCS =		pkcs11-list.c pkcs11-destroy.c \
		pkcs11-keygen.c pkcs11-tokens.c
OBJS =		pkcs11-list.o pkcs11-destroy.o \
		pkcs11-keygen.o pkcs11-tokens.o


MANPAGES =	pkcs11-list.8 pkcs11-destroy.8 \
		pkcs11-keygen.8 pkcs11-tokens.8
HTMLPAGES =	pkcs11-list.html pkcs11-destroy.html \
		pkcs11-keygen.html pkcs11-tokens.html
MANOBJS =	${MANPAGES} ${HTMLPAGES}

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

###
### Common Makefile rules for BIND 9.
###

###
### Paths
###
### Note: paths that vary by Makefile MUST NOT be listed
### here, or they won't get expanded correctly.

prefix =	/usr/local
exec_prefix =	${prefix}
bindir =	${exec_prefix}/bin
sbindir =	${exec_prefix}/sbin
includedir =	${prefix}/include
libdir =	${exec_prefix}/lib
sysconfdir =	/etc
localstatedir =	/var
mandir =	${datarootdir}/man
datarootdir =   ${prefix}/share

plugindir =	${libdir}/named

DESTDIR =



top_builddir =	/p/netbsd/cvsroot/src/external/mpl/bind/import

###
### All
###
### Makefile may define:
###	PREREQS
###	TARGETS

all: ${PREREQS} subdirs ${TARGETS} testdirs

###
### Subdirectories
###
### Makefile may define:
###	SUBDIRS
###	DEPDIRS

ALL_SUBDIRS = ${SUBDIRS} nulldir
ALL_TESTDIRS = ${TESTDIRS} nulldir

#
# We use a single-colon rule so that additional dependencies of
# subdirectories can be specified after the inclusion of this file.
# The "depend" and "testdirs" targets are treated the same way.
#
subdirs:
	@for i in ${ALL_SUBDIRS}; do \
		if [ "$$i" != "nulldir" -a -d $$i ]; then \
			echo "making all in `pwd`/$$i"; \
			(cd $$i; ${MAKE} ${MAKEDEFS} DESTDIR="${DESTDIR}" all) || exit 1; \
		fi; \
	done

#
# Tests are built after the targets instead of before
#
testdirs: subdirs ${TARGETS}
	@for i in ${ALL_TESTDIRS}; do \
		if [ "$$i" != "nulldir" -a -d $$i ]; then \
			echo "making all in `pwd`/$$i"; \
			(cd $$i; ${MAKE} ${MAKEDEFS} DESTDIR="${DESTDIR}" all) || exit 1; \
		fi; \
	done

install:: all

install uninstall clean distclean maintainer-clean doc docclean man manclean::
	@for i in ${ALL_SUBDIRS} ${ALL_TESTDIRS}; do \
		if [ "$$i" != "nulldir" -a -d $$i -a -f $$i/Makefile ]; then \
			echo "making $@ in `pwd`/$$i"; \
			(cd $$i; ${MAKE} ${MAKEDEFS} DESTDIR="${DESTDIR}" $@) || exit 1; \
		fi; \
	done

###
### C Programs
###
### Makefile must define
###	CC
### Makefile may define
###	CFLAGS
###	LDFLAGS
###	CINCLUDES
###	CDEFINES
###	CWARNINGS
### User may define externally
###     EXT_CFLAGS

CC = 		gcc
CFLAGS =	-g -O2 -pthread -I/usr/pkg/include/libxml2 -I/usr/include  -fPIC
LDFLAGS =	 -Wl,-E
STD_CINCLUDES =	
STD_CDEFINES =	
STD_CWARNINGS =	 -W -Wall -Wmissing-prototypes -Wcast-qual -Wwrite-strings -Wformat -Wpointer-arith -fno-strict-aliasing -fno-delete-null-pointer-checks

BUILD_CC = gcc
BUILD_CFLAGS = -g -O2 -pthread -I/usr/pkg/include/libxml2 -I/usr/include  -fPIC
BUILD_CPPFLAGS =  
BUILD_LDFLAGS =  -Wl,-E
BUILD_LIBS =   -Wl,-R/usr/pkg/lib -L/usr/pkg/lib -lxml2 -L/usr/lib -lz -L/usr/lib -llzma -L/usr/lib -lm 

LFS_CFLAGS = 
LFS_LDFLAGS = 
LFS_LIBS = 

MAXMINDDB_CFLAGS = 
MAXMINDDB_LIBS = 

.SUFFIXES:
.SUFFIXES: .c .o

ALWAYS_INCLUDES = -I${top_builddir} -I${top_srcdir}
ALWAYS_DEFINES = 
ALWAYS_WARNINGS =

ALL_CPPFLAGS = \
	${ALWAYS_INCLUDES} ${CINCLUDES} ${STD_CINCLUDES} \
	${ALWAYS_DEFINES} ${CDEFINES} ${STD_CDEFINES}

ALL_CFLAGS = ${EXT_CFLAGS} ${ALL_CPPFLAGS} ${CFLAGS} \
	${ALWAYS_WARNINGS} ${STD_CWARNINGS} ${CWARNINGS}

.c.o:
	${LIBTOOL_MODE_COMPILE} ${CC} ${ALL_CFLAGS} -c $<

SHELL = /bin/sh
LIBTOOL = 
LIBTOOL_MODE_COMPILE = ${LIBTOOL} 
LIBTOOL_MODE_INSTALL = ${LIBTOOL} 
LIBTOOL_MODE_LINK = ${LIBTOOL} 
LIBTOOL_MODE_UNINSTALL = ${LIBTOOL} 
PURIFY = 

MKDEP = ${SHELL} ${top_builddir}/make/mkdep

###
### This is a template compound command to build an executable binary with
### an internal symbol table.
### This process is tricky.  We first link all objects including a tentative
### empty symbol table, then get a tentative list of symbols from the resulting
### binary ($@tmp0).  Next, we re-link all objects, but this time with the
### symbol table just created ($tmp@1).  The set of symbols should be the same,
### but the corresponding addresses would be changed due to the difference on
### the size of symbol tables.  So we create the symbol table and re-create the
### objects once again.  Finally, we check the symbol table embedded in the
### final binaryis consistent with the binary itself; otherwise the process is
### terminated.
###
### To minimize the overhead of creating symbol tables, the autoconf switch
### --enable-symtable takes an argument so that the symbol table can be created
### on a per application basis: unless the argument is set to "all", the symbol
### table is created only when a shell (environment) variable "MAKE_SYMTABLE" is
### set to a non-null value in the rule to build the executable binary.
###
### Each Makefile.in that uses this macro is expected to define "LIBS" and
### "NOSYMLIBS"; the former includes libisc with an empty symbol table, and
### the latter includes libisc without the definition of a symbol table.
### The rule to make the executable binary will look like this
### binary: ${OBJS}
###     #export MAKE_SYMTABLE="yes"; \  <- enable if symtable is always needed
###	export BASEOBJS="${OBJS}"; \
###	${FINALBUILDCMD}
###
### Normally, ${LIBS} includes all necessary libraries to build the binary;
### there are some exceptions however, where the rule lists some of the
### necessary libraries explicitly in addition to (or instead of) ${LIBS},
### like this:
### binary: ${OBJS}
###     cc -o $@ ${OBJS} ${OTHERLIB1} ${OTHERLIB2} ${LIBS}
### in order to modify such a rule to use this compound command, a separate
### variable "LIBS0" should be deinfed for the explicitly listed libraries,
### while making sure ${LIBS} still includes libisc.  So the above rule would
### be modified as follows:
### binary: ${OBJS}
###	export BASEOBJS="${OBJS}"; \
###	export LIBS0="${OTHERLIB1} ${OTHERLIB2}"; \
###     ${FINALBUILDCMD}
### See bin/check/Makefile.in for a complete example of the use of LIBS0.
###
FINALBUILDCMD = if [ X"${MKSYMTBL_PROGRAM}" = X -o X"$${MAKE_SYMTABLE:-${ALWAYS_MAKE_SYMTABLE}}" = X ] ; then \
		${LIBTOOL_MODE_LINK} ${PURIFY} ${CC} ${ALL_CFLAGS} ${LDFLAGS} \
		-o $@ $${BASEOBJS} $${LIBS0} ${LIBS}; \
	else \
		rm -f $@tmp0; \
		${LIBTOOL_MODE_LINK} ${PURIFY} ${CC} ${ALL_CFLAGS} ${LDFLAGS} \
		-o $@tmp0 $${BASEOBJS} $${LIBS0} ${LIBS} || exit 1; \
		rm -f $@-symtbl.c $@-symtbl.o; \
		${MKSYMTBL_PROGRAM} ${top_srcdir}/util/mksymtbl.pl \
		-o $@-symtbl.c $@tmp0 || exit 1; \
		$(MAKE) $@-symtbl.o || exit 1; \
		rm -f $@tmp1; \
		${LIBTOOL_MODE_LINK} ${PURIFY} ${CC} ${ALL_CFLAGS} ${LDFLAGS} \
		-o $@tmp1 $${BASEOBJS} $@-symtbl.o $${LIBS0} ${NOSYMLIBS} || exit 1; \
		rm -f $@-symtbl.c $@-symtbl.o; \
		${MKSYMTBL_PROGRAM} ${top_srcdir}/util/mksymtbl.pl \
		-o $@-symtbl.c $@tmp1 || exit 1; \
		$(MAKE) $@-symtbl.o || exit 1; \
		${LIBTOOL_MODE_LINK} ${PURIFY} ${CC} ${ALL_CFLAGS} ${LDFLAGS} \
		-o $@tmp2 $${BASEOBJS} $@-symtbl.o $${LIBS0} ${NOSYMLIBS}; \
		${MKSYMTBL_PROGRAM} ${top_srcdir}/util/mksymtbl.pl \
		-o $@-symtbl2.c $@tmp2; \
		count=0; \
		until diff $@-symtbl.c $@-symtbl2.c > /dev/null ; \
		do \
			count=`expr $$count + 1` ; \
			test $$count = 42 && exit 1 ; \
			rm -f $@-symtbl.c $@-symtbl.o; \
			${MKSYMTBL_PROGRAM} ${top_srcdir}/util/mksymtbl.pl \
			-o $@-symtbl.c $@tmp2 || exit 1; \
			$(MAKE) $@-symtbl.o || exit 1; \
			${LIBTOOL_MODE_LINK} ${PURIFY} ${CC} ${ALL_CFLAGS} \
			${LDFLAGS} -o $@tmp2 $${BASEOBJS} $@-symtbl.o \
			$${LIBS0} ${NOSYMLIBS}; \
			${MKSYMTBL_PROGRAM} ${top_srcdir}/util/mksymtbl.pl \
			-o $@-symtbl2.c $@tmp2; \
		done ; \
		mv $@tmp2 $@; \
		rm -f $@tmp0 $@tmp1 $@tmp2 $@-symtbl2.c; \
	fi

cleandir: distclean
superclean: maintainer-clean

clean distclean maintainer-clean::
	rm -f *.o *.o *.lo *.la core *.core *-symtbl.c *tmp0 *tmp1 *tmp2
	rm -rf .depend .libs

distclean maintainer-clean::
	rm -f Makefile

depend:
	@for i in ${ALL_SUBDIRS} ${ALL_TESTDIRS}; do \
		if [ "$$i" != "nulldir" -a -d $$i ]; then \
			echo "making depend in `pwd`/$$i"; \
			(cd $$i; ${MAKE} ${MAKEDEFS} DESTDIR="${DESTDIR}" $@) || exit 1; \
		fi; \
	done
	@if [ X"${srcdir}" != X. ] ; then \
		if [ X"${SRCS}" != X -a X"${PSRCS}" != X ] ; then \
			echo ${MKDEP} -vpath ${srcdir} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${MKDEP} -vpath ${srcdir} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			echo ${MKDEP} -vpath ${srcdir} -ap ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${MKDEP} -vpath ${srcdir} -ap ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${DEPENDEXTRA} \
		elif [ X"${SRCS}" != X ] ; then \
			echo ${MKDEP} -vpath ${srcdir} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${MKDEP} -vpath ${srcdir} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${DEPENDEXTRA} \
		elif [ X"${PSRCS}" != X ] ; then \
			echo ${MKDEP} -vpath ${srcdir} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${MKDEP} -vpath ${srcdir} -p ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${DEPENDEXTRA} \
		fi \
	else \
		if [ X"${SRCS}" != X -a X"${PSRCS}" != X ] ; then \
			echo ${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			echo ${MKDEP} -ap ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${MKDEP} -ap ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${DEPENDEXTRA} \
		elif [ X"${SRCS}" != X ] ; then \
			echo ${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${SRCS}; \
			${DEPENDEXTRA} \
		elif [ X"${PSRCS}" != X ] ; then \
			echo ${MKDEP} ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${MKDEP} -p ${ALL_CPPFLAGS} ${ALL_CFLAGS} ${PSRCS}; \
			${DEPENDEXTRA} \
		fi \
	fi

FORCE:

###
### Libraries
###

AR =		/usr/bin/ar
ARFLAGS =	cruv
RANLIB =	ranlib

###
### Installation
###

INSTALL =		/usr/bin/install -c
INSTALL_PROGRAM =	${INSTALL}
LINK_PROGRAM =		ln -s
INSTALL_SCRIPT =	${INSTALL}
INSTALL_DATA =		${INSTALL} -m 644
INSTALL_LIBRARY =	${INSTALL_DATA}

###
### Programs used when generating documentation.  It's ok for these
### not to exist when not generating documentation.
###

XSLTPROC =		/usr/pkg/bin/xsltproc --novalid --xinclude --nonet
XMLLINT =		/usr/pkg/bin/xmllint
PERL =			/usr/pkg/bin/perl
LATEX =			/usr/pkg/bin/latex
PDFLATEX =		/usr/pkg/bin/pdflatex
DBLATEX =		dblatex
W3M =			/usr/pkg/bin/w3m
PANDOC =		pandoc

###
### Script language program used to create internal symbol tables
###
MKSYMTBL_PROGRAM =	/usr/pkg/bin/perl

###
### Switch to create internal symbol table selectively
###
ALWAYS_MAKE_SYMTABLE =	

###
### DocBook -> HTML
### DocBook -> man page
###

.SUFFIXES: .docbook .html .1 .2 .3 .4 .5 .6 .7 .8

.docbook.html:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-docbook-html.xsl $<

.docbook.1:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.2:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.3:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.4:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.5:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.6:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.7:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<

.docbook.8:
	${XSLTPROC} -o $@ ${top_srcdir}/doc/xsl/isc-manpage.xsl $<


pkcs11-list: pkcs11-list.o ${DEPLIBS}
	${LIBTOOL_MODE_LINK} ${PURIFY} ${CC} ${ALL_CFLAGS} ${LDFLAGS} \
		-o $@ pkcs11-list.o ${LIBS}

pkcs11-destroy: pkcs11-destroy.o ${DEPLIBS}
	${LIBTOOL_MODE_LINK} ${PURIFY} ${CC} ${ALL_CFLAGS} ${LDFLAGS} \
		-o $@ pkcs11-destroy.o ${LIBS}

pkcs11-keygen: pkcs11-keygen.o ${DEPLIBS}
	${LIBTOOL_MODE_LINK} ${PURIFY} ${CC} ${ALL_CFLAGS} ${LDFLAGS} \
		-o $@ pkcs11-keygen.o ${LIBS}

pkcs11-tokens: pkcs11-tokens.o ${DEPLIBS}
	${LIBTOOL_MODE_LINK} ${PURIFY} ${CC} ${ALL_CFLAGS} ${LDFLAGS} \
		-o $@ pkcs11-tokens.o ${LIBS}

doc man:: ${MANOBJS}

docclean manclean maintainer-clean::
	rm -f ${MANOBJS}

installdirs:
	$(SHELL) ${top_srcdir}/mkinstalldirs ${DESTDIR}${sbindir}
	$(SHELL) ${top_srcdir}/mkinstalldirs ${DESTDIR}${mandir}/man8

install:: ${TARGETS} installdirs
	${LIBTOOL_MODE_INSTALL} ${INSTALL_PROGRAM} pkcs11-list \
		${DESTDIR}${sbindir}
	${LIBTOOL_MODE_INSTALL} ${INSTALL_PROGRAM} pkcs11-destroy \
		${DESTDIR}${sbindir}
	${LIBTOOL_MODE_INSTALL} ${INSTALL_PROGRAM} pkcs11-keygen \
		${DESTDIR}${sbindir}
	${LIBTOOL_MODE_INSTALL} ${INSTALL_PROGRAM} pkcs11-tokens \
		${DESTDIR}${sbindir}
	${INSTALL_DATA} ${srcdir}/pkcs11-list.8 ${DESTDIR}${mandir}/man8
	${INSTALL_DATA} ${srcdir}/pkcs11-destroy.8 ${DESTDIR}${mandir}/man8
	${INSTALL_DATA} ${srcdir}/pkcs11-keygen.8 ${DESTDIR}${mandir}/man8
	${INSTALL_DATA} ${srcdir}/pkcs11-tokens.8 ${DESTDIR}${mandir}/man8

uninstall::
	rm -f ${DESTDIR}${mandir}/man8/pkcs11-tokens.8
	rm -f ${DESTDIR}${mandir}/man8/pkcs11-keygen.8
	rm -f ${DESTDIR}${mandir}/man8/pkcs11-destroy.8
	rm -f ${DESTDIR}${mandir}/man8/pkcs11-list.8
	${LIBTOOL_MODE_UNINSTALL} rm -f /
		${DESTDIR}${sbindir}/pkcs11-tokens
	${LIBTOOL_MODE_UNINSTALL} rm -f /
		${DESTDIR}${sbindir}/pkcs11-keygen
	${LIBTOOL_MODE_UNINSTALL} rm -f /
		${DESTDIR}${sbindir}/pkcs11-destroy
	${LIBTOOL_MODE_UNINSTALL} rm -f /
		${DESTDIR}${sbindir}/pkcs11-list

clean distclean::
	rm -f ${OBJS} ${TARGETS}
