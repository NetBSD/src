#	$NetBSD: Makefile,v 1.1 2000/03/28 00:38:06 thorpej Exp $

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

#
# DETERMINE WHICH CRYPTO SET TO USE
#

.include <bsd.own.mk>

SRCTOP=	../..
.include <bsd.crypto.mk>

.if defined(CRYPTOBASE) && (${CRYPTOBASE} != "none")
CRYPTOWHICH!= basename ${CRYPTOBASE}
.endif

.if defined(CRYPTOWHICH)
.if (${CRYPTOWHICH} == "crypto-us")
CHECKFLIST_CRYPTOARG=-secr
CRYPTOSET="secr"
.endif

.if (${CRYPTOWHICH} == "crypto-intl")
CHECKFLIST_CRYPTOARG=-cryptint
CRYPTOSET="cryptint"
.endif
.endif # defined(CRYPTOWHICH)

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
	DESTDIR=${DESTDIR} sh ${.CURDIR}/checkflist ${CHECKFLIST_CRYPTOARG}

maketars_base: check_destdir check_releasedir
	sh ${.CURDIR}/maketars -d ${DESTDIR} -t ${RELEASEDIR}/binary/sets

maketars_crypto: check_destdir check_releasedir
	sh ${.CURDIR}/maketars -d ${DESTDIR} -t ${RELEASEDIR}/binary/security \
	    ${CRYPTOSET}

.PRECIOUS: maketars
maketars: maketars_base
.if defined(CRYPTOSET)
	@${MAKE} maketars_crypto
.else
	@true
.endif

makesums_base: check_releasedir
	sh ${.CURDIR}/makesums -t ${RELEASEDIR}/binary/sets

makesums_crypto: check_releasedir
	sh ${.CURDIR}/makesums -t ${RELEASEDIR}/binary/security

.PRECIOUS: makesums
makesums: makesums_base
.if defined(CRYPTOSET)
	@${MAKE} makesums_crypto
.else
	@true
.endif

#
# MAIN ENTRY POINTS
#

sets: maketars makesums
	@true

cryptoset: .PHONY
.if defined(CRYPTOSET)
	${MAKE} maketars_crypto
	${MAKE} makesums_crypto
.else
	@echo "No crypto set to make."
	@false
.endif
