#	$NetBSD: Makefile.inc,v 1.2 2001/08/14 12:36:10 tv Exp $

.include <bsd.own.mk>

# Ensure that TOOLDIR is set.
.if !defined(TOOLDIR)
.BEGIN:
	@echo "===> TOOLDIR must be set in ${MAKECONF} or the environment"
	@false
.endif

# Get default tool locations.
_CURDIR?=${.CURDIR}
.include "${_CURDIR}/../Makefile.tools"

# "make build":  Only build if the installed program is out-of-date.
# ("make all" and "make install" always build and install.)
.if make(build)
MKTOOLS?=no
.if ${MKTOOLS} == "always"
build:
	@cd ${_CURDIR} && ${MAKE} cleandir
.else
build: ${TIMESTAMP}
${TIMESTAMP}: ${_CURDIR}/Makefile
.endif
.if ${MKTOOLS} != "no"
	@cd ${_CURDIR} && ${MAKE} all
	@cd ${_CURDIR} && ${MAKE} install
.else
	@echo "===> $@ must be updated"
	@echo "===> (set MKTOOLS=yes in /etc/mk.conf or the environment to continue)"
	@false
.endif
.endif
