#	$NetBSD: Makefile.inc,v 1.3 2002/02/26 13:25:03 lukem Exp $

.include "../Makefile.inc"

# Top level of distrib
#
DISTRIBDIR=	${_SRC_TOP_}/distrib

# DISTRIBREV is revision without dots, as in "12C"
# DISTRIBVER is revision with dots, as in "1.2C"
#
DISTRIBREV!=	sh ${KERNSRCDIR}/conf/osrelease.sh -s
DISTRIBVER!=	sh ${KERNSRCDIR}/conf/osrelease.sh

# Helper targets
.if !target(check_DESTDIR)
check_DESTDIR: .PHONY
.if !defined(DESTDIR)
	@echo "setenv DESTDIR before doing that!"
	@false
.else
	@true
.endif
.endif

.if !target(check_RELEASEDIR)
check_RELEASEDIR: .PHONY
.if !defined(RELEASEDIR)
	@echo "setenv RELEASEDIR before doing that!"
	@false
.else
	@true
.endif
.endif
