#	$NetBSD: Makefile,v 1.2 2019/07/24 14:07:17 joerg Exp $

SUBDIR=		lib .WAIT bin

CONFIG_DIR:=	${.PARSEDIR}/dist

run-configure:
	[ ! -d tmp ] || rm -r tmp
	mkdir -p tmp
	cd tmp && CONFIG_SITE= ${CONFIG_DIR}/configure --without-xml2 --host=${MACHINE_GNU_ARCH}--netbsd \
	    CC=${CC:Q} CFLAGS=${CFLAGS:N-Werror:Q} CPPFLAGS=${CPPFLAGS:Q} LDFLAGS=${LDFLAGS:Q}
	rm -r tmp

.include <bsd.own.mk>
.include <bsd.subdir.mk>
