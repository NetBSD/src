#	$NetBSD: Makefile,v 1.6 2006/05/11 23:16:30 mrg Exp $

.include <bsd.own.mk>

PROG=	chrtbl
MAN=	chrtbl.8

.PATH:	${NETBSDSRCDIR}/lib/libc/locale

CPPFLAGS+=-I${NETBSDSRCDIR}/lib/libc/locale

# ctypeio.c pulled in from ../../lib/libc/locale
SRCS=	chrtbl.c ctypeio.c

.include <bsd.prog.mk>

.if ${HAVE_GCC} == 4
COPTS.chrtbl.c+=	-Wno-pointer-sign
.endif
