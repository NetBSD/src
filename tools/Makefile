#	$NetBSD: Makefile,v 1.4 2001/09/22 12:29:22 tv Exp $

.include <bsd.own.mk>

.ifdef USE_NEW_TOOLCHAIN
# XXX this should use real dependencies
SUBDIR=		binstall .WAIT gencat mkdep rpcgen tsort \
		yacc .WAIT lex .WAIT \
		compile_et config lint lint1 lint2 lorder \
		toolchain .WAIT texinfo
.endif

TARGETS=	build obj

.include <bsd.subdir.mk>
