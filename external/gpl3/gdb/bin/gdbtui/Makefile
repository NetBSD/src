#	$NetBSD: Makefile,v 1.5.2.1 2014/08/10 07:11:35 tls Exp $

.include "../Makefile.inc"

GDBARCHDIR=${.CURDIR}/../../lib/libgdb/arch/${GDB_MACHINE_ARCH}
.include "${GDBARCHDIR}/defs.mk"

PROG=		gdbtui

SRCS=		tui-main.c
CPPFLAGS+=	-I${.CURDIR} \
		-I${GDBARCHDIR} \
		-I${DIST}/gdb \
		-I${DIST}/gdb/common \
		-I${DIST}/gdb/config \
		-I${DIST}/gdb/gnulib/import \
		-I${DIST}/include/opcode \
		-I${DIST} \
		-I${.CURDIR}/../../lib/libbfd/arch/${GDB_MACHINE_ARCH} \
		-I${DIST}/bfd \
		-I${DIST}/include \
		${GCPPFLAGS:M-D*}

NOMAN=		# defined

.include <bsd.prog.mk>


LIBOPCODESDIR!=   cd ${.CURDIR}/../../lib/libopcodes   && ${PRINTOBJDIR}
LIBBFDDIR!=       cd ${.CURDIR}/../../lib/libbfd       && ${PRINTOBJDIR}
LIBIBERTYDIR!=    cd ${.CURDIR}/../../lib/libiberty    && ${PRINTOBJDIR}
LIBGDBDIR!=	  cd ${.CURDIR}/../../lib/libgdb       && ${PRINTOBJDIR}
LIBDECNUMBERDIR!= cd ${.CURDIR}/../../lib/libdecnumber && ${PRINTOBJDIR}

.if ${USE_LIBEDIT} == yes
LIBEDIT=-ledit
.else
LIBRLDIR!=  cd ${.CURDIR}/../../lib/libreadline && ${PRINTOBJDIR}
LIBRL= -L${LIBRLDIR} -lreadline
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
	${LIBRL} \
	-lterminfo \
	-L${LIBIBERTYDIR} -liberty

DPADD+=	${LIBOPCODESDIR}/libopcodes.a \
	${LIBBFDDIR}/libbfd.a \
	${LIBRLDP} \
	${LIBTERMINFO} \
	${LIBIBERTYDIR}/libiberty.a

LDADD+=		-lexpat -llzma -lz -lcurses -lintl -lm -lkvm
DPADD+=		${LIBEXPAT} ${LIBLZMA} ${LIBZ} ${LIBCURSES} ${LIBINTL} ${LIBM} ${LIBKVM}

LDADD+=                -lpthread_dbg

.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
    ${DIST}/gdb/signals ${DIST}/gdb/doc ${.CURDIR}/../arch/${GDB_MACHINE_ARCH}
