#	$NetBSD: Makefile,v 1.9 2009/11/30 13:07:38 uebayasi Exp $

USE_SHLIBDIR=	yes
REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_s
NOSTATICLIB=	# defined
NOPROFILE=	# defined
NOPICINSTALL=	# defined

.include <bsd.own.mk>

.if ${MKGCC} != "no"

.cc: # disable .cc->NULL transform

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

LDFLAGS+=	-nodefaultlibs
LDFLAGS+=	-Wl,--version-script=${.OBJDIR}/libgcc.map

.if !exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

SRCS+=		${LIB2FUNCS} ${LIB2DIVMOD} ${LIB2_EH} \
		${G_LIB2ADD:T:S/.asm/.S/} ${LIB1ASMFUNCS}

.else	# exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

# New style.
TMPLSRCS=	${TMPLSRCS.libgcc_s.so}
TMPLFPSRCS=	${TMPLFPSRCS.libgcc_s.so}
TMPLASMSRCS=	${TMPLASMSRCS.libgcc_s.so}
SRCS=		${SRCS.libgcc_s.so}

.endif	# exists(${.CURDIR}/../arch/${MACHINE_ARCH}/libs.lib${LIB}.srcs.mk)

COPTS.unwind-dw2.c = -Wno-stack-protector

DPADD+=		libgcc.map
CLEANFILES+=	libgcc.map

.include <bsd.lib.mk>

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
			${TOOL_SED} -e "/^[ 	]*#/d" \
			    -e 's/^%\(if\|else\|elif\|endif\|define\)/#\1/' | \
	  		${CC} ${G_LIBGCC2_CFLAGS} ${G_INCLUDES} -E -xassembler-with-cpp -; \
	} | ${TOOL_AWK} -f ${G_SHLIB_MKMAP} > ${.TARGET}.tmp
	mv ${.TARGET}.tmp ${.TARGET} 

.else
.include <bsd.prog.mk> # do nothing
.endif
