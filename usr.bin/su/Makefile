#	$NetBSD: Makefile,v 1.26 2001/01/11 00:54:57 sjg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=	su
DPADD+=	${LIBCRYPT}
LDADD+=	-lcrypt -lutil
BINOWN=	root
BINMODE=4555
CPPFLAGS+=-DLOGIN_CAP

# Uncomment the following line to change the group that may su root to "sugroup"
#
#CPPFLAGS+=-DSUGROUP=\"sugroup\"

# Uncomment the following line to make su
# treat group wheel (SUGROUP) and/or ROOTAUTH as an indirect
# list of groups.
#CPPFLAGS+=-DSU_INDIRECT_GROUP

.if (${MKKERBEROS} != "no")
.ifdef AFS
DPADD+= ${LIBKAFS}
LDADD+= -lkafs
.endif

CPPFLAGS+=-DKERBEROS5 -I${DESTDIR}/usr/include/krb5
LDADD+= -lkrb5 -lasn1

CPPFLAGS+=-DKERBEROS -I${DESTDIR}/usr/include/kerberosIV
LDADD+= -lkrb

LDADD+=	-lcrypto -lroken -lcom_err
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
