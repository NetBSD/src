#	$NetBSD: Makefile,v 1.10 1997/07/02 05:42:11 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	su
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

# Uncomment the following line to change the group that may su root to "sugroup"
#
#CFLAGS+=-DSUGROUP=\"sugroup\"

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
