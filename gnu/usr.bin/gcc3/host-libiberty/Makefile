#	$NetBSD: Makefile,v 1.5 2004/06/12 14:59:36 jmc Exp $

libiberty/libiberty.a:
	[ ! -d libiberty ] && mkdir libiberty || exit 0
	(cd libiberty; env CC=${HOST_CC:Q} ${HOST_SH} ${DIST}/libiberty/configure && env CC=${HOST_CC:Q} ${MAKE})

cleandir:
	rm -rf libiberty

depend dependall all: libiberty/libiberty.a
	
.include <bsd.prog.mk>
