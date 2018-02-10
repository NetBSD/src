#	$NetBSD: Makefile,v 1.52 2018/02/10 19:34:34 christos Exp $
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
LDADD+= -lkrb5 -lasn1 -lcom_err -lroken -lsqlite3
DPADD+=	${LIBKRB5} ${LIBASN1} ${LIBCOM_ERR} ${LIBROKEN} ${LIBSQLITE3}
.endif

CPPFLAGS+=-DAUTHENTICATION -DENCRYPTION
LDADD+=-ldes -lcrypto -lcrypt
DPADD+=${LIBDES} ${LIBCRYPTO} ${LIBCRYPT}

.if (${USE_PAM} != "no")
LDADD+= -lpam ${PAM_STATIC_LDADD}
DPADD+=	${LIBPAM} ${PAM_STATIC_DPADD}
.endif

.include <bsd.prog.mk>
