#	$NetBSD: Makefile,v 1.44 1997/04/17 07:00:16 thorpej Exp $

SUBDIR+=	bc cpio dc diff diff3 gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar

.if	make(clean) || make(cleandir)
SUBDIR+=	gas gcc gdb ld
.elif	(${MACHINE_ARCH} == "i386") || \
	(${MACHINE_ARCH} == "m68k") || \
	(${MACHINE_ARCH} == "ns32k") || \
	(${MACHINE_ARCH} == "sparc") || \
	(${MACHINE_ARCH} == "vax")
SUBDIR+=        gas gcc gdb ld
.elif	(${MACHINE_ARCH} == "arm32")
SUBDIR+=	gcc ld
.elif	(${MACHINE_ARCH} == "mips") 
SUBDIR+=	gcc
.endif

# XXX alpha and powerpc still need the /usr/bin/cpp script.
.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "powerpc")
afterinstall:
	${INSTALL} -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
	    $(.CURDIR)/gcc/cpp/usr.bin.cpp.sh $(DESTDIR)/usr/bin/cpp
.endif

.include <bsd.subdir.mk>
