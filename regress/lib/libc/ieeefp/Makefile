#	$NetBSD: Makefile,v 1.10 2001/10/28 10:41:56 simonb Exp $

# Don't hook up testfloat just yet. (ross, 2001.3.12)

SUBDIR+= except infinity round
.if ${MACHINE} != "vax"
SUBDIR+= nan
.endif


.include <bsd.subdir.mk>
