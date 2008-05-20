#	$NetBSD: bsd.kmodule.mk,v 1.4 2008/05/20 12:08:05 ad Exp $

.include <bsd.init.mk>
.include <bsd.klinks.mk>

##### Basic targets
clean:		cleankmod
realinstall:	kmodinstall

KERN=		$S/kern

CFLAGS+=	-ffreestanding -fno-zero-initialized-in-bss ${COPTS}
CPPFLAGS+=	-nostdinc -I. -I${.CURDIR} -isystem $S -isystem $S/arch
CPPFLAGS+=	-isystem ${S}/../common/include
CPPFLAGS+=	-D_KERNEL -D_LKM -D_MODULE

# XXX until the kernel is fixed again...
.if ${HAVE_GCC} == 4
CFLAGS+=	-fno-strict-aliasing -Wno-pointer-sign
.endif

_YKMSRCS=	${SRCS:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS:M*.y:.y=.h}}
DPSRCS+=	${_YKMSRCS}
CLEANFILES+=	${_YKMSRCS}
CLEANFILES+=	tmp.o

OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}
PROG?=		${KMOD}.kmod

##### Build rules
realall:	${PROG}

${OBJS} ${LOBJS}: ${DPSRCS}

${PROG}: ${OBJS} ${DPADD}
	${_MKTARGET_LINK}
	${LD} -r -d -o tmp.o ${OBJS}
	mv tmp.o ${.TARGET}

##### Install rules
.if !target(kmodinstall)
KMODINSTDIR=	${DESTDIR}${KMODULEDIR}/${KMOD}
_PROG:=		${KMODINSTDIR}/${PROG} # installed path

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
	${INSTALL_DIR} ${KMODINSTDIR}
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

##### Pull in related .mk logic
.include <bsd.man.mk>
.include <bsd.links.mk>
.include <bsd.sys.mk>
.include <bsd.dep.mk>

.-include "$S/arch/${MACHINE_CPU}/include/Makefile.inc"
.-include "$S/arch/${MACHINE}/include/Makefile.inc"
