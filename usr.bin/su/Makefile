#	$NetBSD: Makefile,v 1.9 1997/01/09 20:21:40 tls Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	su
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

.include <bsd.prog.mk>

.ifdef SKEY
CFLAGS+=-DSKEY
LDADD+= -lcrypt -lskey
DPADD+= ${LIBCRYPT} ${LIBSKEY}
.endif

.ifdef KERBEROS
.ifdef AFS
LDADD+=	-lkafs
DPADD+=	${LIBKAFS}
.endif
CFLAGS+=-DKERBEROS
LDADD+= -lkrb -ldes
DPADD+= ${LIBKRB} ${LIBDES}
.endif
