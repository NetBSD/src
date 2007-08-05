#	$NetBSD: bsd.kmod.mk,v 1.84.6.2 2007/08/05 21:43:25 pooka Exp $

.include <bsd.init.mk>
.include <bsd.klinks.mk>

##### Basic targets
clean:		cleankmod
realinstall:	kmodinstall

KERN=		$S/kern

CFLAGS+=	-ffreestanding ${COPTS}
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -isystem $S -isystem $S/arch
CPPFLAGS+=	-isystem ${S}/../common/include
CPPFLAGS+=	-D_KERNEL -D_LKM

# XXX until the kernel is fixed again...
.if ${HAVE_GCC} == 4
CFLAGS+=	-fno-strict-aliasing -Wno-pointer-sign
.endif

_YKMSRCS=	${SRCS:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS:M*.y:.y=.h}}
DPSRCS+=	${_YKMSRCS}
CLEANFILES+=	${_YKMSRCS}
CLEANFILES+=	tmp.o

.if ${MACHINE_CPU} == "powerpc" || \
      ${MACHINE_CPU} == "arm"
CLEANFILES+=	${KMOD}_tramp.o ${KMOD}_tramp.S tmp.S ${KMOD}_tmp.o
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
	${_MKTARGET_COMPILE}
	${LD} -r -o tmp.o ${OBJS}
	mv tmp.o ${.TARGET}

${KMOD}_tramp.S: ${KMOD}_tmp.o $S/lkm/arch/${MACHINE_CPU}/lkmtramp.awk
	${_MKTARGET_CREATE}
	${OBJDUMP} --syms --reloc ${KMOD}_tmp.o | \
		 awk -f $S/lkm/arch/${MACHINE_CPU}/lkmtramp.awk > tmp.S
	mv tmp.S ${.TARGET}

${PROG}: ${KMOD}_tmp.o ${KMOD}_tramp.o
	${_MKTARGET_LINK}
	${LD} -r \
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
	${_MKTARGET_LINK}
	${LD} -r -o tmp.o ${OBJS}
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
	${_MKTARGET_INSTALL}
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
