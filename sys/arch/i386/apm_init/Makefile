#	$NetBSD: Makefile,v 1.4 1996/09/09 18:07:40 mycroft Exp $

CPPFLAGS=	${APMCPPFLAGS}

SRCS=	biostramp.S
OBJS=	biostramp.o
DEPS=	${APMDEPS}

biostramp.inc: biostramp Makefile
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

clean:
	rm -f *.o biostramp biostramp.sym biostramp.inc

.include <bsd.dep.mk>
.include <bsd.subdir.mk>
#.include <bsd.prog.mk>
