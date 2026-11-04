#	$NetBSD: Makefile,v 1.10.2.1 2026/11/04 23:01:11 perseant Exp $

libiberty/libiberty.a:
	[ ! -d libiberty ] && mkdir libiberty || true
	(cd libiberty; \
		CC=${HOST_CC:Q} \
		CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} \
		MAKE=${MAKE:Q} \
		CONFIG_SHELL=${HOST_SH:Q} \
		${HOST_SH} ${DIST}/libiberty/configure \
		&& CC=${HOST_CC:Q} CFLAGS=${HOST_CFLAGS:Q} \
		CPPFLAGS=${HOST_CPPFLAGS:Q} ${MAKE})

cleandir:
	-rm -rf libiberty

includes depend dependall all: libiberty/libiberty.a

.include <bsd.prog.mk>
