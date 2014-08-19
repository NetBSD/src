#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:52:13 tls Exp $
#

LIBISPRIVATE=yes

.include "../../Makefile.inc"

LIB=smb
SRCS+=	rcfile.c ctx.c cfopt.c subr.c nls.c rap.c mbuf.c rq.c file.c \
	print.c kiconv.c nb.c nb_name.c nb_net.c nbns_rq.c smb_kernelops.c

.PATH:	${SMBDIST}/lib/smb

.for f in ctx mbuf nb_name nbns_rq
COPTS.${f}.c+=       -Wno-pointer-sign
.endfor

.include <bsd.lib.mk>
