# $Makefile$

URCU:= ${.PARSEDIR}
URCU_DIST= ${URCU}/dist

.PATH.c: ${URCU_DIST}/src

CPPFLAGS+=	-DHAVE_CONFIG_H -include ${URCU}/include/urcu/config.h
CPPFLAGS+=	-I${URCU}/include -I${URCU_DIST}/include
CFLAGS+=	-pthread
LDFLAGS+=	-pthread
