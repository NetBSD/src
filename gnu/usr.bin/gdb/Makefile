#	$NetBSD: Makefile,v 1.49 2001/08/06 19:36:58 tv Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

.if !defined(USE_NEW_TOOLCHAIN)
.if (${OBJECT_FMT} == "ELF") || defined(BOOTSTRAP_ELF)
ELF_PLEASE=
.endif

MAN=		gdb.1
.if (${MACHINE_ARCH} != "sparc64")
PROG=		gdb
SRCS=		main.c ${COMMON_SRCS} annotate.c inflow.c gnu-regex.c
OBJS=		init.o
DPSRCS=		nm.h tm.h
CLEANFILES=	init.c
PARALLEL=	yes

CPPFLAGS+=	-I. -I${.CURDIR} -I${DIST}/gdb/config -I${DIST}/gdb \
		-I${BFDOBJ} -I${DIST}/bfd -I${DIST}/include \
		-I${DESTDIR}/usr/include/readline \
		-DHOST_NAME=\"${MACHINE_GNU_ARCH}--netbsd\" \
		-DTARGET_NAME=\"${MACHINE_GNU_ARCH}--netbsd\"
.if defined(ELF_PLEASE)
CPPFLAGS+=	-DSVR4_SHARED_LIBS
.endif

.if (${MACHINE_ARCH} == "sh3") && (${OBJECT_FMT} != "ELF")
CPPFLAGS+=	-DNO_SOLIB
.endif

LDADD+=		-ledit -lkvm -ltermcap -L${BFDOBJ} -lbfd
DPADD+=		${LIBEDIT} ${LIBKVM} ${LIBTERMCAP} ${BFDOBJ}/libbfd_pic.a

TEXINFO=	gdb.info gdbint.info stabs.info
INFOFLAGS=	-I${DIST}/gdb/doc -I${.CURDIR} -I${DIST}/readline/doc

# This exists only for easy comparison with the GNU Makefile.
COMMON_SRCS= version.c blockframe.c breakpoint.c findvar.c stack.c thread.c \
	source.c values.c eval.c valops.c valarith.c valprint.c printcmd.c \
	symtab.c symfile.c symmisc.c infcmd.c infrun.c command.c \
	expprint.c environ.c gdbtypes.c copying.c ${DEPFILES} \
	mem-break.c target.c parse.c language.c ${YYSRCS} buildsym.c \
	exec.c bcache.c objfiles.c minsyms.c maint.c demangle.c \
	dbxread.c coffread.c elfread.c \
	dwarfread.c dwarf2read.c mipsread.c stabsread.c corefile.c \
	c-lang.c ch-exp.c ch-lang.c f-lang.c \
	jv-lang.c jv-valprint.c jv-typeprint.o m2-lang.c \
	scm-exp.c scm-lang.c scm-valprint.c complaints.c typeprint.c \
	c-typeprint.c ch-typeprint.c f-typeprint.c m2-typeprint.c \
	c-valprint.c cp-valprint.c ch-valprint.c f-valprint.c m2-valprint.c \
	nlmread.c serial.c mdebugread.c os9kread.c top.c utils.c
YYSRCS= c-exp.y f-exp.y jv-exp.y m2-exp.y
REMOTE_SRCS= remote.c dcache.c remote-utils.c tracepoint.c remote-ipkdb.c
DEPFILES= ${TDEPFILES.${MACHINE_ARCH}} ${XDEPFILES} ${SER_HARDWIRE} \
	${NATDEPFILES} ${REMOTE_SRCS} ${SIM_SRCS}

XDEPFILES= ser-tcp.c
SER_HARDWIRE= ser-unix.c
NATDEPFILES= infptrace.c inftarg.c fork-child.c corelow.c \
	${NATDEPFILES.${MACHINE_ARCH}}
.if (${MACHINE_CPU} != "arm") && (${MACHINE_CPU} != "sh3")
NATDEPFILES+= kcore-nbsd.c
.endif
NATDEPFILES+= solib.c

NATDEPFILES.alpha=	alphanbsd-nat.c
TDEPFILES.alpha=	alpha-tdep.c

NATDEPFILES.arm32=	armb-nat.c arm-convert.s
TDEPFILES.arm32=	arm-tdep.c

NATDEPFILES.i386=	i386nbsd-nat.c
TDEPFILES.i386=		i386-tdep.c i387-tdep.c

NATDEPFILES.m68k=	m68knbsd-nat.c
TDEPFILES.m68k=		m68k-tdep.c

NATDEPFILES.mipseb=	mipsnbsd-nat.c
TDEPFILES.mipseb=	mips-tdep.c

NATDEPFILES.mipsel=	mipsnbsd-nat.c
TDEPFILES.mipsel=	mips-tdep.c

NATDEPFILES.ns32k=	ns32knbsd-nat.c
TDEPFILES.ns32k=	ns32k-tdep.c

NATDEPFILES.powerpc=	ppcnbsd-nat.c
TDEPFILES.powerpc=	rs6000-tdep.c

NATDEPFILES.sh3eb=	shnbsd-nat.c
TDEPFILES.sh3eb=	sh-tdep.c

NATDEPFILES.sh3el=	shnbsd-nat.c
TDEPFILES.sh3el=	sh-tdep.c

NATDEPFILES.sparc=	sparcnbsd-nat.c
TDEPFILES.sparc=	sparc-tdep.c

NATDEPFILES.sparc64=	sp64nbsd-nat.c
TDEPFILES.sparc64=	sparc-tdep.c

NATDEPFILES.vax=	vaxnbsd-nat.c
TDEPFILES.vax=		vax-tdep.c

BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${PRINTOBJDIR}
.endif
DIST=		${.CURDIR}/../../dist

.PATH: ${DIST}/gdb ${DIST}/gdb/doc

.include <bsd.info.mk>
.include <bsd.prog.mk>

.if (${MACHINE_ARCH} != "sparc64")
init.o: ${OBJS:Ninit.o} Makefile
	@-rm -f init.c
	NM=${NM} sh ${.CURDIR}/mkinit.sh ${OBJS:Ninit.o} >init.c
	${COMPILE.c} init.c

${OBJS}: nm.h tm.h xm.h

ARCHSUBDIR= ${MACHINE_GNU_ARCH:C/mipse[bl]/mips/:C/sparc64/sparc/:C/sh3e[bl]/sh3/}

nm.h: ${DIST}/gdb/config/${ARCHSUBDIR}/nm-nbsd.h
	@-rm -f $@
	cp $> $@

tm.h: ${DIST}/gdb/config/${ARCHSUBDIR}/tm-nbsd.h
	@-rm -f $@
	cp $> $@
.endif
.else
.include <bsd.prog.mk>
.endif
