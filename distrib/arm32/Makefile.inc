# $NetBSD: Makefile.inc,v 1.2.2.1 2002/03/25 17:32:45 he Exp $
.include <bsd.own.mk>

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s
VERDEP= ${BSDSRCDIR}/sys/conf/osrelease.sh
