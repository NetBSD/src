#	$NetBSD: Makefile.inc,v 1.3 2002/03/04 16:43:09 shiba Exp $

.include "../../../Makefile.inc"

REV!=sh ${KERNSRCDIR}/conf/osrelease.sh -s
VER!=sh ${KERNSRCDIR}/conf/osrelease.sh 
