#	$NetBSD: Makefile,v 1.51 2018/02/25 00:16:49 mrg Exp $
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

.else	# USE_PAM == no

SRCS=su.c grutil.c suutil.c

LDADD+=	-lcrypt -lutil
DPADD+=	${LIBCRYPT} ${LIBUTIL}

# Uncomment the following line to change the group that may su root to "sugroup"
#
#CPPFLAGS+=-DSU_GROUP=\"sugroup\"

# Uncomment the following line to make su
# treat group wheel (SUGROUP) and/or ROOTAUTH as an indirect
# list of groups.
#CPPFLAGS+=-DSU_INDIRECT_GROUP

.if (${USE_KERBEROS} != "no")
.ifdef AFS
LDADD+= -lkafs
DPADD+= ${LIBKAFS}
.endif

CPPFLAGS+=-DKERBEROS5
LDADD+=	${LIBKRB5_LDADD}
DPADD+=	${LIBKRB5_DPADD}
.endif

.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
LDADD+= -lskey
DPADD+= ${LIBSKEY}
.endif

.ifdef SU_ROOTAUTH
CPPFLAGS+=-DSU_ROOTAUTH=\"${SU_ROOTAUTH}\"
.endif

.endif	# USE_PAM == no

.include <bsd.prog.mk>
