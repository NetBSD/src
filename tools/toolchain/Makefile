#	$NetBSD: Makefile,v 1.12 2001/11/13 15:32:15 tv Exp $

.include <bsd.own.mk>

MODULE=		toolchain

FIND_ARGS=	\! \( -type d -name sim -prune \)

CONFIGURE_ENV=	LANGUAGES="c c++ objc"
CONFIGURE_ARGS=	--target=${MACHINE_GNU_PLATFORM} --disable-nls

.MAKEFLAGS+=	MACHINE= \
		LIBGCC= LIBGCC1= LIBGCC1_TEST= LIBGCC2= INSTALL_LIBGCC=

ALL_TARGET=	all-binutils all-gas all-ld all-gcc
INSTALL_TARGET=	install-binutils install-gas install-ld install-gcc

.if ${MKCROSSGDB:Uno} == "yes"
ALL_TARGET+=	all-gdb
INSTALL_TARGET+=install-gdb
.endif

.include "${.CURDIR}/../Makefile.gnuhost"

afterinstall: _mvcpp
_mvcpp:
	[ ! -f ${TOOLDIR}/bin/cpp ] || \
		mv -f ${TOOLDIR}/bin/cpp ${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-cpp
