#	$NetBSD: Makefile,v 1.26 2000/08/03 04:09:30 assar Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

.include <bsd.own.mk>

MAN=	telnetd.8

CPPFLAGS+=-DINET6

PROG=	telnetd
CPPFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DUSE_TERMIO -DDIAGNOSTICS -DOPENPTY_PTY
CPPFLAGS+=-DOLD_ENVIRON -DENV_HACK -DSECURELOGIN
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBUTIL} ${LIBTERMCAP} ${LIBTELNET}
LDADD+=	-lutil -ltermcap -ltelnet

CPPFLAGS+=-I${.CURDIR}/../../lib
CPPFLAGS+=-I${.CURDIR}

.if (${MKKERBEROS} != "no")
CPPFLAGS+=-DAUTHENTICATION -DENCRYPTION
CPPFLAGS+=-I${DESTDIR}/usr/include/krb5
CPPFLAGS+=-DKRB5

LDADD+=	-lkrb5 -lkrb -lcrypto -lasn1 -lcom_err -lvers -lroken
DPADD+=	${LIBKRB5} ${LIBKRB} ${LIBCRYPTO} ${LIBASN1} ${LIBCOM_ERR} ${LIBROKEN}
.endif

.include <bsd.prog.mk>
