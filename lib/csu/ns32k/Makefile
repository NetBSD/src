#	$NetBSD: Makefile,v 1.13 1997/05/29 15:57:47 veego Exp $
#	from: @(#)Makefile	5.5 (Berkeley) 5/21/91

CFLAGS+=	-DLIBC_SCCS -I${.CURDIR}/..
OBJS=		crt0.o gcrt0.o scrt0.o
SRCS=		crt0.c

.include	<bsd.own.mk>	# for OBJDIR and BUILDDIR

all: ${OBJS}

crt0.o: ${SRCS}
	@echo "${COMPILE.c} -DCRT0 -DDYNAMIC ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DCRT0 -DDYNAMIC ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

gcrt0.o: ${SRCS}
	@echo "${COMPILE.c} -DMCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DMCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

scrt0.o: ${SRCS}
	@echo "${COMPILE.c} -DSCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DSCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o
.if defined(OBJDIR)
	@echo install -d ${BUILDDIR}${LIBDIR}
	@install -d ${BUILDDIR}${LIBDIR}
	@echo ${BUILDDIR}${LIBDIR}/${.TARGET} '->' `pwd`/${.TARGET};
	@rm -f ${BUILDDIR}${LIBDIR}/${.TARGET};
	@ln -s `pwd`/${.TARGET} ${BUILDDIR}${LIBDIR}/${.TARGET};
.endif

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
