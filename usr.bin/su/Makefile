#	$NetBSD: Makefile,v 1.13.2.1 2000/01/08 18:35:13 he Exp $
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

.ifdef SUROOTAUTH
CPPFLAGS+=-DROOTAUTH=\"${SUROOTAUTH}\"
.endif
