#	$NetBSD: Makefile,v 1.11 2001/09/22 03:34:19 tv Exp $

CPPFLAGS=	${APMCPPFLAGS}
KSRC=${.CURDIR}/../../..
ARCHSRC=${.CURDIR}/../..
I386=${KSRC}/arch/i386
CFLAGS+= -I${ARCHSRC} -I${KSRC} -D_LKM

SRCS=	biostramp.S
OBJS=	biostramp.o
DEPS=	${APMDEPS}
STRIP?=	strip

all: biostramp.inc

biostramp.inc: biostramp.bin Makefile
	-mv -f biostramp.inc biostramp.inc.bak
	sh ${.CURDIR}/bin2asm.sh biostramp.bin > biostramp.inc

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF"
biostramp.bin: biostramp.obj
	${OBJCOPY} --output-target=binary $? $@
.else
biostramp.bin: biostramp.obj
	cp $? $@
	@${STRIP} $@
	@sh ${.CURDIR}/rmaouthdr $@ $@.tmp
	@if [ `ls -l $@.tmp |awk '{print $$5}'` -gt 4096 ]; then \
		echo "$@ too big"; \
		exit 1; \
	fi
	@mv -f $@.tmp $@
.endif

biostramp.obj: ${OBJS} ${DEPS}
	${LD} -Bstatic -N -Ttext 0 -o $@ $(OBJS)

assym.h: ${KSRC}/kern/genassym.sh genassym.cf
	sh ${KSRC}/kern/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    < genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

biostramp.S: assym.h
biostramp.o: biostramp.S
	${CC} ${CFLAGS} -DASM -D_LOCORE -c biostramp.S

clean:
	rm -f *.o biostramp.bin biostramp.sym biostramp.inc assym.h

.include <bsd.dep.mk>
.include <bsd.subdir.mk>
#.include <bsd.prog.mk>
