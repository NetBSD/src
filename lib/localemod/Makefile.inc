.include <bsd.own.mk>

SHLIB_MAJOR=1
SHLIB_MINOR=0
SHLIBDIR= /usr/lib/runemodule
LIBDIR= /usr/lib/runemodule
INTERNALLIB=yes
MKLINKLIB=no
CFLAGS+= -Wall -I${.CURDIR}/../../../include -I${.CURDIR}/../../libc/locale
CFLAGS+= -DDLRUNEMOD -DRUNEMOD_MAJOR=${SHLIB_MAJOR}
BASENAME!= basename ${.CURDIR}
LIB= ${BASENAME}
SRCS!=echo ${BASENAME}.c | tr A-Z a-z
.PATH: ${.CURDIR}/../../libc/locale

beforeinstall:
	if [ ! -d ${SHLIBDIR} ]; then mkdir ${SHLIBDIR}; fi
.if !defined(OBJECT_FMT) || ${OBJECT_FMT} != "ELF"
SYMLINKS+= \
	lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
		${SHLIBDIR}/lib${LIB}.so.${SHLIB_MAJOR}
SYMLINKS+= \
	lib${LIB}.so.${SHLIB_MAJOR} ${SHLIBDIR}/lib${LIB}.so
.endif
