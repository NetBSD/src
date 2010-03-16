# $NetBSD: Makefile,v 1.2 2010/03/16 00:22:52 agc Exp $

PROG=hkpd
SRCS=hkpd.c main.c
CPPFLAGS+=-g # -DHAVE_CONFIG_H=1
LDFLAGS+=-g
LDADD+= -lnetpgp
MAN=hkpd.8
WARNS=0 # anything over 0 will fail at the link stage with IDEA errors

.include <bsd.prog.mk>
