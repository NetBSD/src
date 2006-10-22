#	@(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.9 2006/10/22 08:22:34 mrg Exp $

.include <bsd.own.mk>

PROG=	fmt
SRCS=	fmt.c head.c
CPPFLAGS+=	-DFMT_PROG
WARNS=	4

.PATH: ${NETBSDSRCDIR}/usr.bin/mail

.include <bsd.prog.mk>
