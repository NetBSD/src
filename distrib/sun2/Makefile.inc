#	$NetBSD: Makefile.inc,v 1.2 2001/12/17 20:27:39 fredette Exp $

.include "../../Makefile.inc"

REV!=sh ${.CURDIR}/../../../sys/conf/osrelease.sh -s

LDSTATIC=	-static		# only static compilation makes sense here

# Where to find kernel images...
.include <bsd.kernobj.mk>
