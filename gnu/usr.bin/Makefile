#	$NetBSD: Makefile,v 1.30 1995/12/31 04:00:00 thorpej Exp $

SUBDIR+=	bc cpio dc diff diff3 gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar

.if	(${MACHINE_ARCH} == "i386") || (${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "vax") || (${MACHINE} == "amiga") || \
	(${MACHINE} == "hp300")
SUBDIR+=        gas gcc gdb ld
.else if	(${MACHINE_ARCH} == "m68k") || (${MACHINE_ARCH} == "sparc")
SUBDIR+=	gas gcc2 gdb ld
.endif


# XXX alpha still needs the /usr/bin/cpp script.
.if (${MACHINE_ARCH} == "alpha")
afterinstall:
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
	    $(.CURDIR)/gcc2/cpp/usr.bin.cpp.sh $(DESTDIR)/usr/bin/cpp
.endif

.include <bsd.subdir.mk>
