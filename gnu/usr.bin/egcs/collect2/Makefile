#	$NetBSD: Makefile,v 1.1 1998/08/16 23:04:18 tv Exp $

PROG=		collect2
BINDIR=		/usr/libexec
SRCS=		collect2.c tlink.c hash.c cplus-dem.c underscore.c \
		version.c choose-temp.c mkstemp.c obstack.c
CPPFLAGS+=	-DTARGET_MACHINE=\"${target_alias}\"

NOMAN=

.include <bsd.prog.mk>
