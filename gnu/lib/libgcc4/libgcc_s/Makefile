#	$NetBSD: Makefile,v 1.1 2006/05/10 22:53:50 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_s
NOSTATICLIB=	# defined
NOPROFILE=	# defined
NOPICINSTALL=	# defined

.include <bsd.own.mk>

.if ${MKGCC} != "no"
.if exists(${.CURDIR}/../arch/${MACHINE_ARCH}.mk)
.include "${.CURDIR}/../arch/${MACHINE_ARCH}.mk"
.endif

.cc: # disable .cc->NULL transform

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

LDFLAGS+=	-nodefaultlibs
LDFLAGS+=	-Wl,--version-script=${.OBJDIR}/libgcc.map

SRCS+=		${LIB2FUNCS} ${LIB2DIVMOD} ${LIB2_EH} \
		${G_LIB2ADD:T:S/.asm/.S/} ${LIB1ASMFUNCS}

DPADD+=		libgcc.map
CLEANFILES+=	libgcc.map

.include <bsd.lib.mk>

${LIB2_EH}: unwind.h
unwind.h: ${G_UNWIND_H}
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	ln -s ${G_UNWIND_H} ${.TARGET}
CLEANFILES+= unwind.h

.if ${MKPICLIB} != "no"
LIBGCC_S_OBJS=	${SOBJS}
.else
LIBGCC_S_OBJS=	${OBJS}
.endif

libgcc.map:	${LIBGCC_S_OBJS} ${G_SHLIB_MKMAP} ${G_SHLIB_MAPFILES}
	{ \
		${NM} ${G_SHLIB_NM_FLAGS} ${LIBGCC_S_OBJS}; \
		echo %%; \
		cat ${G_SHLIB_MAPFILES} | \
			sed -e "/^[ 	]*#/d" -e 's/^%\(if\|else\|elif\|endif\|define\)/#\1/' | \
	  		${CC} ${G_LIBGCC2_CFLAGS} ${G_INCLUDES} -E -xassembler-with-cpp -; \
	} | ${AWK} -f ${G_SHLIB_MKMAP} > ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET} 

.else
.include <bsd.prog.mk> # do nothing
.endif
