#	$NetBSD: Makefile,v 1.14 2001/11/02 05:07:23 tv Exp $

.include <bsd.own.mk>

.if !defined(NOSUBDIR)
SUBDIR=		binstall .WAIT gencat mkdep mtree rpcgen tsort \
		yacc .WAIT lex .WAIT \
		asn1_compile compile_et config lint lint1 lint2 lorder \
		toolchain .WAIT texinfo crunchgen msgc menuc mklocale \
		mdsetimage
.endif

TARGETS+=	build

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleantools:
	rm -r -f tools.${HOST_OSTYPE}
