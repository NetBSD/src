#	$NetBSD: Makefile,v 1.31 2002/01/29 10:20:31 tv Exp $

# XXX Note: NO_DBSYM and NO_MAKEFS are stopgap temporary variables for use
# with the development of non-NetBSD hosting support.  These will go away.

.include <bsd.own.mk>

# XXX We can't currently build the toolchain for x86-64.
.if ${MACHINE_ARCH} != "x86_64"
TOOLCHAIN_BITS= toolchain .WAIT ${NO_DBSYM:D:Udbsym} mdsetimage
.endif

.if ${MKLINT} != "no"
LINT_BITS=	lint lint1 lint2
.endif

# Dependencies in SUBDIR below ordered to maximize parallel ability.
.if !defined(NOSUBDIR)
SUBDIR=	host-mkdep .WAIT compat .WAIT \
	binstall .WAIT \
		crunchgen gencat lorder makewhatis mkdep mtree \
		rpcgen texinfo tsort \
	${TOOLCHAIN_BITS} \
	yacc .WAIT \
		groff \
	lex .WAIT \
		${LINT_BITS} asn1_compile compile_et config \
		${NO_MAKEFS:D:Umakefs} menuc mklocale msgc pax pwd_mkdb zic

.if ${MKMAINTAINERTOOLS:Uno} != "no"
SUBDIR+=	m4 .WAIT autoconf
.endif

.if ${MACHINE} == sparc || ${MACHINE} == sparc64
SUBDIR+=	sparc-installboot
.endif
.endif

.if ${MKTOOLS:Uyes} == "no" || ${USETOOLS} != "yes"
realall install: _warn
.if ${MKTOOLS:Uyes} == "no"
_warn:
	@echo '*** WARNING: "MKTOOLS" is set to "no"; this will prevent building and'
	@echo '*** updating your host toolchain.  This should be used only as a'
	@echo '*** temporary workaround for toolchain problems, as it will result'
	@echo '*** in version skew and build errors over time!'
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
