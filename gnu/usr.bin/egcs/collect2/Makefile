#	$NetBSD: Makefile,v 1.2 1998/08/17 15:46:33 tv Exp $

PROG=		collect2
BINDIR=		/usr/libexec
SRCS=		collect2.c tlink.c hash.c cplus-dem.c underscore.c \
		version.c choose-temp.c mkstemp.c obstack.c
CPPFLAGS+=	-DTARGET_MACHINE=\"${MACHINE_GNU_ARCH}-netbsd\"

NOMAN=

.include <bsd.prog.mk>
