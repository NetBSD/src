#	$NetBSD: Makefile.inc,v 1.7.2.1 2002/03/25 17:32:50 he Exp $

.include "../../Makefile.inc"
.include <bsd.own.mk>

REV!=sh ${.CURDIR}/../../..//sys/conf/osrelease.sh -s
VERDEP= ${.CURDIR}/../../..//sys/conf/osrelease.sh
