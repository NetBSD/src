#	$NetBSD: Makefile,v 1.43 1997/04/09 06:23:44 mikel Exp $

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
.elif	(${MACHINE_ARCH} == "powerpc")
SUBDIR+=	gas ld
.elif	(${MACHINE_ARCH} == "arm32")
SUBDIR+=	gcc ld
.elif	(${MACHINE_ARCH} == "mips") 
SUBDIR+=	gcc
.endif

# XXX alpha still needs the /usr/bin/cpp script.
.if (${MACHINE_ARCH} == "alpha")
afterinstall:
	${INSTALL} -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
	    $(.CURDIR)/gcc/cpp/usr.bin.cpp.sh $(DESTDIR)/usr/bin/cpp
.endif

.include <bsd.subdir.mk>
