#	$NetBSD: Makefile,v 1.25 2000/01/13 06:17:56 mjl Exp $
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
CFLAGS+=	-DLOGIN_CAP

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
