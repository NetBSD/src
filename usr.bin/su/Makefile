#	from: @(#)Makefile	5.5 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.7 1996/10/12 23:54:38 christos Exp $

PROG=	su
BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>

.ifdef SKEY
CFLAGS+=-DSKEY
LDADD+= -lcrypt -lskey
DPADD+= ${LIBCRYPT} ${LIBSKEY}
.endif

.ifdef KERBEROS
CFLAGS+=-DKERBEROS
LDADD+= -lkrb -ldes
DPADD+= ${LIBKRB} ${LIBDES}
.endif
