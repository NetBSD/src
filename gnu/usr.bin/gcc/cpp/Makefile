#	from: @(#)Makefile	6.4 (Berkeley) 2/21/91
#	$NetBSD: Makefile,v 1.1.1.1 1995/12/01 17:59:57 phil Exp $

PROG=		cpp
BINDIR=		/usr/libexec
SRCS=	cccp.c cexp.c obstack.c version.c 
CFLAGS+=	-I. -I$(.CURDIR) -I$(.CURDIR)/../common -I$(.CURDIR)/../arch \
                -I$(.CURDIR)/../arch/$(MACHINE_ARCH) \
		-DGCC_INCLUDE_DIR=\"\" \
		-DTOOL_INCLUDE_DIR=\"\" \
		-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"
LDADD+=		-lgnumalloc
DPADD+=		/usr/lib/libgnumalloc.a
YFLAGS=

MLINKS=	cpp.1 cccp.1 cpp.1 gcpp.1

.PATH:		$(.CURDIR)/../common

afterinstall:
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/usr.bin.cpp.sh $(DESTDIR)/usr/bin/cpp
	@/bin/rm -rf $(DESTDIR)/usr/bin/gcpp
	@ln $(DESTDIR)/usr/bin/cpp $(DESTDIR)/usr/bin/gcpp
	@echo $(DESTDIR)/usr/bin/gcpp -\> $(DESTDIR)/usr/bin/cpp

.include <bsd.prog.mk>
