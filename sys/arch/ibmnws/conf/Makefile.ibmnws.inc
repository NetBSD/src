#	$NetBSD: Makefile.ibmnws.inc,v 1.1.4.2 2014/05/22 11:39:54 yamt Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo "Convert kernel $@ into NCD firmware image."; \
		${TOOL_NCDCS} $@ $@.ncd; \
		mv $@.ncd $@
