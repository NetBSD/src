#	$NetBSD: Makefile,v 1.15.4.1 2002/02/09 16:56:48 he Exp $

PROG=	ipnat
SRCS=	ipnat.c kmem.c natparse.c common.c printnat.c
MAN=	ipnat.8 ipnat.4 ipnat.5
MLINKS=	ipnat.5 ipnat.conf.5
DPADD=	${LIBKVM}
LDADD=	-lkvm

.include <bsd.prog.mk>
