#	$NetBSD: Makefile.ibmnws.inc,v 1.1.2.2 2014/05/18 17:45:14 rmind Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo "Convert kernel $@ into NCD firmware image."; \
		${TOOL_NCDCS} $@ $@.ncd; \
		mv $@.ncd $@
