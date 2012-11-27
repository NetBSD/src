#	$NetBSD: Makefile,v 1.4 2012/11/27 20:00:39 jkunz Exp $

SUBDIR+= gzboot
SUBDIR+= boot2440
.if ${MACHINE_ARCH} == "arm"
# According to the i.MX23 Reference Manual section 3.1, Page 3-3:
# "The i.MX23 always operates in litle-endian mode."
SUBDIR+= bootimx23
.endif

.include <bsd.subdir.mk>
