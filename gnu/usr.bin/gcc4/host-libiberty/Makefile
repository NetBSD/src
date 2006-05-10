#	$NetBSD: Makefile,v 1.1 2006/05/10 22:48:14 mrg Exp $

libiberty/libiberty.a:
	[ ! -d libiberty ] && mkdir libiberty || exit 0
	(cd libiberty; env CC=${HOST_CC:Q} ${HOST_SH} ${DIST}/libiberty/configure && env CC=${HOST_CC:Q} ${MAKE})

cleandir:
	-rm -rf libiberty

depend dependall all: libiberty/libiberty.a
	
.include <bsd.prog.mk>
