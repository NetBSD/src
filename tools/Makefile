#	$NetBSD: Makefile,v 1.19 2001/11/22 19:09:42 jmc Exp $

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

.if ${MKTOOLS:Uyes} == "no" || ${USETOOLS} != "yes"
realall install: _warn
.if ${MKTOOLS:Uyes} == "no"
_warn:
	@echo '*** WARNING: "MKTOOLS" is set to "no"; this will prevent building and'
	@echo '*** updating your host toolchain.  This should be used only as a'
	@echo '*** temporary workaround for toolchain problems, as it will result'
	@echo '*** in version skew and build errors onver time!'
.else
_warn:
.endif

.for dir in ${SUBDIR:N.WAIT}
all-${dir} depend-${dir} dependall-${dir} install-${dir}:
	@true
.endfor
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
