#	$NetBSD: Makefile,v 1.2 2002/01/04 14:39:06 lukem Exp $

HOSTLIB=	nbcompat

SRCS=		err.c fgetln.c fparseln.c setprogname.c
HOST_CPPFLAGS+=	-I. -DHAVE_CONFIG_H

DPSRCS=		config.h
CLEANFILES=	config.cache config.log config.status confdefs.h defs.mk

config.h: configure config.h.in defs.mk.in
	-rm -f config.h defs.mk
	sh ${.CURDIR}/configure --cache-file=config.cache

# Run by hand, then "configure" script committed:
regen:
	cd ${.CURDIR} && ${TOOLDIR}/bin/nbautoconf

.include <bsd.hostlib.mk>
