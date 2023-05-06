#	$NetBSD: stack.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/stack

STACK_SRCS += \
stack.c \

SRCS += ${STACK_SRCS}

.for cryptosrc in ${STACK_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/stack ${STACKCPPFLAGS}
.endfor
