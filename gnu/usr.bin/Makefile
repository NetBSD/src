#	$NetBSD: Makefile,v 1.22 1995/04/23 07:50:55 cgd Exp $

SUBDIR+=	bc cpio dc diff diff3 gawk grep
SUBDIR+=	gzip rcs sdiff send-pr sort tar

# some ports don't use the tool chain in /usr/src
.if (${MACHINE_ARCH} != "alpha")
SUBDIR+=	gas gcc2 gdb groff ld
.endif

# XXX alpha still needs the /usr/bin/cpp script.
.if (${MACHINE_ARCH} == "alpha")
afterinstall:
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
	    $(.CURDIR)/gcc2/cpp/usr.bin.cpp.sh $(DESTDIR)/usr/bin/cpp
.endif

.include <bsd.subdir.mk>
