#	$NetBSD: Makefile,v 1.39 1998/01/23 19:01:17 cgd Exp $

LIB=	kern
NOPIC=

M=	${.CURDIR}/arch/${MACHINE_ARCH}

CPPFLAGS=	-I$M ${KERNCPPFLAGS} ${KERNMISCCPPFLAGS}

.if exists ($M/Makefile.inc)
.PATH:	$M
.include "$M/Makefile.inc"
.endif

.if (${MACHINE_ARCH} != "alpha")
# Quad support
SRCS+=	adddi3.c anddi3.c ashldi3.c ashrdi3.c cmpdi2.c divdi3.c iordi3.c \
	lshldi3.c lshrdi3.c moddi3.c muldi3.c negdi2.c notdi2.c qdivrem.c \
	subdi3.c  ucmpdi2.c udivdi3.c umoddi3.c xordi3.c
.endif

# Other stuff
SRCS+=	md5c.c

# Files to clean up
CLEANFILES+= lib${LIB}.o lib${LIB}.po

#
# Special rules for certain objects
#

# mcount cannot be compiled with profiling
mcount.po: mcount.o
	cp mcount.o mcount.po

# if no machine specific strchr(3), build one out of index(3).
.if empty(SRCS:Mstrchr.S)
OBJS+=	strchr.o
strchr.o: index.c
	@echo ${COMPILE.c:Q} -DSTRCHR ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DSTRCHR ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

strchr.po: index.c
	@echo ${COMPILE.c:Q} -DSTRCHR -pg ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DSTRCHR -pg ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

strchr.so: index.c
	@echo ${COMPILE.c:Q} -DSTRCHR ${CPICFLAGS} ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DSTRCHR ${CPICFLAGS} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

LOBJS+= strchr.ln
strchr.ln: index.c
	${LINT} -DSTRCHR ${LINTFLAGS} ${CPPFLAGS:M-[IDU]*} -i -o ${.TARGET} \
	    ${.ALLSRC}
.endif

# if no machine specific strrchr(3), build one out of rindex(3).
.if empty(SRCS:Mstrrchr.S)
OBJS+=	strrchr.o
strrchr.o: rindex.c
	@echo ${COMPILE.c:Q} -DSTRRCHR ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DSTRRCHR ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

strrchr.po: rindex.c
	@echo ${COMPILE.c:Q} -DSTRRCHR -pg ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DSTRRCHR -pg ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

strrchr.so: rindex.c
	@echo ${COMPILE.c:Q} -DSTRRCHR ${CPICFLAGS} ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DSTRRCHR ${CPICFLAGS} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

LOBJS+= strrchr.ln
strrchr.ln: rindex.c
	${LINT} -DSTRRCHR ${LINTFLAGS} ${CPPFLAGS:M-[IDU]*} -i -o ${.TARGET} \
	    ${.ALLSRC}
.endif

# only needed during build
libinstall::

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `NM=${NM} ${LORDER} ${OBJS} | ${TSORT}`

POBJS+=	${OBJS:.o=.po} mcount.po
lib${LIB}.po:: ${POBJS}
	@echo building profiled ${LIB} library
	@rm -f lib${LIB}.po
	@${LD} -r -o lib${LIB}.po `NM=${NM} ${LORDER} ${POBJS} | ${TSORT}`
