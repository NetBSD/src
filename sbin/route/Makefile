#	$NetBSD: Makefile,v 1.13 1996/11/15 18:01:36 gwr Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
MAN=	route.8
SRCS=	route.c show.c keywords.c ccitt_addr.c
BINOWN=	root
BINMODE=4555

# CFLAGS+=-I.
# CLEANFILES+= keywords.c keywords.h
# keywords.c keywords.h : keywords.sh
# 	sh keywords.sh

route.o .depend lint tags: keywords.h

.include <bsd.prog.mk>
