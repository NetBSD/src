#	$NetBSD: Makefile,v 1.23 2000/06/20 06:00:35 thorpej Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

DIST=	${.CURDIR}/../../crypto/dist
.PATH:	${DIST}/heimdal/appl/telnet/telnetd .DOTLAST

# XXX XXX XXX
WARNS=0

MAN=	telnetd.8

CPPFLAGS+=-DINET6

PROG=	telnetd
CPPFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DUSE_TERMIO -DDIAGNOSTICS -DOPENPTY_PTY
CPPFLAGS+=-DOLD_ENVIRON -DENV_HACK -DSECURELOGIN
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBUTIL} ${LIBTERMCAP} ${LIBTELNET}
LDADD+=	-lutil -ltermcap -ltelnet

CPPFLAGS+= -I${DIST}/heimdal/appl/telnet
CPPFLAGS+= -I${.CURDIR}/../../include/heimdal
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
CPPFLAGS+= -I${DESTDIR}/usr/include/krb5
CPPFLAGS+= -I${DESTDIR}/usr/include/openssl
CPPFLAGS+= -DHAVE_CONFIG_H

LDADD+=	-lkrb5 -lkrb -lcrypto -lasn1 -lcom_err -lroken

CPPFLAGS+=-I${.CURDIR}

.include <bsd.prog.mk>
