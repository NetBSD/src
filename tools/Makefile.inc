#	$NetBSD: Makefile.inc,v 1.11 2001/10/31 17:46:08 tv Exp $

.include <bsd.own.mk>

.if ${USETOOLS} != "yes"
.undef HOSTPROG
.endif

# Stash the current directory (some builds here actually change .CURDIR).
_CURDIR?=${.CURDIR}

# Don't use target tool "strip" at install phase
INSTALL=	${TOOLDIR}/bin/binstall

# "make build":  Only build if the installed program is out-of-date.
# ("make all" and "make install" always build and install.)
.if make(build)
MKTOOLS?=yes
.if ${MKTOOLS} == "always"
build:
.else
build: ${TIMESTAMP}
${TIMESTAMP}: ${_CURDIR}/Makefile
.endif
.if ${MKTOOLS} != "no"
	@cd ${_CURDIR} && ${MAKE} cleandir
	@cd ${_CURDIR} && ${MAKE} all
	@cd ${_CURDIR} && ${MAKE} install
.else
	@echo "===> $@ must be updated, but MKTOOLS=no, so cannot build"
	@false
.endif
.endif
