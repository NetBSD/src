#	@(#)Makefile	6.4 (Berkeley) 2/21/91
PROG=		cpp
BINDIR=		/usr/libexec
SRCS=		cccp.c cexp.y version.c
MAN1=		cpp.0
CFLAGS+=	-I. -I$(.CURDIR) -I$(.CURDIR)/../common \
                -I$(.CURDIR)/../$(MACHINE) -I$(.CURDIR)/.. \
		-DGCC_INCLUDE_DIR=\"\" \
		-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"
YFLAGS=

MLINKS=	cpp.1 gcpp.1

.PATH:		$(.CURDIR)/../common

afterinstall:
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/usr.bin.cpp.sh $(DESTDIR)/usr/bin/cpp
	@/bin/rm -rf $(DESTDIR)/usr/bin/gcpp
	@ln $(DESTDIR)/usr/bin/cpp $(DESTDIR)/usr/bin/gcpp
	@echo $(DESTDIR)/usr/bin/gcpp -\> $(DESTDIR)/usr/bin/cpp

.include <bsd.prog.mk>
