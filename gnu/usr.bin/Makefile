#	$NetBSD: Makefile,v 1.23 1995/06/20 09:55:18 cgd Exp $

SUBDIR+=	bc cpio dc diff diff3 gawk grep
SUBDIR+=	gzip rcs sdiff send-pr sort tar

# some ports don't use the tool chain in /usr/src
.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "mips")
SUBDIR+=	gas gcc2 gdb ld
.endif

# some ports can't build groff (because it uses g++)
.if (${MACHINE_ARCH} != "alpha")
SUBDIR+=	groff
.endif

# XXX alpha still needs the /usr/bin/cpp script.
.if (${MACHINE_ARCH} == "alpha")
afterinstall:
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
	    $(.CURDIR)/gcc2/cpp/usr.bin.cpp.sh $(DESTDIR)/usr/bin/cpp
.endif

.include <bsd.subdir.mk>
