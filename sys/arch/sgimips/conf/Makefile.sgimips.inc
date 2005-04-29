#	$NetBSD: Makefile.sgimips.inc,v 1.8.4.1 2005/04/29 11:28:21 kent Exp $

WANT_ECOFF?=no

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff; \
		${OBJCOPY} --impure -O ecoff-bigmips \
		  -R .pdr -R .mdebug.abi32 -R .comment -R .ident $@ $@.ecoff
.endif
