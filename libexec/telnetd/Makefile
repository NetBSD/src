#	$NetBSD: Makefile,v 1.40 2004/12/22 01:25:05 lha Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

.include <bsd.own.mk>

MAN=	telnetd.8

PROG=	telnetd
CPPFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DDIAGNOSTICS
CPPFLAGS+=-DOLD_ENVIRON -DENV_HACK -DSECURELOGIN -DSUPPORT_UTMP -DSUPPORT_UTMPX
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBUTIL} ${LIBTERMCAP} ${LIBTELNETDIR}/libtelnet.a
LDADD+=	-lutil -ltermcap ${LIBTELNETDIR}/libtelnet.a

CPPFLAGS+=-I${NETBSDSRCDIR}/lib
CPPFLAGS+=-I${.CURDIR}

LIBTELNETDIR!=	cd ${.CURDIR}/../../lib/libtelnet; ${PRINTOBJDIR}

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-I${DESTDIR}/usr/include/krb5
CPPFLAGS+=-DKRB5 -DAUTHENTICATION -DENCRYPTION
LDADD+= -lkrb5 -lasn1 -lcrypto
DPADD+=	${LIBKRB5} ${LIBASN1} ${LIBCRYPTO}

.if (${USE_KERBEROS4} != "no")
CPPFLAGS+=-DKRB4
LDADD+= -lkrb
DPADD+=	${LIBKRB}
.endif

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	 -ldes -lcom_err -L${VERS} -lvers -lroken
DPADD+=	${LIBDES} ${LIBCOM_ERR} ${LIBROKEN}

.endif

.include <bsd.prog.mk>
