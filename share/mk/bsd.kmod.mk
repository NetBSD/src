#	$NetBSD: bsd.kmod.mk,v 1.79 2006/05/06 02:20:23 groo Exp $

.include <bsd.init.mk>

##### Basic targets
clean:		cleankmod
realinstall:	kmodinstall

##### Default values
.if !defined(S)
.if defined(NETBSDSRCDIR)
S=		${NETBSDSRCDIR}/sys
.elif defined(BSDSRCDIR)
S=		${BSDSRCDIR}/sys
.else
S=		/sys
.endif
.endif
KERN=		$S/kern

CFLAGS+=	-ffreestanding ${COPTS}
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -isystem $S -isystem $S/arch
CPPFLAGS+=	-isystem ${S}/../common/include
CPPFLAGS+=	-D_KERNEL -D_LKM

_YKMSRCS=	${SRCS:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS:M*.y:.y=.h}}
DPSRCS+=	${_YKMSRCS}
CLEANFILES+=	${_YKMSRCS}
CLEANFILES+=	machine ${MACHINE_CPU} tmp.o

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
.elif ${MACHINE_CPU} == "powerpc" || \
      ${MACHINE_CPU} == "arm"
CLEANFILES+=	${KMOD}_tramp.o ${KMOD}_tramp.S tmp.S ${KMOD}_tmp.o
.endif
.if defined(XEN_BUILD) || ${MACHINE} == "xen"
CLEANFILES+=	xen xen-ma/machine # xen-ma
CPPFLAGS+=	-I${.OBJDIR}/xen-ma
.if ${MACHINE_CPU} == "i386"
CLEANFILES+=	x86
.endif
.endif

OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}
PROG?=		${KMOD}.o
MAN?=		${KMOD}.4

##### Build rules
realall:	${PROG}

${OBJS} ${LOBJS}: ${DPSRCS}

.if ${MACHINE_CPU} == "powerpc" || \
    ${MACHINE_CPU} == "arm"
${KMOD}_tmp.o: ${OBJS} ${DPADD}
	${LD} -r ${LDFLAGS} -o tmp.o ${OBJS}
	mv tmp.o ${.TARGET}

${KMOD}_tramp.S: ${KMOD}_tmp.o $S/lkm/arch/${MACHINE_CPU}/lkmtramp.awk
	${OBJDUMP} --syms --reloc ${KMOD}_tmp.o | \
		 awk -f $S/lkm/arch/${MACHINE_CPU}/lkmtramp.awk > tmp.S
	mv tmp.S ${.TARGET}

${PROG}: ${KMOD}_tmp.o ${KMOD}_tramp.o
	${LD} -r ${LDFLAGS} \
		`${OBJDUMP} --syms --reloc ${KMOD}_tmp.o | \
			 awk -f $S/lkm/arch/${MACHINE_CPU}/lkmwrap.awk` \
		 -o tmp.o ${KMOD}_tmp.o ${KMOD}_tramp.o
.if exists($S/lkm/arch/${MACHINE_CPU}/lkmhide.awk)
	${OBJCOPY} \
		`${NM} tmp.o | awk -f $S/lkm/arch/${MACHINE_CPU}/lkmhide.awk` \
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
.if defined(XEN_BUILD) || ${MACHINE} == "xen"
	@rm -f xen && \
	    ln -s $S/arch/xen/include xen
	@rm -rf xen-ma && mkdir xen-ma && \
	    ln -s ../${XEN_BUILD:U${MACHINE_ARCH}} xen-ma/machine
.endif
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
		${.ALLSRC} ${.TARGET}

kmodinstall::	${_PROG}
.PHONY:		kmodinstall
.PRECIOUS:	${_PROG}				# keep if install fails

.undef _PROG
.endif # !target(kmodinstall)

##### Clean rules
cleankmod: .PHONY
	rm -f a.out [Ee]rrs mklog core *.core \
		${PROG} ${OBJS} ${LOBJS} ${CLEANFILES}

##### Custom rules
lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	${LINT} ${LINTFLAGS} ${LDFLAGS:C/-L[  ]*/-L/Wg:M-L*} ${LOBJS} ${LDADD}
.endif

.if !target(load)
load: ${PROG}
	/sbin/modload ${KMOD_LOADFLAGS} -o ${KMOD} ${PROG}
.endif
.PHONY: load

.if !target(unload)
unload:
	/sbin/modunload -n ${KMOD}
.endif
.PHONY: unload

##### Pull in related .mk logic
.include <bsd.man.mk>
.include <bsd.links.mk>
.include <bsd.sys.mk>
.include <bsd.dep.mk>

.-include "$S/arch/${MACHINE_CPU}/include/Makefile.inc"
.-include "$S/arch/${MACHINE}/include/Makefile.inc"
