#	$NetBSD: Makefile,v 1.8 2023/06/03 09:09:19 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	unifdef

SCRIPTS=unifdefall.sh

MLINKS=	unifdef.1 unifdefall.1

COPTS.unifdef.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
