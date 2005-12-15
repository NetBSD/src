#	$NetBSD: Makefile,v 1.43 2005/12/15 14:03:34 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

PROG=	su
BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DLOGIN_CAP

.if ${USE_PAM} != "no"

CPPFLAGS+=-DUSE_PAM -DPAM_DEBUG
# XXX: Need libcrypt here, because libcrypto defines it too.
DPADD+=	${LIBPAM} ${LIBCRYPT} ${LIBUTIL} ${PAM_STATIC_DPADD}
LDADD+= -lpam -lcrypt -lutil ${PAM_STATIC_LDADD}
SRCS=su_pam.c

.else

SRCS=su.c

DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+=	-lcrypt -lutil

# Uncomment the following line to change the group that may su root to "sugroup"
#
#CPPFLAGS+=-DSU_GROUP=\"sugroup\"

# Uncomment the following line to make su
# treat group wheel (SUGROUP) and/or ROOTAUTH as an indirect
# list of groups.
#CPPFLAGS+=-DSU_INDIRECT_GROUP

.if (${USE_KERBEROS} != "no")
.ifdef AFS
DPADD+= ${LIBKAFS}
LDADD+= -lkafs
.endif

CPPFLAGS+=-DKERBEROS5 -I${DESTDIR}/usr/include/krb5
DPADD+=	${LIBKRB5} ${LIBASN1}
LDADD+= -lkrb5 -lasn1

.if (${USE_KERBEROS4} != "no")
CPPFLAGS+=-DKERBEROS -I${DESTDIR}/usr/include/kerberosIV
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+= -lkrb -ldes
.endif

DPADD+=	${LIBCRYPTO} ${LIBROKEN} ${LIBCOM_ERR}
LDADD+=	-lcrypto -lroken -lcom_err
.endif


.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif

.ifdef SU_ROOTAUTH
CPPFLAGS+=-DSU_ROOTAUTH=\"${SU_ROOTAUTH}\"
.endif

.endif

.include <bsd.prog.mk>
