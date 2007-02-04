#	$NetBSD: Makefile,v 1.37 2007/02/04 20:54:02 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

WARNS=	4
PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL}
LDADD=	-lutil
CPPFLAGS+=	-DMFS_DEV_IF_NO_CONSOLE -DSUPPORT_UTMP -DSUPPORT_UTMPX

.ifdef	SMALLPROG
CPPFLAGS+=	-DLETS_GET_SMALL
.else
CPPFLAGS+=	-DALTSHELL -DSECURE -DCHROOT
DPADD+=		${LIBCRYPT}
LDADD+=		-lcrypt
.endif

.include <bsd.prog.mk>
