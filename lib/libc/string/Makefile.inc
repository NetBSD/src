#	from: @(#)Makefile.inc	5.6 (Berkeley) 3/5/91
#	$Id: Makefile.inc,v 1.29 1994/08/02 05:01:20 jtc Exp $

# string sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/string ${.CURDIR}/string

SRCS+=	bm.c memccpy.c strcasecmp.c strcoll.c strdup.c strerror.c \
	strftime.c strmode.c strsignal.c strtok.c strxfrm.c 

.if   (${MACHINE_ARCH} == "m68k")
SRCS+=	bcmp.S bcopy.S bzero.S ffs.S index.S memchr.c memcmp.S memset.S \
	rindex.S strcat.c strcmp.S strcpy.S strcspn.c strlen.S \
	strncat.c strncmp.S strncpy.S strpbrk.c strsep.c \
	strspn.c strstr.c swab.S
SRCS+=	memcpy.S memmove.S strchr.S strrchr.S
.elif (${MACHINE_ARCH} == "i386")
SRCS+=	bcmp.S bcopy.S bzero.S ffs.S index.S memchr.S memcmp.S memset.S \
	rindex.S strcat.S strcmp.S strcpy.S strcspn.c strlen.S \
	strncat.c strncmp.S strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.S
SRCS+=	memcpy.S memmove.S strchr.S strrchr.S
.elif (${MACHINE_ARCH} == "ns32k")
SRCS+=	bcmp.c bcopy.c bzero.c ffs.c index.c memchr.c memcmp.c memset.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c strlen.c \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
#SRCS+=	memmove.c strchr.c strrchr.c
.elif (${MACHINE_ARCH} == "tahoe")
SRCS+=	bcmp.S bcopy.S bzero.S ffs.S index.c memchr.c memcmp.S memset.c \
	rindex.c strcat.S strcmp.S strcpy.S strcspn.c strlen.S \
	strncat.S strncmp.S strncpy.S strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
SRCS+=	memmove.S
.elif (${MACHINE_ARCH} == "vax")
SRCS+=	bcmp.S bcopy.S bzero.S ffs.S index.c memchr.S memcmp.S memset.S \
	rindex.S strcat.S strcmp.S strcpy.S strcspn.S strlen.S \
	strncat.S strncmp.S strncpy.S strpbrk.S strsep.S \
	strspn.S strstr.S swab.c
SRCS+=	memmove.S strchr.S strrchr.S
.elif (${MACHINE_ARCH} == "sparc")
SRCS+=	bcmp.c bcopy.c bzero.S ffs.S index.c memchr.c memcmp.c memset.c \
	rindex.c strcat.c strcmp.c strcpy.c strcspn.c strlen.S \
	strncat.c strncmp.c strncpy.c strpbrk.c strsep.c \
	strspn.c strstr.c swab.c
.endif

# if no machine specific memmove(3), build one out of bcopy(3).
.if empty(SRCS:Mmemmove.S)
OBJS+=	memmove.o
memmove.o: bcopy.c
	${CC} -DMEMMOVE ${CFLAGS} ${CPPFLAGS} -c ${.ALLSRC} -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

memmove.po: bcopy.c
	${CC} -DMEMMOVE ${CFLAGS} ${CPPFLAGS} -c -p ${.ALLSRC} -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}

memmove.so: bcopy.c
	${CC} ${PICFLAG} -DPIC -DMEMMOVE ${CFLAGS} ${CPPFLAGS} -c ${.ALLSRC} \
		-o ${.TARGET}
.endif

# if no machine specific memcpy(3), build one out of bcopy(3).
# if there is a machine specific memmove(3), we'll assume it aliases
# memcpy(3).
.if empty(SRCS:Mmemcpy.S)
.if empty(SRCS:Mmemmove.S)
OBJS+=	memcpy.o
memcpy.o: bcopy.c
	${CC} -DMEMCOPY ${CFLAGS} ${CPPFLAGS} -c ${.ALLSRC} -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

memcpy.po: bcopy.c
	${CC} -DMEMCOPY ${CFLAGS} ${CPPFLAGS} -c -p ${.ALLSRC} -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}

memcpy.so: bcopy.c
	${CC} ${PICFLAG} -DPIC -DMEMCOPY ${CFLAGS} ${CPPFLAGS} -c ${.ALLSRC} \
		-o ${.TARGET}
.endif
.endif

# if no machine specific strchr(3), build one out of index(3).
.if empty(SRCS:Mstrchr.S)
OBJS+=	strchr.o
strchr.o: index.c
	${CC} -DSTRCHR ${CFLAGS} ${CPPFLAGS} -c ${.ALLSRC} -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

strchr.po: index.c
	${CC} -DSTRCHR ${CFLAGS} ${CPPFLAGS} -c -p ${.ALLSRC} -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}

strchr.so: index.c
	${CC} ${PICFLAG} -DPIC -DSTRCHR ${CFLAGS} ${CPPFLAGS} -c ${.ALLSRC} \
	    -o ${.TARGET}
.endif

# if no machine specific strrchr(3), build one out of rindex(3).
.if empty(SRCS:Mstrrchr.S)
OBJS+=	strrchr.o
strrchr.o: rindex.c
	${CC} -DSTRRCHR ${CFLAGS} ${CPPFLAGS} -c ${.ALLSRC} -o ${.TARGET}
	@${LD} -x -r ${.TARGET}
	@mv a.out ${.TARGET}

strrchr.po: rindex.c
	${CC} -DSTRRCHR ${CFLAGS} ${CPPFLAGS} -c -p ${.ALLSRC} -o ${.TARGET}
	@${LD} -X -r ${.TARGET}
	@mv a.out ${.TARGET}

strrchr.so: rindex.c
	${CC} ${PICFLAG} -DPIC -DSTRRCHR ${CFLAGS} ${CPPFLAGS} -c ${.ALLSRC} \
	    -o ${.TARGET}
.endif

MAN3+=	bm.0 bcmp.0 bcopy.0 bstring.0 bzero.0 ffs.0 index.0 memccpy.0 memchr.0 \
	memcmp.0 memcpy.0 memmove.0 memset.0 rindex.0 strcasecmp.0 strcat.0 \
	strchr.0 strcmp.0 strcoll.0 strcpy.0 strcspn.0 strerror.0 strftime.0 \
	string.0 strlen.0 strmode.0 strdup.0 strpbrk.0 strrchr.0 strsep.0 \
	strsignal.0 strspn.0 strstr.0 strtok.0 strxfrm.0 swab.0

MLINKS+=bm.3 bm_comp.3 bm.3 bm_exec.3 bm.3 bm_free.3
MLINKS+=strcasecmp.3 strncasecmp.3
MLINKS+=strcat.3 strncat.3
MLINKS+=strcmp.3 strncmp.3
MLINKS+=strcpy.3 strncpy.3
