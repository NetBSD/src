#	$NetBSD: Makefile,v 1.16 2001/11/12 23:16:22 tv Exp $

.include <bsd.own.mk>

# Dependencies in SUBDIR below ordered to maximize parallel ability.
.if !defined(NOSUBDIR)
SUBDIR=	mkdep .WAIT \
	binstall .WAIT \
		crunchgen gencat lint lint2 lorder mtree rpcgen \
		texinfo tsort \
	toolchain .WAIT \
		dbsym mdsetimage \
	yacc .WAIT \
		groff \
	lex .WAIT \
		asn1_compile compile_et config lint1 msgc menuc mklocale
.endif

.include <bsd.subdir.mk>
.include <bsd.obj.mk>

# For each .WAIT point, make sure the previous target is installed before
# building a dependent target.
_dep:=
_last:=
.for d in ${SUBDIR}
_this:=		${d}
.if ${_this} == ".WAIT"
_dep:=		${_dep} install-${_last}
.else
_last:=		${d}
depend-${d} all-${d} dependall-${d}: ${_dep}
.endif
.endfor

cleantools:
	rm -r -f tools.${HOST_OSTYPE}
