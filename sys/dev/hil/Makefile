#	$NetBSD: Makefile,v 1.1.4.2 2011/03/06 15:07:56 bouyer Exp $
#	$OpenBSD: Makefile,v 1.1 2003/02/11 19:39:30 miod Exp $

.include <bsd.own.mk>

PROG=	devlist2h.awk
SOURCE=	hildevs

${SOURCE}_data.h: ${SOURCE} ${PROG}
	/bin/rm -f $@
	${TOOL_AWK} -f ${PROG} ${SOURCE} > $@
