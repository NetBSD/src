#	$NetBSD: Makefile,v 1.2 2000/06/20 06:00:27 thorpej Exp $

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

#
# DETERMINE WHICH CRYPTO SET TO USE
#

.include <bsd.own.mk>

#
# UTILITY TARGETS
#

check_destdir: .PHONY
.if !defined(DESTDIR)
	@echo "DESTDIR must be set"
	@false
.else
	@true
.endif

check_releasedir: .PHONY
.if !defined(RELEASEDIR)
	@echo "RELEASEDIR must be set"
	@false
.else
	@true
.endif

#
# SET BUILDING TARGETS
#

.PRECIOUS: checkflist
checkflist: check_destdir
	DESTDIR=${DESTDIR} sh ${.CURDIR}/checkflist

.PRECIOUS: maketars
maketars: check_destdir check_releasedir
	sh ${.CURDIR}/maketars -d ${DESTDIR} -t ${RELEASEDIR}/binary/sets

.PRECIOUS: makesums
makesums: check_releasedir
	sh ${.CURDIR}/makesums -t ${RELEASEDIR}/binary/sets

#
# MAIN ENTRY POINTS
#

sets: maketars makesums
	@true
