#	$NetBSD: Makefile,v 1.9 2001/11/12 23:16:35 tv Exp $

.include <bsd.own.mk>

MODULE=		toolchain

FIND_ARGS=	\! \( -type d -name sim -prune \)

CONFIGURE_ENV=	LANGUAGES="c c++ objc"
CONFIGURE_ARGS=	--target=${MACHINE_GNU_PLATFORM} --disable-nls

.MAKEFLAGS+=	MACHINE= \
		LIBGCC= LIBGCC1= LIBGCC1_TEST= LIBGCC2= INSTALL_LIBGCC= \
		INSTALL_CPP=

ALL_TARGET=	all-binutils all-gas all-ld all-gcc
INSTALL_TARGET=	install-binutils install-gas install-ld install-gcc

.if ${MKCROSSGDB:Uno} == "yes"
ALL_TARGET+=	all-gdb
INSTALL_TARGET+=install-gdb
.endif

.include "${.CURDIR}/../Makefile.gnuhost"
