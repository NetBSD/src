#	$NetBSD: Makefile.inc,v 1.7 2002/04/26 15:50:21 lukem Exp $

.include "../Makefile.inc"

# DISTRIBDIR -	Top level of distrib
#
DISTRIBDIR=	${NETBSDSRCDIR}/distrib

# DISTRIBREV -	NetBSD version without dots, as in "15ZD"
# DISTRIBVER -	NetBSD version with dots, as in "1.5ZD"
#
DISTRIBREV!=	sh ${NETBSDSRCDIR}/sys/conf/osrelease.sh -s
DISTRIBVER!=	sh ${NETBSDSRCDIR}/sys/conf/osrelease.sh

# MAKESUMS -	runs makesums with the appropriate environment setup
# RELINSTALL -	installs release binaries
#
MAKESUMS=	CKSUM=${CKSUM:Q} sh ${DISTRIBDIR}/sets/makesums
RELINSTALL=	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}
