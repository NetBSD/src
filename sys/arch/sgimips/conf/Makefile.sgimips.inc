#	$NetBSD: Makefile.sgimips.inc,v 1.7.8.2 2005/03/04 16:39:09 skrll Exp $

WANT_ECOFF?=no

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff; \
		${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff
.endif
