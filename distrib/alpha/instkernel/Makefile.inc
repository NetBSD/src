#	$NetBSD: Makefile.inc,v 1.1.10.1 2002/03/25 17:32:42 he Exp $

REV!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh -s
VER!=sh ${.CURDIR}/../../../../sys/conf/osrelease.sh
VERDEP= ${.CURDIR}/../../../../sys/conf/osrelease.sh
