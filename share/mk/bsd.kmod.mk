#	$NetBSD: bsd.kmod.mk,v 1.68 2003/09/04 07:15:43 lukem Exp $

.include <bsd.init.mk>

##### Basic targets
.PHONY:		cleankmod kmodinstall load unload
clean:		cleankmod
realinstall:	kmodinstall

##### Default values
S?=		/sys
KERN=		$S/kern

CFLAGS+=	-ffreestanding ${COPTS}
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -isystem $S -isystem $S/arch
CPPFLAGS+=	-D_KERNEL -D_LKM

DPSRCS+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${YHEADER:D${SRCS:M*.y:.y=.h}}
CLEANFILES+=	tmp.o

LNFILES+=	${S}/arch/${MACHINE}/include machine
LNFILES+=	${S}/arch/${MACHINE_CPU}/include ${MACHINE_CPU}
DPSRCS+=	machine ${MACHINE_CPU}

.if ${MACHINE} == "sun2" || ${MACHINE} == "sun3"
LNFILES+=	${S}/arch/sun68k/include sun68k
DPSRCS+=	sun68k
.elif ${MACHINE} == "sparc64"
LNFILES+=	${S}/arch/sparc64/include sparc64
DPSRCS+=	sparc64
.elif ${MACHINE} == "i386" || ${MACHINE} == "amd64"
LNFILES+=	${S}/arch/x86/include x86
DPSRCS+=	x86
.if ${MACHINE} == "amd64"
CFLAGS+=	-mcmodel=kernel
.endif
.elif ${MACHINE_ARCH} == "powerpc"
CLEANFILES+=	${KMOD}_tramp.o ${KMOD}_tramp.S tmp.S ${KMOD}_tmp.o
.endif

OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}
PROG?=		${KMOD}.o
MAN?=		${KMOD}.4

##### Build rules
realall:	${PROG}

${OBJS} ${LOBJS}: ${DPSRCS}

.if ${MACHINE_ARCH} == "powerpc"
${KMOD}_tmp.o: ${OBJS} ${DPADD}
	${LD} -r ${LDFLAGS} -o tmp.o ${OBJS}
	mv tmp.o ${.TARGET}

${KMOD}_tramp.S: ${KMOD}_tmp.o $S/lkm/arch/${MACHINE_ARCH}/lkmtramp.awk
	${OBJDUMP} --reloc ${KMOD}_tmp.o | \
		 awk -f $S/lkm/arch/${MACHINE_ARCH}/lkmtramp.awk > tmp.S
	mv tmp.S ${.TARGET}

${PROG}: ${KMOD}_tmp.o ${KMOD}_tramp.o
	${LD} -r ${LDFLAGS} \
		`${OBJDUMP} --reloc ${KMOD}_tmp.o | \
			 awk -f $S/lkm/arch/${MACHINE_ARCH}/lkmwrap.awk` \
		 -o tmp.o ${KMOD}_tmp.o ${KMOD}_tramp.o
.if exists($S/lkm/arch/${MACHINE_ARCH}/lkmhide.awk)
	${OBJCOPY} \
		`${NM} tmp.o | awk -f $S/lkm/arch/${MACHINE_ARCH}/lkmhide.awk` \
		tmp.o tmp1.o
	mv tmp1.o tmp.o
.endif
	mv tmp.o ${.TARGET}
.else
${PROG}: ${OBJS} ${DPADD}
	${LD} -r ${LDFLAGS} -o tmp.o ${OBJS}
	mv tmp.o ${.TARGET}
.endif

##### Install rules
.if !target(kmodinstall)
_PROG:=		${DESTDIR}${KMODDIR}/${PROG}		# installed path

.if ${MKUPDATE} == "no"
${_PROG}! ${PROG}					# install rule
.if !defined(BUILD) && !make(all) && !make(${PROG})
${_PROG}!	.MADE					# no build at install
.endif
.else
${_PROG}: ${PROG}					# install rule
.if !defined(BUILD) && !make(all) && !make(${PROG})
${_PROG}:	.MADE					# no build at install
.endif
.endif
	${INSTALL_FILE} -o ${KMODOWN} -g ${KMODGRP} -m ${KMODMODE} \
		${SYSPKGTAG} ${.ALLSRC} ${.TARGET}

kmodinstall::	${_PROG}
.PRECIOUS:	${_PROG}				# keep if install fails

.undef _PROG
.endif # !target(kmodinstall)

##### Clean rules
cleankmod:
	rm -f a.out [Ee]rrs mklog core *.core \
		${PROG} ${OBJS} ${LOBJS} ${CLEANFILES}

##### Custom rules
lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	${LINT} ${LINTFLAGS} ${LDFLAGS:M-L*} ${LOBJS} ${LDADD}
.endif

.if !target(load)
load: ${PROG}
	/sbin/modload ${KMOD_LOADFLAGS} -o ${KMOD} ${PROG}
.endif

.if !target(unload)
unload:
	/sbin/modunload -n ${KMOD}
.endif

##### Pull in related .mk logic
.include <bsd.man.mk>
.include <bsd.files.mk>
.include <bsd.links.mk>
.include <bsd.sys.mk>
.include <bsd.dep.mk>

.-include "$S/arch/${MACHINE_CPU}/include/Makefile.inc"
.-include "$S/arch/${MACHINE}/include/Makefile.inc"
