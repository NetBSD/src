#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/4/93
#	$NetBSD: Makefile.inc,v 1.43 1998/01/30 23:38:12 perry Exp $

# string sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/string ${.CURDIR}/string

SRCS+=	bm.c strcasecmp.c strcoll.c strdup.c strerror.c \
	strmode.c strsignal.c strtok.c strtok_r.c strxfrm.c \
	__strerror.c __strsignal.c

# machine-dependent net sources
# m-d Makefile.inc must include sources for:
#	bcmp() bcopy() bzero() ffs() index() memchr() memcmp() memset()
#	rindex() strcat() strcmp() strcpy() strcspn() strlen()
#	strncat() strncmp() strncpy() strpbrk() strsep()
#	strspn() strstr() swav()
# m-d Makefile.inc may include sources for:
#	memcpy() memmove() strchr() strrchr()

.include "${.CURDIR}/arch/${MACHINE_ARCH}/string/Makefile.inc"

# if no machine specific memccpy(3), use the machine independent version.
.if empty(SRCS:Mmemccpy.S)
SRCS+=	memccpy.c
.endif

# if no machine specific memmove(3), build one out of bcopy(3).
.if empty(SRCS:Mmemmove.S)
OBJS+=	memmove.o
memmove.o: bcopy.c
	@echo ${COMPILE.c:Q} -DMEMMOVE ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DMEMMOVE ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

memmove.po: bcopy.c
	@echo ${COMPILE.c:Q} -DMEMMOVE -pg ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DMEMMOVE -pg ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

memmove.so: bcopy.c
	@echo ${COMPILE.c:Q} -DMEMMOVE ${CPICFLAGS} ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DMEMMOVE ${CPICFLAGS} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

LOBJS+= memmove.ln
memmove.ln: bcopy.c
	${LINT} -DMEMMOVE ${LINTFLAGS} ${CPPFLAGS:M-[IDU]*} -i -o ${.TARGET} \
	    ${.ALLSRC}
.endif

# if no machine specific memcpy(3), build one out of bcopy(3).
# if there is a machine specific memmove(3), we'll assume it aliases
# memcpy(3).
.if empty(SRCS:Mmemcpy.S)
.if empty(SRCS:Mmemmove.S)
OBJS+=	memcpy.o
memcpy.o: bcopy.c
	@echo ${COMPILE.c:Q} -DMEMCOPY ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DMEMCOPY ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

memcpy.po: bcopy.c
	@echo ${COMPILE.c:Q} -DMEMCOPY -pg ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DMEMCOPY -pg ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

memcpy.so: bcopy.c
	@echo ${COMPILE.c:Q} -DMEMCOPY ${CPICFLAGS} ${.ALLSRC} -o ${.TARGET}
	@${COMPILE.c} -DMEMCOPY ${CPICFLAGS} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o

LOBJS+= memcpy.ln
memcpy.ln: bcopy.c
	${LINT} -DMEMCOPY ${LINTFLAGS} ${CPPFLAGS:M-[IDU]*} -i -o ${.TARGET} \
	    ${.ALLSRC}
.endif
.endif

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

MAN+=	bm.3 bcmp.3 bcopy.3 bstring.3 bzero.3 ffs.3 index.3 memccpy.3 memchr.3 \
	memcmp.3 memcpy.3 memmove.3 memset.3 rindex.3 strcasecmp.3 strcat.3 \
	strchr.3 strcmp.3 strcoll.3 strcpy.3 strcspn.3 strerror.3 \
	string.3 strlen.3 strmode.3 strdup.3 strpbrk.3 strrchr.3 strsep.3 \
	strsignal.3 strspn.3 strstr.3 strtok.3 strxfrm.3 swab.3

MLINKS+=bm.3 bm_comp.3 bm.3 bm_exec.3 bm.3 bm_free.3
MLINKS+=strcasecmp.3 strncasecmp.3
MLINKS+=strcat.3 strncat.3
MLINKS+=strcmp.3 strncmp.3
MLINKS+=strcpy.3 strncpy.3
