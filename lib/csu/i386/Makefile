#	$NetBSD: Makefile,v 1.21 1997/05/31 21:21:42 cjs Exp $
#	from: @(#)Makefile	5.5 (Berkeley) 5/21/91

CFLAGS+=	-DLIBC_SCCS -I${.CURDIR}/..
OBJS=		crt0.o gcrt0.o scrt0.o
SRCS=		crt0.c

all: ${OBJS}

crt0.o: ${SRCS}
	@echo "${COMPILE.c} -DCRT0 -DDYNAMIC ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DCRT0 -DDYNAMIC ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

gcrt0.o: ${SRCS}
	@echo "${COMPILE.c} -DMCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DMCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

scrt0.o: ${SRCS}
	@echo "${COMPILE.c} -DSCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DSCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

.if make(depend)
CPPFLAGS+= -DDYNAMIC
.endif

afterdepend: .depend
	@(TMP=/tmp/_depend$$$$; \
		sed -e 's/^\([^\.]*\).o[ ]*:/\1.o g\1.o s\1.o:/' \
		< .depend > $$TMP; \
	mv $$TMP .depend)

FILES=${OBJS}
FILESDIR=/usr/lib

.include <bsd.prog.mk>
