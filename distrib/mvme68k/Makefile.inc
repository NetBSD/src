#	$NetBSD: Makefile.inc,v 1.4 2002/04/10 15:05:43 lukem Exp $

.include "../../../Makefile.inc"

REV!=sh ${_SRC_TOP_}/sys/conf/osrelease.sh -s

LDSTATIC=	-static		# only static compilation makes sense here

# Hack needed to find kernel images...
SRCSYSDIR?=	${_SRC_TOP_}/sys
