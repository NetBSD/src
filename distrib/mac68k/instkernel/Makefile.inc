#	$NetBSD: Makefile.inc,v 1.2.2.1 2002/03/25 17:32:54 he Exp $
.include <bsd.own.mk>

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh
VERDEP= ${BSDSRCDIR}/sys/conf/osrelease.sh
