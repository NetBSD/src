#	$NetBSD: Makefile,v 1.1 2026/01/18 08:08:22 mrg Exp $

PROG=		f951
SRCS=		${G_F95_OBJS:.o=.cc}
SRCS+=		attribs.cc
SRCS+=		main.cc ${PROG}-checksum.cc

CPPFLAGS+=	-I${DIST}/gcc/fortran
#CPPFLAGS+=	-DOBJCPLUS
CPPFLAGS.prefix.cc+=	-DPREFIX=\"/usr\"
CPPFLAGS.module.cc+=	-DTARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\" \
			-DHOST_MACHINE=\"${MACHINE_GNU_PLATFORM}\"

.include "../Makefile.backend"
.include "../Makefile.backtrace"
.include "../Makefile.libcody"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"

CHECKSUM_OBJS=	${BACKENDOBJ}/libbackend.a \
		main.o \
		${LIBBACKTRACEOBJ}/libbacktrace.a \
		${COMMONTARGETOBJ}/libcommon-target.a \
		${COMMONOBJ}/libcommon.a \
		${LIBCPPOBJ}/libcpp.a \
		${LIBDECNUMBEROBJ}/libdecnumber.a \
		${LIBIBERTYOBJ}/libiberty.a \
		${OBJS:Nmain.o:Nf951-checksum.o} \
		checksum-options

.include "../Makefile.checksum"

# XXX: like cc1plus
CFLAGS+=-Wno-error=stack-protector

LDADD+= ${LIBIBERTYOBJ}/libiberty.a
DPADD+= ${LIBIBERTYOBJ}/libiberty.a
LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz -lm
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ} ${LIBM}

.PATH: ${DIST}/gcc/fortran ${DIST}/gcc ${DIST}/gcc/config
#.PATH: ${G_out_file:H}
