#	$NetBSD: Makefile,v 1.18.2.2 2000/06/24 06:59:36 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

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

.if (${MKKERBEROS} != "no")
.ifdef AFS
DPADD+= ${LIBKAFS}
LDADD+= -lkafs
.endif

CPPFLAGS+=-DKERBEROS -I${DESTDIR}/usr/include/kerberosIV
DPADD+= ${LIBKRB} ${LIBDES} ${LIBCOM_ERR} ${LIBROKEN}
LDADD+= -lkrb -ldes -lcom_err -lroken
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
