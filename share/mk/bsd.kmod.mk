#	$NetBSD: bsd.kmod.mk,v 1.53 2002/04/09 17:14:38 jdolecek Exp $

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
		machine ${MACHINE_CPU}

OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.o/g}
PROG?=		${KMOD}.o
MAN?=		${KMOD}.4

##### Build rules
realall:	${PROG}

${OBJS}:	${DPSRCS}

${PROG}: ${OBJS} ${DPADD}
	${LD} -r ${LDFLAGS} -o tmp.o ${OBJS}
	mv tmp.o ${.TARGET}

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
		${.ALLSRC} ${.TARGET}

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
