#	$NetBSD: Makefile,v 1.34 2003/07/14 15:55:53 itojun Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

.include <bsd.own.mk>

MAN=	telnetd.8

PROG=	telnetd
CPPFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DDIAGNOSTICS
CPPFLAGS+=-DOLD_ENVIRON -DENV_HACK -DSECURELOGIN -DSUPPORT_UTMP -DSUPPORT_UTMPX
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBUTIL} ${LIBTERMCAP} ${LIBTELNET}
LDADD+=	-lutil -ltermcap -ltelnet

CPPFLAGS+=-I${NETBSDSRCDIR}/lib
CPPFLAGS+=-I${.CURDIR}

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DAUTHENTICATION -DENCRYPTION
CPPFLAGS+=-I${DESTDIR}/usr/include/krb5
CPPFLAGS+=-DKRB5

VERS!=	cd ${NETBSDSRCDIR}/lib/libvers && ${PRINTOBJDIR}

LDADD+=	-lkrb5 -lkrb -lcrypto -lasn1 -lcom_err -L${VERS} -lvers -lroken
DPADD+=	${LIBKRB5} ${LIBKRB} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>
