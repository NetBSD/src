#	$NetBSD: Makefile,v 1.32 2003/05/26 09:34:55 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL}
LDADD=	-lutil
CPPFLAGS+=	-DMFS_DEV_IF_NO_CONSOLE -DSUPPORT_UTMP -DSUPPORT_UTMPX

.ifdef	SMALLPROG
CPPFLAGS+=	-DLETS_GET_SMALL
.else
CPPFLAGS+=	-DALTSHELL -DSECURE
DPADD+=		${LIBCRYPT}
LDADD+=		-lcrypt
.endif

.if defined(RESCUEDIR)
CPPFLAGS+= -DRESCUEDIR=\"${RESCUEDIR}\"
.endif

.include <bsd.prog.mk>
