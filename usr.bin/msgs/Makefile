#	$NetBSD: Makefile,v 1.12 2023/06/03 09:09:17 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/28/95

PROG=	msgs
DPADD=	${LIBTERMINFO}
LDADD=	-lterminfo

COPTS.msgs.c+=	${CC_WNO_FORMAT_TRUNCATION} ${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
