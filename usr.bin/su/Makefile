#	$NetBSD: Makefile,v 1.17 2000/01/14 02:39:14 mjl Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	su
DPADD+=	${LIBCRYPT}
LDADD+=	-lcrypt -lutil
BINOWN=	root
BINMODE=4555
INSTALLFLAGS=-fschg
CPPFLAGS+=-DLOGIN_CAP

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
