#	$NetBSD: bsd.kmod.mk,v 1.64 2003/07/18 04:04:03 lukem Exp $

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
CLEANFILES+=	${DPSRCS} ${YHEADER:D${SRCS:M*.y:.y=.h}} \
		machine ${MACHINE_CPU} tmp.o

# see below why this is necessary
.if ${MACHINE} == "sun2" || ${MACHINE} == "sun3"
CLEANFILES+=	sun68k
.elif ${MACHINE} == "sparc64"
CLEANFILES+=	sparc
.elif ${MACHINE} == "i386"
CLEANFILES+=	x86
.elif ${MACHINE} == "amd64"
CLEANFILES+=	x86
CFLAGS+=	-mcmodel=kernel
.elif ${MACHINE_ARCH} == "powerpc"
CLEANFILES+=	${KMOD}_tramp.o ${KMOD}_tramp.S tmp.S ${KMOD}_tmp.o
.endif

OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}
PROG?=		${KMOD}.o
MAN?=		${KMOD}.4

##### Build rules
realall:	${PROG}

${OBJS}:	${DPSRCS}

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

# XXX.  This should be done a better way.  It's @'d to reduce visual spew.
# XXX   .BEGIN is used to make sure the links are done before anything else.
.if make(depend) || make(all) || make(dependall)
.BEGIN:
	@rm -f machine && \
	    ln -s $S/arch/${MACHINE}/include machine
	@rm -f ${MACHINE_CPU} && \
	    ln -s $S/arch/${MACHINE_CPU}/include ${MACHINE_CPU}
# XXX. it gets worse..
.if ${MACHINE} == "sun2" || ${MACHINE} == "sun3"
	@rm -f sun68k && \
	    ln -s $S/arch/sun68k/include sun68k
.endif
.if ${MACHINE} == "sparc64"
	@rm -f sparc && \
	    ln -s $S/arch/sparc/include sparc
.endif
.if ${MACHINE} == "amd64"
	@rm -f x86 && \
	    ln -s $S/arch/x86/include x86
.endif
.if ${MACHINE_CPU} == "i386"
	@rm -f x86 && \
	    ln -s $S/arch/x86/include x86
.endif
.endif

##### Install rules
.if !target(kmodinstall)
_PROG:=		${DESTDIR}${KMODDIR}/${PROG}		# installed path

.if !defined(UPDATE)
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
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.sys.mk>

.-include "$S/arch/${MACHINE_CPU}/include/Makefile.inc"
.-include "$S/arch/${MACHINE}/include/Makefile.inc"
