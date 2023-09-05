#	$NetBSD: Makefile,v 1.5 2023/09/05 04:16:56 mrg Exp $

XCBEXT=	xinput

# XXXGCC12:  avoids
# xinput.c:491:19: error: argument 1 value '4294967294' exceeds maximum object size 2147483647 [-Werror=alloc-size-larger-than=]
#  491 |         xcb_out = malloc(xcb_buffer_len);
# it also does not check the return value of this malloc().
COPTS.xinput.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 12:? -Wno-alloc-size-larger-than :}

.include "../xcb.mk"
