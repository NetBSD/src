#	$NetBSD: Makefile,v 1.5 1998/10/10 07:38:23 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	chflags
SRCS=	chflags.c stat_flags.c
.PATH:	${.CURDIR}/../../bin/ls
CPPFLAGS+=	-I${.CURDIR}/../../bin/ls

.include <bsd.prog.mk>
