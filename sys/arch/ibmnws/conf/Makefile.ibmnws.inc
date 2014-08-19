#	$NetBSD: Makefile.ibmnws.inc,v 1.1.10.2 2014/08/20 00:03:09 tls Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo "Convert kernel $@ into NCD firmware image."; \
		${TOOL_NCDCS} $@ $@.ncd; \
		mv $@.ncd $@
