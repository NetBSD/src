#	$NetBSD: Makefile,v 1.11 1997/10/12 12:54:20 mycroft Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	su
DPADD+=	${LIBCRYPT}
LDADD+=	-lcrypt
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

# Uncomment the following line to change the group that may su root to "sugroup"
#
#CPPFLAGS+=-DSUGROUP=\"sugroup\"

.include <bsd.prog.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif

.ifdef KERBEROS
.ifdef AFS
DPADD+=	${LIBKAFS}
LDADD+=	-lkafs
.endif
DPADD+= ${LIBKRB} ${LIBDES}
LDADD+= -lkrb -ldes
CPPFLAGS+=-DKERBEROS
.endif
