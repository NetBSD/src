#	$NetBSD: Makefile,v 1.1 2003/10/26 01:06:27 cl Exp $

.include <bsd.own.mk>

MODULE=		gdb

FIND_ARGS=	\! \( -type d -name sim -prune \)

CONFIGURE_ARGS=	--target=${MACHINE_GNU_PLATFORM} --disable-nls \
		--program-transform-name="s,^,${MACHINE_GNU_PLATFORM}-,"

MAKE_ARGS=	MACHINE= MAKEINFO=${TOOL_MAKEINFO:Q}

ALL_TARGET=	all-gdb
INSTALL_TARGET=	install-gdb

.include "${.CURDIR}/../Makefile.gnuhost"

