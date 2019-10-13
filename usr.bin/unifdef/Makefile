#	$NetBSD: Makefile,v 1.7 2019/10/13 07:28:17 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	unifdef

SCRIPTS=unifdefall.sh

MLINKS=	unifdef.1 unifdefall.1

COPTS.unifdef.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
