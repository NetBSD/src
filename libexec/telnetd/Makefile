#	$NetBSD: Makefile,v 1.53 2018/02/25 00:16:48 mrg Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

WARNS?=	4		# XXX: const issues in sys_term.c

.include <bsd.own.mk>

MAN=	telnetd.8

PROG=	telnetd
CPPFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DDIAGNOSTICS
CPPFLAGS+=-DOLD_ENVIRON -DENV_HACK -DSECURELOGIN -DSUPPORT_UTMP -DSUPPORT_UTMPX
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBTELNETDIR}/libtelnet.a ${LIBTERMINFO} ${LIBUTIL}
LDADD+=	${LIBTELNETDIR}/libtelnet.a -lterminfo -lutil

CPPFLAGS+=-I${NETBSDSRCDIR}/lib
CPPFLAGS+=-I${.CURDIR}

LIBTELNETDIR!=	cd ${.CURDIR}/../../lib/libtelnet; ${PRINTOBJDIR}

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DKRB5
LDADD+= ${LIBKRB5_LDADD}
DPADD+= ${LIBKRB5_DPADD}
.endif

CPPFLAGS+=-DAUTHENTICATION -DENCRYPTION
LDADD+=-ldes -lcrypto -lcrypt
DPADD+=${LIBDES} ${LIBCRYPTO} ${LIBCRYPT}

.if (${USE_PAM} != "no")
LDADD+= -lpam ${PAM_STATIC_LDADD}
DPADD+= ${LIBPAM} ${PAM_STATIC_DPADD}
.endif

.include <bsd.prog.mk>
