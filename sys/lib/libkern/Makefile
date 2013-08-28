#	$NetBSD: Makefile,v 1.95.28.1 2013/08/28 23:59:35 rmind Exp $

LIB=		kern
LIBISPRIVATE?=	yes

.include "Makefile.libkern"
.ifndef ARCHSUBDIR
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `NM=${NM} ${LORDER} ${OBJS} | ${TSORT}`

lib${LIB}.po:: ${POBJS}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}.po
	@${LD} -r -o lib${LIB}.po `NM=${NM} ${LORDER} ${POBJS} | ${TSORT}`

showsources: ${SRCS}
	@echo ${.ALLSRC}
