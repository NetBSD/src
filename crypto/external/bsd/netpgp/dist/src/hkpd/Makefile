# $NetBSD: Makefile,v 1.1 2010/03/01 07:41:56 agc Exp $

PROG=hkpd
SRCS=hkpd.c main.c
CPPFLAGS+=-g # -DHAVE_CONFIG_H=1
LDFLAGS+=-g
LDADD+= -lnetpgp
MKMAN=no
WARNS=0 # anything over 0 will fail at the link stage with IDEA errors

.include <bsd.prog.mk>
