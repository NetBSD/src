#	$NetBSD: Makefile,v 1.1.1.2.8.2 2014/08/19 23:58:06 tls Exp $

USE_SHLIBDIR=	yes
REQUIRETOOLS=	yes
NOLINT=		# defined

LIB=		gcc_s
NOSTATICLIB=	# defined
NOPROFILE=	# defined
NOPICINSTALL=	# defined

.include "../../../Makefile.gcc_path"
.include <bsd.own.mk>

.if ${MKGCC} != "no"

.cc: # disable .cc->NULL transform

SHLIB_MAJOR=	1
SHLIB_MINOR=	0

LDFLAGS+=	-nodefaultlibs
LDFLAGS+=	-Wl,--version-script=${.OBJDIR}/libgcc.map

# XXX handle this better?
.if ${MACHINE} == "emips"
G_LIB2ADD_HACK+=	${GNUHOSTDIST}/gcc/config/floatunsidf.c \
			${GNUHOSTDIST}/gcc/config/floatunsisf.c
.endif

SRCS+=		${LIB2FUNCS} ${LIB2DIVMOD} ${LIB2_EH} ${LIB2_EHASM} \
		${G_LIB2ADD:T:S/.asm/.S/} ${G_LIB2ADD_HACK:T:S/.asm/.S/} \
		${LIB1ASMFUNCS}

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
