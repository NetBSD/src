#	$NetBSD: Makefile.inc,v 1.2 2013/09/29 12:11:59 joerg Exp $

LIBISPRIVATE=	yes

.if !empty(LIB:Mlldb*)
CPPFLAGS+=	-I${LLDB_SRCDIR}/source
.endif

.include "${.PARSEDIR}/../Makefile.inc"
