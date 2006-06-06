#	$NetBSD: Makefile,v 1.17 2006/06/06 15:41:50 christos Exp $

CPPFLAGS=	${APMCPPFLAGS}
KSRC=${.CURDIR}/../../..
ARCHSRC=${.CURDIR}/../..
I386=${KSRC}/arch/i386
X86=${KSRC}/arch/x86
CPPFLAGS+=-I${ARCHSRC} -I${KSRC} -I${.CURDIR} -I${.OBJDIR} -D_LKM -D_KERNEL

SRCS=	biostramp.S
OBJS=	biostramp.o
DEPS=	${APMDEPS}
STRIP?=	strip
TRAMP=	${.CURDIR}/biostramp.inc

all: ${TRAMP}

${TRAMP}: biostramp.bin Makefile
	@([ -f ${TRAMP} ] && mv -f ${TRAMP} ${TRAMP}.bak; exit 0)
	${HOST_SH} ${.CURDIR}/bin2asm.sh biostramp.bin > ${.TARGET}

machine:
	@rm -f machine && ln -s ${I386}/include machine

x86:
	@rm -f x86 && ln -s ${X86}/include x86

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF"
biostramp.bin: biostramp.obj
	${OBJCOPY} --output-target=binary $? $@
.else
biostramp.bin: biostramp.obj
	cp $? $@
	@${STRIP} $@
	@${HOST_SH} ${.CURDIR}/rmaouthdr $@ $@.tmp
	@if [ `ls -l $@.tmp |awk '{print $$5}'` -gt 4096 ]; then \
		echo "$@ too big"; \
		exit 1; \
	fi
	@mv -f $@.tmp $@
.endif

biostramp.obj: ${OBJS} ${DEPS}
	${LD} -Bstatic -N -Ttext 0 -e do_bios_call -o $@ $(OBJS)

assym.h: machine x86 genassym.cf
	${TOOL_GENASSYM} -- ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    < ${.CURDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

biostramp.S: assym.h

biostramp.o: biostramp.S
	${CC} ${CPPFLAGS} -DASM -D_LOCORE -c ${.IMPSRC}

clean:
	rm -f *.o biostramp.obj biostramp.bin biostramp.sym ${TRAMP} \
	    assym.h ${TRAMP}.bak machine

.include <bsd.dep.mk>
.include <bsd.subdir.mk>
.include <bsd.obj.mk>
