#	$NetBSD: Makefile.inc,v 1.8.4.1 2002/03/25 17:32:59 he Exp $

.include "../../../Makefile.inc"

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VERDEP= ${BSDSRCDIR}/sys/conf/osrelease.sh
