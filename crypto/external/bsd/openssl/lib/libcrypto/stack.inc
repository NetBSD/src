#	$NetBSD: stack.inc,v 1.1.58.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/stack

STACK_SRCS += \
stack.c \

SRCS += ${STACK_SRCS}

.for cryptosrc in ${STACK_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/stack ${STACKCPPFLAGS}
.endfor
