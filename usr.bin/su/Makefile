#	$NetBSD: Makefile,v 1.13 1999/02/18 21:22:52 scottr Exp $
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
