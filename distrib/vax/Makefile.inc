#	$NetBSD: Makefile.inc,v 1.7 2000/05/31 21:51:27 matt Exp $

.include "../../Makefile.inc"
.include <bsd.own.mk>

REV!=sh ${.CURDIR}/../../..//sys/conf/osrelease.sh -s
