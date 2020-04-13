#	$NetBSD: Makefile,v 1.10.46.1 2020/04/13 08:05:44 martin Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/28/95

PROG=	msgs
DPADD=	${LIBTERMINFO}
LDADD=	-lterminfo

COPTS.msgs.c+=	${GCC_NO_FORMAT_TRUNCATION} ${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
