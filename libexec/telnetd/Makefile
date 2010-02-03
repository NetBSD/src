#	$NetBSD: Makefile,v 1.46 2010/02/03 15:34:43 roy Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

WARNS?=	2		# XXX: const issues in sys_term.c

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
CPPFLAGS+=-I${DESTDIR}/usr/include/krb5
CPPFLAGS+=-DKRB5 -DAUTHENTICATION -DENCRYPTION
LDADD+= -lkrb5 -lasn1 -lcrypto -lcrypt
DPADD+=	${LIBKRB5} ${LIBASN1} ${LIBCRYPTO} ${LIBCRYPT}

.if (${USE_PAM} != "no")
LDADD+= -lpam ${PAM_STATIC_LDADD}
DPADD+=	${LIBPAM} ${PAM_STATIC_DPADD}
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	 -ldes -lcom_err -L${VERS} -lvers -lroken
DPADD+=	${LIBDES} ${LIBCOM_ERR} ${LIBROKEN}

.endif

.include <bsd.prog.mk>
