#	$NetBSD: bsd.kmod.mk,v 1.42 2001/10/05 15:30:06 simonb Exp $

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.depall.mk>
.MAIN:		all
.endif

.PHONY:		cleankmod kmodinstall load unload
realinstall:	kmodinstall
clean:		cleankmod

S?=		/sys
KERN=		$S/kern

CFLAGS+=	${COPTS} -D_KERNEL -D_LKM -I. -I${.CURDIR} -I$S -I$S/arch

DPSRCS+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${DPSRCS}
.if defined(YHEADER)
CLEANFILES+=	${SRCS:M*.y:.y=.h}
.endif

OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}

.if !defined(PROG)
PROG=	${KMOD}.o
.endif

${PROG}: ${DPSRCS} ${OBJS} ${DPADD}
	${LD} -r ${LDFLAGS} -o tmp.o ${OBJS}
	mv tmp.o ${.TARGET}

.if	!defined(MAN)
MAN=	${KMOD}.4
.endif

realall: machine-links ${PROG}

.PHONY:	machine-links
beforedepend: machine-links
machine-links:
	-rm -f machine && \
	    ln -s $S/arch/${MACHINE}/include machine
	-rm -f ${MACHINE_CPU} && \
	    ln -s $S/arch/${MACHINE_CPU}/include ${MACHINE_CPU}
CLEANFILES+=machine ${MACHINE_CPU}

cleankmod:
	rm -f a.out [Ee]rrs mklog core *.core \
		${PROG} ${OBJS} ${LOBJS} ${CLEANFILES}

#
# define various install targets
#
.if !target(kmodinstall)
kmodinstall:: ${DESTDIR}${KMODDIR}/${PROG}
.PRECIOUS: ${DESTDIR}${KMODDIR}/${PROG}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${KMODDIR}/${PROG}
.endif

__kmodinstall: .USE
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${INSTPRIV} -o ${KMODOWN} \
	    -g ${KMODGRP} -m ${KMODMODE} ${.ALLSRC} ${.TARGET}

.if !defined(BUILD) && !make(all) && !make(${PROG})
${DESTDIR}${KMODDIR}/${PROG}: .MADE
.endif
${DESTDIR}${KMODDIR}/${PROG}: ${PROG} __kmodinstall
.endif

lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	${LINT} ${LINTFLAGS} ${LDFLAGS:M-L*} ${LOBJS} ${LDADD}
.endif

.if !target(load)
load:	${PROG}
	/sbin/modload ${KMOD_LOADFLAGS} -o ${KMOD} ${PROG}
.endif

.if !target(unload)
unload:
	/sbin/modunload -n ${KMOD}
.endif

.include <bsd.man.mk>
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.sys.mk>

.-include "$S/arch/${MACHINE_CPU}/include/Makefile.inc"
.-include "$S/arch/${MACHINE}/include/Makefile.inc"
