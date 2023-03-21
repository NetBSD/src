#	$NetBSD: Makefile,v 1.10 2023/03/21 16:54:43 christos Exp $

NOCTF=
.include <bsd.own.mk>
.include "../Makefile.inc"

GDBARCHDIR=${.CURDIR}/../../lib/libgdb/arch/${GDB_MACHINE_ARCH}
.include "${GDBARCHDIR}/defs.mk"

PROG=		gdb

SRCS=		gdb.c
CPPFLAGS+=	-I${.CURDIR} \
		-I${.CURDIR}/../../lib/libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib \
		-I${DIST}/gdb \
		-I${DIST}/gdbsupport \
		-I${DIST}/gdb/config \
		-I${DIST}/gnulib/import \
		-I${DIST}/include/opcode \
		-I${DIST}/readline \
		-I${DIST}/libdecnumber \
		-I${DIST} \
		-I${.CURDIR}/../../lib/libbfd/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../../lib/libgdbsupport/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../../lib/libgnulib/arch/${GDB_MACHINE_ARCH} \
		-I${.CURDIR}/../../lib/libgnulib/arch/${GDB_MACHINE_ARCH}/gnulib/import \
		-I${DIST}/bfd \
		-I${DIST}/include \
		${GCPPFLAGS:M-D*}

TEXINFO=	gdb.texinfo stabs.texinfo
INFOFLAGS=	-I${DIST}/gdb/doc -I${DIST}/gdb/mi \
		-I${DIST}/readline/readline/doc -I${.CURDIR}/.. -DGDBN=${GDBN}

GDBvn.texi: ${DIST}/gdb/version.in
	echo "@set GDBVN $$(${TOOL_CAT} ${.ALLSRC})" > ${.TARGET}

gdb-cfg.texi: ${DIST}/gdb/doc/all-cfg.texi
	${TOOL_CAT} ${.ALLSRC} > ${.TARGET}

CLEANFILES+= GDBvn.texi gdb-cfg.texi
stabs.info gdb.info: gdb-cfg.texi GDBvn.texi

FILES=		netbsd.xml
FILESDIR=	/usr/share/gdb/syscalls

.include "../../Makefile.inc"
.include <bsd.info.mk>

LIBCTFDIR!=	     cd ${.CURDIR}/../../lib/libctf	   && ${PRINTOBJDIR}
LIBGNULIBDIR!=	     cd ${.CURDIR}/../../lib/libgnulib	   && ${PRINTOBJDIR}
LIBGDBSUPPORTDIR!=   cd ${.CURDIR}/../../lib/libgdbsupport && ${PRINTOBJDIR}
LIBOPCODESDIR!=	     cd ${.CURDIR}/../../lib/libopcodes	   && ${PRINTOBJDIR}
LIBBFDDIR!=	     cd ${.CURDIR}/../../lib/libbfd	   && ${PRINTOBJDIR}
LIBIBERTYDIR!=	     cd ${.CURDIR}/../../lib/libiberty	   && ${PRINTOBJDIR}
LIBGDBDIR!=	     cd ${.CURDIR}/../../lib/libgdb	   && ${PRINTOBJDIR}
LIBDECNUMBERDIR!=    cd ${.CURDIR}/../../lib/libdecnumber  && ${PRINTOBJDIR}

.if ${USE_LIBEDIT} == yes
LIBRL=-ledit
LIBRLDP=${LIBEDIT}
.else
LIBRLDIR!=  cd ${.CURDIR}/../../lib/libreadline && ${PRINTOBJDIR}
LIBRL=      -L${LIBRLDIR} -lreadline
LIBRLDP=    ${LIBRLDIR}/libreadline.a
.endif

LDADD+= -L${LIBGDBDIR} -lgdb
DPADD+= ${LIBGDBDIR}/libgdb.a

# Simulator support
.if ${GDB_MACHINE_ARCH} == powerpc || ${GDB_MACHINE_ARCH} == powerpcle || \
    ${GDB_MACHINE_ARCH} == powerpc64
SIMOBJ!=	cd ${.CURDIR}/../../lib/libsim && ${PRINTOBJDIR}
LDADD+=		-L${SIMOBJ} -lsim
DPADD+=		${SIMOBJ}/libsim.a
.elif ${MACHINE_CPU} == xxmips
SIMOBJ!=	cd ${.CURDIR}/../../lib/libsim && ${PRINTOBJDIR}
LDADD+=		-L${SIMOBJ} -lsim
DPADD+=		${SIMOBJ}/libsim.a
.endif

LDADD+= -L${LIBOPCODESDIR} -lopcodes \
	-L${LIBBFDDIR} -lbfd \
	-L${LIBDECNUMBERDIR} -ldecnumber \
	-L${LIBGDBSUPPORTDIR} -lgdbsupport \
	-L${LIBCTFDIR} -lctf \
	-L${LIBGNULIBDIR} -lgnulib \
	${LIBRL} \
	-lterminfo \
	-L${LIBIBERTYDIR} -liberty

DPADD+=	${LIBOPCODESDIR}/libopcodes.a \
	${LIBBFDDIR}/libbfd.a \
	${LIBDECNUMBERDIR}/libdecnumber.a \
	${LIBGDBSUPPORTDIR}/libgdbsupport.a \
	${LIBCTFDIR}/libctf.a \
	${LIBGNULIBDIR}/libgnulib.a \
	${LIBRLDP} \
	${LIBTERMINFO} \
	${LIBIBERTYDIR}/libiberty.a

LDADD+=		-lexpat -llzma -lz -lcurses -lintl -lm -lkvm -lutil -lpthread
DPADD+=		${LIBEXPAT} ${LIBLZMA} ${LIBZ} ${LIBCURSES} ${LIBINTL} ${LIBM} ${LIBKVM} ${LIBUTIL} ${LIBPTHREAD}

.include <bsd.prog.mk>

# Force c++
CWARNFLAGS.clang+=	-Wno-deprecated
HOST_CC:=		${HOST_CXX}
HOST_CFLAGS+=		${HOST_CXXFLAGS}
CC:=			${CXX}
CFLAGS:=		${CXXFLAGS} -std=gnu++11 -Wno-error=stack-protector

.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/signals ${DIST}/gdb/syscalls ${DIST}/gdb/doc \
    ${.CURDIR}/../arch/${GDB_MACHINE_ARCH}
