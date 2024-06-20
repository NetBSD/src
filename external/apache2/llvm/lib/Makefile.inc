#	$NetBSD: Makefile.inc,v 1.2 2024/06/20 04:13:35 mrg Exp $

LIBISPRIVATE=	yes

.include "${.PARSEDIR}/../Makefile.inc"

CXXFLAGS+=	${CC_WNO_MISSING_TEMPLATE_KEYWORD}
