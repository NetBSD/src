#	$NetBSD: Makefile,v 1.24 1999/07/20 09:35:20 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	login
SRCS=	login.c
DPADD+=	${LIBUTIL} ${LIBCRYPT}
LDADD+=	-lutil -lcrypt
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

.include <bsd.own.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+=	-lskey
.endif

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/usr.bin/login/Makefile.frag"
.endif

.include <bsd.prog.mk>
