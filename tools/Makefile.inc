#	$NetBSD: Makefile.inc,v 1.8 2001/10/27 01:57:40 perry Exp $

.include <bsd.own.mk>

# Stash the current directory (some builds here actually change .CURDIR).
_CURDIR?=${.CURDIR}

# Don't use target tool "strip" at install phase
INSTALL=	${TOOLDIR}/bin/binstall

# "make build":  Only build if the installed program is out-of-date.
# ("make all" and "make install" always build and install.)
.if make(build)
MKTOOLS?=no
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
	@echo "===> $@ must be updated"
	@echo "===> (set MKTOOLS=yes in /etc/mk.conf or the environment to continue)"
	@false
.endif
.endif
