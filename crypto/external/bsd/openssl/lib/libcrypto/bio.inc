#	$NetBSD: bio.inc,v 1.3 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/bio

BIO_SRCS += \
bf_buff.c \
bf_lbuf.c \
bf_nbio.c \
bf_null.c \
bf_prefix.c \
bf_readbuff.c \
bio_addr.c \
bio_cb.c \
bio_dump.c \
bio_err.c \
bio_lib.c \
bio_meth.c \
bio_print.c \
bio_sock.c \
bio_sock2.c \
bss_acpt.c \
bss_bio.c \
bss_conn.c \
bss_core.c \
bss_dgram.c \
bss_fd.c \
bss_file.c \
bss_log.c \
bss_mem.c \
bss_null.c \
bss_sock.c \
ossl_core_bio.c \

SRCS += ${BIO_SRCS}

.for cryptosrc in ${BIO_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/bio ${BIOCPPFLAGS}
.endfor
