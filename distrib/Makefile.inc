#	$NetBSD: Makefile.inc,v 1.2 2002/02/08 02:27:55 lukem Exp $

.include "../Makefile.inc"

# Top level of distrib
#
DISTRIBDIR=	${_SRC_TOP_}/distrib

# DISTRIBREV is revision without dots, as in "12C"
# DISTRIBVER is revision with dots, as in "1.2C"
#
DISTRIBREV!=	sh ${KERNSRCDIR}/conf/osrelease.sh -s
DISTRIBVER!=	sh ${KERNSRCDIR}/conf/osrelease.sh
