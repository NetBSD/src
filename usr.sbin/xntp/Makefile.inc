#	$NetBSD: Makefile.inc,v 1.9 1998/04/01 15:01:17 christos Exp $

#WARNS?=	0
CPPFLAGS += -I${.CURDIR}/../include -DHAVE_CONFIG_H -Drestrict=1 -Dunrestrict=1
LDADD += -lntp
DPADD += ${LIBNTP}

# Some later versions (at least 2.7.2.3.f.1) of GCC have a `restrict'
# keyword?!
CFLAGS+=-ansi

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
