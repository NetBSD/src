#	$NetBSD: Makefile,v 1.16 1997/04/03 02:35:46 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	route
MAN=	route.8
SRCS=	route.c show.c keywords.c ccitt_addr.c

# The Makefile over in ../../distrib/utils/x_route
# would like keywords.[ch] to always exist here, so
# they are now checked in as sources.
#
# CFLAGS+=-I.
# CLEANFILES+= keywords.c keywords.h
# keywords.c keywords.h : keywords.sh
# 	sh keywords.sh

route.o .depend lint tags: keywords.h


.include <bsd.prog.mk>
