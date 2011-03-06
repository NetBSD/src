#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:52 bouyer Exp $

LIB=	clangFrontendTool

.include <bsd.init.mk>

.PATH: ${CLANG_SRCDIR}/lib/FrontendTool

SRCS+=	ExecuteCompilerInvocation.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
