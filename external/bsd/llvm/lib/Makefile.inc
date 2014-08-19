#	$NetBSD: Makefile.inc,v 1.1.12.1 2014/08/19 23:51:23 tls Exp $

LIBISPRIVATE=	yes

.if !empty(LIB:Mlldb*)
CPPFLAGS+=	-I${LLDB_SRCDIR}/source
CPPFLAGS+=	-I${LLDB_SRCDIR}/source/Plugins/Process/POSIX
.endif

.include "${.PARSEDIR}/../Makefile.inc"
