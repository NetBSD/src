#	$NetBSD: Makefile.inc,v 1.4 2002/03/05 03:51:23 lukem Exp $

.include "../Makefile.inc"

# Top level of distrib
#
DISTRIBDIR=	${_SRC_TOP_}/distrib

# DISTRIBREV is revision without dots, as in "12C"
# DISTRIBVER is revision with dots, as in "1.2C"
#
DISTRIBREV!=	sh ${KERNSRCDIR}/conf/osrelease.sh -s
DISTRIBVER!=	sh ${KERNSRCDIR}/conf/osrelease.sh
