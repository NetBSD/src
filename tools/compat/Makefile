#	$NetBSD: Makefile,v 1.5 2002/01/24 04:05:27 lukem Exp $

HOSTLIB=	nbcompat

SRCS=		basename.c dirname.c err.c fgetln.c flock.c fparseln.c \
		getmode.c pread.c pwcache.c pwrite.c setprogname.c \
		strlcat.c strlcpy.c vis.c
HOST_CPPFLAGS+=	-I. -I${.CURDIR} -DHAVE_CONFIG_H

.PATH:		${.CURDIR}/../../lib/libc/gen \
		${.CURDIR}/../../lib/libc/string \
		${.CURDIR}/../../lib/libutil \

DPSRCS=		config.h
CLEANFILES=	config.cache config.log config.status config.stamp \
		confdefs.h defs.mk

# Get components of Berkeley DB.
_CURDIR:=	${.CURDIR}
.CURDIR:=	${_CURDIR}/../../lib/libc
.include "${.CURDIR}/db/Makefile.inc"
.CURDIR:=	${_CURDIR}

HOST_CPPFLAGS+=	${CPPFLAGS}
SRCS:=		${SRCS:Nndbm.c}

config.h defs.mk: config.stamp
config.stamp: configure config.h.in defs.mk.in
	CC=${HOST_CC:Q} CFLAGS=${HOST_CFLAGS:Q} LDFLAGS=${HOST_LDFLAGS:Q} \
		sh ${.CURDIR}/configure --cache-file=config.cache
	@touch $@

# Run by hand, then "configure" script committed:
regen:
	cd ${.CURDIR} && ${TOOLDIR}/bin/nbautoconf

.include <bsd.hostlib.mk>
