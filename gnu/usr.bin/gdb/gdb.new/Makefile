#	$NetBSD: Makefile,v 1.2 2001/08/16 16:59:24 mrg Exp $

# XXX - This gdb.new directory should be renamed to "gdb" after all platforms
# have been migrated to the new toolchain.

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

TOP=		${.CURDIR}/../../..
DIST=		${TOP}/dist/toolchain

PROG=		gdb

SRCS=		${G_LIBGDB_OBS:.o=.c} ${G_OBJECTS:Nxmalloc.o:.o=.c} main.c
CPPFLAGS=	-I${.CURDIR}/../arch/${MACHINE_ARCH} \
		${G_INTERNAL_CFLAGS:M-[ID]*:N*readline*:N*intl*:N-I.*} \
		${G_CCFLAGS:M-DRL_*} \
		-I${TOP}/lib/libbfd/arch/${MACHINE_ARCH}

LDADD=		-ltermcap -lintl -lm -lkvm
DPADD=		${LIBTERMCAP} ${LIBINTL} ${LIBM} ${LIBKVM}

TEXINFO=	gdb.texinfo gdbint.texinfo stabs.texinfo
INFOFLAGS=	-I${DIST}/gdb/doc -I${DIST}/gdb/mi \
		-I${DIST}/readline/doc -I${.CURDIR}/..

.include "../../Makefile.inc"
.include <bsd.prog.mk>
.include <bsd.info.mk>

BFDOBJ!=	cd ${TOP}/lib/libbfd && ${PRINTOBJDIR}
LDADD+=		-L${BFDOBJ} -lbfd
DPADD+=		${BFDOBJ}/libbfd_pic.a

IBERTYOBJ!=	cd ${TOP}/lib/libiberty && ${PRINTOBJDIR}
LDADD+=		-L${IBERTYOBJ} -liberty
DPADD+=		${IBERTYOBJ}/libiberty.a

.PATH: ${DIST}/gdb ${DIST}/gdb/tui ${DIST}/gdb/doc \
	${DIST}/readline ${.CURDIR}/../arch/${MACHINE_ARCH}
