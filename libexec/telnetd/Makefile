#	$NetBSD: Makefile,v 1.21 1999/07/12 22:06:03 thorpej Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93

SRCTOP=	../..
.include <bsd.crypto.mk>

MAN=	telnetd.8

CPPFLAGS+=-DINET6

PROG=	telnetd
CPPFLAGS+=-DLINEMODE -DKLUDGELINEMODE -DUSE_TERMIO -DDIAGNOSTICS -DOPENPTY_PTY
CPPFLAGS+=-DOLD_ENVIRON -DENV_HACK -DSECURELOGIN
SRCS=	authenc.c global.c slc.c state.c sys_term.c telnetd.c \
	termstat.c utility.c
DPADD=	${LIBUTIL} ${LIBTERMCAP} ${LIBTELNET}
LDADD+=	-lutil -ltermcap -ltelnet

.if defined(CRYPTOPATH)
.include "${CRYPTOPATH}/libexec/telnetd/Makefile.frag"
.endif

CPPFLAGS+=-I${.CURDIR}

.include <bsd.prog.mk>
