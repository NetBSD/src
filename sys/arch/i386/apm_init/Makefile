#	$NetBSD: Makefile,v 1.5 1996/09/13 00:02:36 jtk Exp $

CPPFLAGS=	${APMCPPFLAGS}
CFLAGS+=	-I${.CURDIR}/../.. -I${.CURDIR}/../../..

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
	@if [ `${LS} -l biostramp |awk '{print $$5}'` -gt 4096 ]; then \
		echo "biostramp too big"; \
		exit 1; \
	fi
	@sh ${.CURDIR}/rmaouthdr biostramp biostramp.tmp
	@mv -f biostramp.tmp biostramp

assym.h: genassym
	./genassym > assym.h

biostramp.S: assym.h
biostramp.o: biostramp.S
	${CC} ${CFLAGS} -DASM -D_LOCORE -c biostramp.S

clean:
	rm -f *.o biostramp biostramp.sym biostramp.inc genassym assym.h

.include <bsd.dep.mk>
.include <bsd.subdir.mk>
#.include <bsd.prog.mk>
