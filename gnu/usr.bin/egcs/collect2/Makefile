#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:25 lukem Exp $

PROG=		collect2
BINDIR=		/usr/libexec
SRCS=		collect2.c tlink.c hash.c cplus-dem.c underscore.c \
		version.c choose-temp.c mkstemp.c obstack.c
CPPFLAGS+=	-DTARGET_MACHINE=\"${MACHINE_GNU_ARCH}-netbsd\"

MKMAN=		no

.include <bsd.prog.mk>
