#	$NetBSD: Makefile,v 1.18 2002/02/11 21:24:09 tv Exp $

.include <bsd.own.mk>

MODULE=		toolchain

FIND_ARGS=	\! \( -type d -name sim -prune \)

CONFIGURE_ENV=	LANGUAGES="c c++ objc"
CONFIGURE_ARGS=	--target=${MACHINE_GNU_PLATFORM} --disable-nls \
		--program-transform-name="s,^,${MACHINE_GNU_PLATFORM}-,"

MAKE_ARGS=	MACHINE= MAKEINFO=${MAKEINFO:Q} \
		LIBGCC= LIBGCC1= LIBGCC1_TEST= LIBGCC2= INSTALL_LIBGCC=

ALL_TARGET=	all-binutils all-gas all-ld all-gcc
INSTALL_TARGET=	install-binutils install-gas install-ld install-gcc

.if ${MKCROSSGDB:Uno} == "yes"
ALL_TARGET+=	all-gdb
INSTALL_TARGET+=install-gdb
.endif

bootstrap-libgcc: .configure_done
	@echo 'Creating files needed for libgcc by a native bootstrap build.'
	@MAKE=${MAKE:Q} sh ${.CURDIR}/mknative libgcc ${.CURDIR}/../..

.include "${.CURDIR}/../Makefile.gnuhost"
