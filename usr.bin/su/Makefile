#	$NetBSD: Makefile,v 1.16 1999/07/20 09:35:21 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	su
DPADD+=	${LIBCRYPT}
LDADD+=	-lcrypt
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg

# Uncomment the following line to change the group that may su root to "sugroup"
#
#CPPFLAGS+=-DSUGROUP=\"sugroup\"

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/usr.bin/su/Makefile.frag"
.endif

.include <bsd.prog.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif

.ifdef SUROOTAUTH
CPPFLAGS+=-DROOTAUTH=\"${SUROOTAUTH}\"
.endif
