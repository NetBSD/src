#	$NetBSD: Makefile,v 1.2 2006/09/29 20:14:10 apb Exp $

libiberty/libiberty.a:
	[ ! -d libiberty ] && mkdir libiberty || exit 0
	(cd libiberty; \
		CC=${HOST_CC:Q} \
		MAKE=${MAKE:Q} \
		CONFIG_SHELL=${HOST_SH:Q} \
		${HOST_SH} ${DIST}/libiberty/configure \
		&& CC=${HOST_CC:Q} ${MAKE})

cleandir:
	-rm -rf libiberty

depend dependall all: libiberty/libiberty.a
	
.include <bsd.prog.mk>
