#	$NetBSD: Makefile.inc,v 1.6 2002/04/11 05:59:39 lukem Exp $

.include "../Makefile.inc"

# DISTRIBDIR -	Top level of distrib
#
DISTRIBDIR=	${_SRC_TOP_}/distrib

# DISTRIBREV -	NetBSD version without dots, as in "15ZD"
# DISTRIBVER -	NetBSD version with dots, as in "1.5ZD"
#
DISTRIBREV!=	sh ${_SRC_TOP_}/sys/conf/osrelease.sh -s
DISTRIBVER!=	sh ${_SRC_TOP_}/sys/conf/osrelease.sh

# MAKESUMS -	runs makesums with the appropriate environment setup
# RELINSTALL -	installs release binaries
#
MAKESUMS=	CKSUM=${CKSUM:Q} sh ${DISTRIBDIR}/sets/makesums
RELINSTALL=	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}
