#	$NetBSD: Makefile,v 1.2 1998/03/19 00:08:25 thorpej Exp $
# Build a smaller sh (e.g. for boot media)

# This can't be used, since crunchgen does not run this makefile
# from this directory.
#.CURDIR :=	${.CURDIR}/../../../bin/sh

.include <bsd.own.mk>	# XXX for BSDSRCDIR

# XXX Yes, this is really ugly.
.if !defined(BSDSRCDIR) || empty(BSDSRCDIR)
FOO!="***** error: BSDSRCDIR must be defined."
breakthingsnow
.endif
.CURDIR :=	${BSDSRCDIR}/bin/sh

NOMAN=1

CPPFLAGS+=	-I${.CURDIR}
CFLAGS+=	-DSMALL

.PATH: ${.CURDIR} ${.CURDIR}/bltin ${.CURDIR}/../../usr.bin/printf

.include "${.CURDIR}/Makefile"
