#	$NetBSD: Makefile,v 1.7 1998/08/31 23:52:40 jtk Exp $

CPPFLAGS=	${APMCPPFLAGS}
KSRC=${.CURDIR}/../../..
ARCHSRC=${.CURDIR}/../..
I386=${KSRC}/arch/i386
CFLAGS+= -I${ARCHSRC} -I${KSRC} -D_LKM

SRCS=	biostramp.S
OBJS=	biostramp.o
DEPS=	${APMDEPS}
STRIP?= strip

all: biostramp.inc

biostramp.inc: biostramp Makefile
	-mv -f biostramp.inc biostramp.inc.bak
	sh ${.CURDIR}/bin2asm.sh biostramp > biostramp.inc

biostramp: ${OBJS} ${DEPS}
	${LD} -Bstatic -N -T 0 -o biostramp $(OBJS)
	cp biostramp biostramp.sym
	@${STRIP} biostramp
	@if [ `ls -l biostramp |awk '{print $$5}'` -gt 4096 ]; then \
		echo "biostramp too big"; \
		exit 1; \
	fi
	@sh ${.CURDIR}/rmaouthdr biostramp biostramp.tmp
	@mv -f biostramp.tmp biostramp

assym.h: ${KSRC}/kern/genassym.sh genassym.cf
	sh ${KSRC}/kern/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    < genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

biostramp.S: assym.h
biostramp.o: biostramp.S
	${CC} ${CFLAGS} -DASM -D_LOCORE -c biostramp.S

clean:
	rm -f *.o biostramp biostramp.sym biostramp.inc assym.h

.include <bsd.dep.mk>
.include <bsd.subdir.mk>
#.include <bsd.prog.mk>
