#	$NetBSD: Makefile.inc,v 1.4 2001/09/22 01:19:11 mrg Exp $

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
.else
build: ${TIMESTAMP}
${TIMESTAMP}: ${_CURDIR}/Makefile
.endif
.if ${MKTOOLS} != "no"
.if !defined(UPDATE) && !defined(NOCLEANDIR)
	@cd ${_CURDIR} && ${MAKE} cleandir
.endif
	@cd ${_CURDIR} && ${MAKE} all
	@cd ${_CURDIR} && ${MAKE} install
.else
	@echo "===> $@ must be updated"
	@echo "===> (set MKTOOLS=yes in /etc/mk.conf or the environment to continue)"
	@false
.endif
.endif
