#	$NetBSD: bsd.endian.mk,v 1.3 2002/06/05 02:47:56 thorpej Exp $

.include <bsd.init.mk>

# find out endianness of target and set proper flag for pwd_mkdb and such,
# so that it creates database in same endianness.
.if exists(${DESTDIR}/usr/include/sys/endian.h)
TARGET_ENDIANNESS!= \
	printf '\#include <sys/endian.h>\n_BYTE_ORDER\n' | \
	${CC} -nostdinc ${CPPFLAG_ISYSTEM} ${DESTDIR}/usr/include -E - | \
	tail -1 | awk '{print $$1}'
.else
TARGET_ENDIANNESS=
.endif

#.if ${TARGET_ENDIANNESS} == "1234"
#TARGET_ENDIANNESS=	little
#.elif ${TARGET_ENDIANNESS} == "4321"
#TARGET_ENDIANNESS=	big
#.else
#TARGET_ENDIANNESS=	unknown
#.endif
