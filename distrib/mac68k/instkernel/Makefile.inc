#	$NetBSD: Makefile.inc,v 1.2 2000/06/17 03:16:54 matt Exp $
.include <bsd.own.mk>

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VER!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh
