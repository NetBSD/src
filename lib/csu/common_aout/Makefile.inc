#	$NetBSD: Makefile.inc,v 1.6 2002/11/12 14:33:49 itohy Exp $
#	from: @(#)Makefile	5.5 (Berkeley) 5/21/91

CPPFLAGS+=	-DLIBC_SCCS -I${.CURDIR}/../common_aout
OBJS+=		crt0.o gcrt0.o scrt0.o
SRCS=		crt0.c

.include <bsd.own.mk>
.include <bsd.shlib.mk>

.if ${SHLINKINSTALLDIR} != "/usr/libexec"
OBJS+=		lcrt0.o
.endif

realall: ${OBJS}

crt0.o: ${SRCS}
	@echo "${COMPILE.c} -DCRT0 -DDYNAMIC ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DCRT0 -DDYNAMIC ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

gcrt0.o: ${SRCS}
	@echo "${COMPILE.c} -DMCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DMCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

scrt0.o: ${SRCS}
	@echo "${COMPILE.c} -DSCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DSCRT0 ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

# same as crt0.o modulo the dynamic linker is in /libexec
lcrt0.o: ${SRCS}
	@echo "${COMPILE.c} -DCRT0 -DDYNAMIC -DLDSO=\"${SHLINKINSTALLDIR}/ld.so\" ${.CURDIR}/${SRCS} -o ${.TARGET}"
	@${COMPILE.c} -DCRT0 -DDYNAMIC -DLDSO=\"${SHLINKINSTALLDIR}/ld.so\" ${.CURDIR}/${SRCS} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
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
FILESDIR=${LIBDIR}

FILESDIR_lcrt0.o:=${SHLIBINSTALLDIR}
FILESNAME_lcrt0.o:=crt0.o

.include <bsd.prog.mk>
