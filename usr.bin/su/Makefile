#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93
#	$Id: Makefile,v 1.8 1997/01/09 11:43:05 tls Exp $

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
