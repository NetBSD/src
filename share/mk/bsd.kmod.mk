#	$NetBSD: bsd.kmod.mk,v 1.18 1997/05/09 07:56:01 mycroft Exp $

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.include <bsd.own.mk>

.MAIN:		all
.PHONY:		cleankmod kmodinstall load unload
install:	kmodinstall
clean cleandir:	cleankmod

S?=		/sys
KERN=		$S/kern

.SUFFIXES: .out .o .c .cc .cxx .C .y .l .s .S

CFLAGS+=	${COPTS} -D_KERNEL -D_LKM -I. -I${.CURDIR} -I$S -I$S/arch

CLEANFILES+=	machine ${MACHINE_ARCH}

DPSRCS+= ${SRCS:M*.h}
OBJS+=	${SRCS:N*.h:R:S/$/.o/g}

.if !defined(PROG)
PROG=	${KMOD}.o
.endif

${PROG}: ${DPSRCS} ${OBJS} ${DPADD}
	${LD} -r ${LDFLAGS} -o tmp.o ${OBJS}
	mv tmp.o ${.TARGET}

.if	!defined(MAN)
MAN=	${KMOD}.4
.endif

all: machine-links ${PROG}

.PHONY:	machine-links
beforedepend: machine-links
machine-links:
	-rm -f machine && \
	    ln -s $S/arch/${MACHINE}/include machine
	-rm -f ${MACHINE_ARCH} && \
	    ln -s $S/arch/${MACHINE_ARCH}/include ${MACHINE_ARCH}

cleankmod:
	rm -f a.out [Ee]rrs mklog core *.core \
		${PROG} ${OBJS} ${LOBJS} ${CLEANFILES}

#
# define various install targets
#
.if !target(kmodinstall)
kmodinstall:: ${DESTDIR}${KMODDIR}/${PROG}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${KMODDIR}/${PROG}
.endif
.if !defined(BUILD)
${DESTDIR}${KMODDIR}/${PROG}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${KMODDIR}/${PROG}
${DESTDIR}${KMODDIR}/${PROG}: ${PROG}
	${INSTALL} ${COPY} -o ${KMODOWN} -g ${KMODGRP} -m ${KMODMODE} \
		${.ALLSRC} ${.TARGET}
.endif

lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	@${LINT} ${LINTFLAGS} ${LDFLAGS:M-L*} ${LOBJS} ${LDADD}
.endif

.if !target(load)
load:	${PROG}
	/sbin/modload -o ${KMOD} -e${KMOD}_lkmentry ${PROG}
.endif

.if !target(unload)
unload: ${PROG}
	/sbin/modunload -n ${KMOD}
.endif

.if !defined(NOMAN)
.include <bsd.man.mk>
.endif

.include <bsd.obj.mk>
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
