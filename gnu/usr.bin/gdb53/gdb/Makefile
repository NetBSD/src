#	$NetBSD: Makefile,v 1.2 2003/08/16 16:03:05 christos Exp $

.include <bsd.own.mk>

.if ${MKGDB} != "no" && ${MKBFD} != "no" && \
    exists(${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk)

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

.include "${.CURDIR}/../arch/${MACHINE_ARCH}/defs.mk"

PROG=		gdb

SRCS=		${G_LIBGDB_OBS:.o=.c} ${G_OBJECTS:Nxmalloc.o:.o=.c} ${G_SIM_OBS:.o=.c} main.c
CPPFLAGS=	-I${.CURDIR}/../arch/${MACHINE_ARCH} \
		-I${DIST}/ \
		-I${DIST}/gdb \
		-I.\
		-I${DIST}/gdb \
		-I${DIST}/gdb/config \
		-I${DIST}/include \
		-I${DIST}/include/opcode \
		-I${DIST}/readline \
		-I${DIST}/bfd   \
		-I${DIST}/gdb/tui \
		-DHAVE_CONFIG_H \
		-DLOCALEDIR='"/usr/share/locale"' \
		-DUSE_INCLUDED_REGEX \
		-DRL_LIBRARY_VERSION='"4.0"' \
		-DNETBSD_CORE \
		${G_CCFLAGS} 


CFLAGS+=-g -O2
LDADD=		-lcurses -ltermcap -lintl -lm -lkvm
DPADD=		${LIBTERMCAP} ${LIBINTL} ${LIBM} ${LIBKVM}

.if ${SRCS:Mnbsd-thread.c} != ""
LDADD+=                -lpthread_dbg
.endif

TEXINFO=	gdb.texinfo gdbint.texinfo stabs.texinfo
INFOFLAGS=	-I${DIST}/gdb/doc -I${DIST}/gdb/mi \
		-I${DIST}/readline/doc -I${.CURDIR}/..

.endif	# MKGDB != no && MKBFD != no

.include "../../Makefile.inc"
.include <bsd.prog.mk>
.include <bsd.info.mk>

# PSIM support for PowerPC.
#.if ${MACHINE_ARCH} == powerpc || ${MACHINE_ARCH} == powerpcle
PSIMOBJ!=	cd ${TOP}/usr.bin/sim/ppc && ${PRINTOBJDIR}
LDADD+=		-L${PSIMOBJ} -lsim
DPADD+=		${PSIMOBJ}/libsim.a
#.endif

.PATH: ${DIST}/gdb ${DIST}/gdb/mi ${DIST}/gdb/cli ${DIST}/gdb/tui \
	${DIST}/gdb/doc ${DIST}/libiberty ${DIST}/bfd ${DIST}/opcodes \
	${DIST}/readline ${.CURDIR}/../arch/${MACHINE_ARCH}

rl-signals.o: ${DIST}/readline/signals.c
	${CC} ${CPPFLAGS} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}

gdb-signals.o: ${DIST}/gdb/signals/signals.c
	${CC} ${CPPFLAGS} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}

gdb-corefile.o: ${DIST}/gdb/corefile.c
	${CC} ${CPPFLAGS} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}

bfd-corefile.o: ${DIST}/bfd/corefile.c
	${CC} ${CPPFLAGS} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}

bfd-init.o: ${DIST}/bfd/init.c
	${CC} ${CPPFLAGS} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}
