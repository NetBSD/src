#	$NetBSD: Makefile,v 1.12 2001/10/31 17:46:08 tv Exp $

.include <bsd.own.mk>

# XXX this should use real dependencies
SUBDIR=		binstall .WAIT gencat mkdep mtree rpcgen tsort \
		yacc .WAIT lex .WAIT \
		asn1_compile compile_et config lint lint1 lint2 lorder \
		toolchain .WAIT texinfo crunchgen msgc menuc mklocale \
		mdsetimage

TARGETS+=	build

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

cleandir: cleantools
cleantools:
	rm -r -f tools.${HOST_OSTYPE}
