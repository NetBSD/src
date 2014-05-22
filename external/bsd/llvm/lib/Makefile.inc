#	$NetBSD: Makefile.inc,v 1.1.6.1 2014/05/22 16:14:40 yamt Exp $

LIBISPRIVATE=	yes

.if !empty(LIB:Mlldb*)
CPPFLAGS+=	-I${LLDB_SRCDIR}/source
CPPFLAGS+=	-I${LLDB_SRCDIR}/source/Plugins/Process/POSIX
.endif

.include "${.PARSEDIR}/../Makefile.inc"
