#	$NetBSD: Makefile,v 1.38 1998/01/21 21:23:18 cgd Exp $

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
	${COMPILE.c} -DSTRCHR ${.ALLSRC} -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

strchr.po: index.c
	${COMPILE.c} -DSTRCHR -p ${.ALLSRC} -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}

strchr.so: index.c
	${COMPILE.c} ${PICFLAG} -DPIC -DSTRCHR ${.ALLSRC} -o ${.TARGET}

LOBJS+= strchr.ln
strchr.ln: index.c
	${LINT} -DSTRCHR ${LINTFLAGS} ${CPPFLAGS:M-[IDU]*} -i -o ${.TARGET} \
	    ${.ALLSRC}
.endif

# if no machine specific strrchr(3), build one out of rindex(3).
.if empty(SRCS:Mstrrchr.S)
OBJS+=	strrchr.o
strrchr.o: rindex.c
	${COMPILE.c} -DSTRRCHR ${.ALLSRC} -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

strrchr.po: rindex.c
	${COMPILE.c} -DSTRRCHR -p ${.ALLSRC} -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}

strrchr.so: rindex.c
	${COMPILE.c} ${PICFLAG} -DPIC -DSTRRCHR ${.ALLSRC} -o ${.TARGET}

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
