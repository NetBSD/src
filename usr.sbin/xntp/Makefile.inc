#	$NetBSD: Makefile.inc,v 1.10 1998/04/13 14:42:03 lukem Exp $

#WARNS?=	0
CPPFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H -Drestrict=1 -Dunrestrict=1
LDADD += -lntp
DPADD += ${LIBNTP}

# Some later versions (at least 2.7.2.3.f.1) of GCC have a `restrict'
# keyword?!
COPTS+=-ansi

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
