#	$NetBSD: Makefile,v 1.4 2001/12/12 01:49:03 tv Exp $

PROG=		collect2
BINDIR=		/usr/libexec
SRCS=		collect2.c tlink.c hash.c cplus-dem.c underscore.c \
		version.c choose-temp.c mkstemp.c obstack.c
CPPFLAGS+=	-DTARGET_MACHINE=\"${MACHINE_GNU_ARCH}-netbsd\"

NOMAN=		# defined

.include <bsd.prog.mk>
