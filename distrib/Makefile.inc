#	$NetBSD: Makefile.inc,v 1.5 2002/04/10 15:02:39 lukem Exp $

.include "../Makefile.inc"

# Top level of distrib
#
DISTRIBDIR=	${_SRC_TOP_}/distrib

# DISTRIBREV is revision without dots, as in "12C"
# DISTRIBVER is revision with dots, as in "1.2C"
#
DISTRIBREV!=	sh ${_SRC_TOP_}/sys/conf/osrelease.sh -s
DISTRIBVER!=	sh ${_SRC_TOP_}/sys/conf/osrelease.sh
