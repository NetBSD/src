# $NetBSD: Makefile,v 1.1 1999/07/01 19:59:59 itojun Exp $

PROG=	ping6
MAN=	ping6.8

BINOWN  = root
BINGRP  = bin
BINMODE = 4555

CFLAGS+= -DINET6
#CFLAGS+= -DIPSEC
#LDADD+=	-L${.OBJDIR}/../libipsec -L${.OBJDIR}/../libipsec/obj \
#	-L/usr/local/v6/lib -lipsec
#DPADD+= ${.OBJDIR}/../libipsec/libipsec.a \
#	${.OBJDIR}/../libipsec/obj/libipsec.a \
#	/usr/local/v6/lib/libipsec.a

.include <bsd.prog.mk>
