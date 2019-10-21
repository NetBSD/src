#	$NetBSD: Makefile,v 1.8 2019/10/21 02:36:48 jhigh Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	pwhash

.if ( defined(MKARGON2) && ${MKARGON2} != "no" )
CPPFLAGS+=	-DHAVE_ARGON2
.endif

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
