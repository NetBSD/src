#	$NetBSD: bsd.kmod.mk,v 1.43 2001/11/02 05:21:50 tv Exp $

.include <bsd.init.mk>

##### Basic targets
.PHONY:		cleankmod kmodinstall load unload
beforedepend:	machine-links
clean:		cleankmod
realinstall:	kmodinstall

##### Default values
S?=		/sys
KERN=		$S/kern

CFLAGS+=	${COPTS} -D_KERNEL -D_LKM -I. -I${.CURDIR} -I$S -I$S/arch

DPSRCS+=	${SRCS:M*.l:.l=.c} ${SRCS:M*.y:.y=.c}
CLEANFILES+=	${DPSRCS} ${YHEADER:D${SRCS:M*.y:.y=.h}} \
		machine ${MACHINE_CPU}

OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}
PROG?=		${KMOD}.o
MAN?=		${KMOD}.4

##### Build rules
realall:	machine-links ${PROG}
.ORDER:		machine-links ${PROG}

${PROG}: ${DPSRCS} ${OBJS} ${DPADD}
	${LD} -r ${LDFLAGS} -o tmp.o ${OBJS}
	mv tmp.o ${.TARGET}

# XXX.  This should be done a better way.  It's @'d to reduce visual spew.
machine-links:
	@rm -f machine && \
	    ln -s $S/arch/${MACHINE}/include machine
	@rm -f ${MACHINE_CPU} && \
	    ln -s $S/arch/${MACHINE_CPU}/include ${MACHINE_CPU}

##### Install rules
.if !target(kmodinstall)
_PROG:=		${DESTDIR}${KMODDIR}/${PROG}		# installed path

${_PROG}: ${PROG}					# install rule
	${INSTALL_FILE} -o ${KMODOWN} -g ${KMODGRP} -m ${KMODMODE} \
		${.ALLSRC} ${.TARGET}

kmodinstall::	${_PROG}
.PRECIOUS:	${_PROG}				# keep if install fails
.PHONY:		${UPDATE:U${_PROG}}			# clobber unless UPDATE
.if !defined(BUILD) && !make(all) && !make(${PROG})
${_PROG}:	.MADE					# no build at install
.endif

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
