#	$NetBSD: Makefile,v 1.6 2002/01/29 10:20:31 tv Exp $

HOSTLIB=	nbcompat

SRCS=		basename.c dirname.c fgetln.c flock.c fparseln.c \
		getmode.c getopt_long.c pread.c pwcache.c pwrite.c \
		pw_scan.c setenv.c setgroupent.c setpassent.c \
		setprogname.c snprintf.c \
		strlcat.c strlcpy.c strmode.c strsep.c unvis.c vis.c \
		_err.c _errx.c _verr.c _verrx.c _vwarn.c _vwarnx.c \
		_warn.c _warnx.c __fts13.c

# -D_FILE_OFFSET_BITS=64 produces a much more amenable `struct stat', and
# other file ops, on many systems, without changing function names.

CPPFLAGS+=	-I. -I./include -I${.CURDIR} -DHAVE_CONFIG_H -Dlint \
		-D_FILE_OFFSET_BITS=64

.PATH:		${.CURDIR}/../../lib/libc/gen \
		${.CURDIR}/../../lib/libc/stdlib \
		${.CURDIR}/../../lib/libc/string \
		${.CURDIR}/../../lib/libutil \

DPSRCS+=	defs.mk
CLEANFILES=	config.log config.status confdefs.h includedir.stamp

# Get components of Berkeley DB.
_CURDIR:=	${.CURDIR}
.CURDIR:=	${_CURDIR}/../../lib/libc
.include "${.CURDIR}/db/Makefile.inc"
.CURDIR:=	${_CURDIR}

SRCS:=		${SRCS:Nndbm.c}

defs.mk: includedir.stamp configure config.h.in defs.mk.in
	CC=${HOST_CC:Q} CFLAGS=${HOST_CFLAGS:Q} LDFLAGS=${HOST_LDFLAGS:Q} \
		sh ${.CURDIR}/configure --cache-file=config.cache
	@touch $@

# Run by hand, then "configure" script committed:
regen:
	cd ${.CURDIR} && ${TOOLDIR}/bin/nbautoconf

includedir.stamp:
	mkdir -p include/sys
	@touch $@

cleandir: compat.clean
compat.clean:
	-rm -r -f include
	-rm -f config.cache config.h

HOST_CPPFLAGS:=	${CPPFLAGS}
CPPFLAGS:=	# empty

.include <bsd.hostlib.mk>
