#	$NetBSD: Makefile,v 1.48 2007/10/27 15:37:45 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

USE_FORT?= yes	# setuid
PROG=	su
BINOWN=	root
BINMODE=4555

.PATH.c: ${.CURDIR}/../newgrp
CPPFLAGS+=-I${.CURDIR}/../newgrp
CPPFLAGS+=-DLOGIN_CAP
CPPFLAGS+=-DALLOW_GROUP_CHANGE
CPPFLAGS+=-DALLOW_EMPTY_USER
CPPFLAGS+=-DGRUTIL_SETGROUPS_MAKESPACE

.if ${USE_PAM} != "no"

CPPFLAGS+=-DUSE_PAM
# XXX: Need libcrypt here, because libcrypto defines it too.
DPADD+=	${LIBPAM} ${LIBCRYPT} ${LIBUTIL} ${PAM_STATIC_DPADD}
LDADD+= -lpam -lcrypt -lutil ${PAM_STATIC_LDADD}
SRCS=su_pam.c grutil.c suutil.c

.else

SRCS=su.c grutil.c suutil.c

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
