# $NetBSD: Makefile,v 1.14 2001/12/12 00:05:10 tv Exp $
# Build a "small init" (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../sbin/init

PROG=		init
NOMAN=		# defined

CPPFLAGS+=	-DLETS_GET_SMALL -I${SRCDIR}

# Need -lutil for: logout, logwtmp, login_tty
DPADD=		${LIBUTIL}
LDADD=		-lutil

.include <bsd.prog.mk>

.PATH:  ${SRCDIR}
.NOPATH: ${PROG}
