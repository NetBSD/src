#	$NetBSD: Makefile,v 1.32.2.1 1999/12/27 18:29:19 wrstuden Exp $

# for OBJECT_FMT
.include <bsd.own.mk>

.if (${OBJECT_FMT} == "ELF") || defined(BOOTSTRAP_ELF)
ELF_PLEASE=
.endif

PROG=		gdb
MAN=		gdb.1
SRCS=		main.c ${COMMON_SRCS} annotate.c inflow.c gnu-regex.c
OBJS=		init.o
DPSRCS=		nm.h tm.h
CLEANFILES=	init.c

CPPFLAGS+=	-I. -I${.CURDIR} -I${DIST}/gdb/config -I${DIST}/gdb \
		-I${BFDOBJ} -I${DIST}/bfd -I${DIST}/include \
		-DHOST_NAME=\"${MACHINE_GNU_ARCH}--netbsd\" \
		-DTARGET_NAME=\"${MACHINE_GNU_ARCH}--netbsd\"
.if defined(ELF_PLEASE)
CPPFLAGS+=	-DSVR4_SHARED_LIBS
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
REMOTE_SRCS= remote.c dcache.c remote-utils.c tracepoint.c
DEPFILES= ${TDEPFILES.${MACHINE_ARCH}} ${XDEPFILES} ${SER_HARDWIRE} \
	${NATDEPFILES} ${REMOTE_SRCS} ${SIM_SRCS}

XDEPFILES= ser-tcp.c
SER_HARDWIRE= ser-unix.c
NATDEPFILES= infptrace.c inftarg.c fork-child.c corelow.c \
	${NATDEPFILES.${MACHINE_ARCH}}
.if (${MACHINE_ARCH} != "arm32") && (${MACHINE_ARCH} != "powerpc")
NATDEPFILES+= kcore-nbsd.c
.endif
.if ${MKPIC} != "no"
NATDEPFILES+= solib.c
.endif

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

#NATDEPFILES.sh3=	sh-nat.c
TDEPFILES.sh3=		sh-tdep.c

NATDEPFILES.sparc=	sparcnbsd-nat.c
TDEPFILES.sparc=	sparc-tdep.c

NATDEPFILES.sparc64=	sp64nbsd-nat.c
TDEPFILES.sparc64=	sparc-tdep.c

NATDEPFILES.vax=	vaxnbsd-nat.c
TDEPFILES.vax=		vax-tdep.c

DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/libbfd && ${MAKE} -s print-objdir

.PATH: ${DIST}/gdb ${DIST}/gdb/doc

.include <bsd.info.mk>
.include <bsd.prog.mk>

init.o: ${OBJS:Ninit.o} Makefile
	@-rm -f init.c
	NM=${NM} sh ${.CURDIR}/mkinit.sh ${OBJS:Ninit.o} >init.c
	${COMPILE.c} init.c

${OBJS}: nm.h tm.h xm.h

ARCHSUBDIR= ${MACHINE_GNU_ARCH:C/mipse[bl]/mips/:C/sparc64/sparc/}

nm.h: ${DIST}/gdb/config/${ARCHSUBDIR}/nm-nbsd.h
	@-rm -f $@
	cp $> $@

tm.h: ${DIST}/gdb/config/${ARCHSUBDIR}/tm-nbsd.h
	@-rm -f $@
	cp $> $@
