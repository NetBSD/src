#	$NetBSD: Makefile,v 1.11 2019/10/13 07:28:16 mrg Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/28/95

PROG=	msgs
DPADD=	${LIBTERMINFO}
LDADD=	-lterminfo

COPTS.msgs.c+=	${GCC_NO_FORMAT_TRUNCATION} ${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
