#	$NetBSD: Makefile,v 1.12 2002/05/12 13:17:52 bjh21 Exp $

# Don't hook up testfloat just yet. (ross, 2001.3.12)

SUBDIR+= except infinity round
.if ${MACHINE} != "vax"
SUBDIR+= nan
.endif


.include <bsd.subdir.mk>
