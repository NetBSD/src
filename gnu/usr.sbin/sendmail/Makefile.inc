#	$NetBSD: Makefile.inc,v 1.2 2000/05/03 10:50:56 itojun Exp $

WARNS?=	0

CPPFLAGS+=	-DNEWDB -DNIS
CPPFLAGS+=	-DMAP_REGEX -DTCPWRAPPERS -DNETISO
#CPPFLAGS+=	-DNEEDSGETIPNODE -DNETINET6
CPPFLAGS+=	-I. -I${DIST}/sendmail/sendmail -I${DIST}/sendmail/include

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
