#	$NetBSD: Makefile,v 1.16 2002/01/24 08:24:40 martti Exp $

PROG=	ipnat
SRCS=	ipnat.c kmem.c natparse.c common.c printnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
MLINKS=	ipnat.5 ipnat.conf.5
DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
