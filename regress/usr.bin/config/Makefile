# $NetBSD: Makefile,v 1.1 2005/10/02 21:27:02 cube Exp $

# Those configurations only have to be understood by config(1) without
# errors.

SIMPLE=	SHADOW_INSTANCE

.include <bsd.obj.mk>

.PHONY: regress clean cleandir
regress: compile
cleandir: clean
clean:
	@if [ -e compile ]; then rmdir compile; fi

compile:
	@mkdir compile

.for __conf__ in ${SIMPLE}
regress: ${__conf__}-regress
clean: ${__conf__}-clean
.endfor

.for __conf__ in ${SIMPLE}
.PHONY: ${__conf__}-regress ${__conf__}-clean
${__conf__}-regress:
	@mkdir compile/${__conf__}
	${TOOL_CONFIG} -s ${.CURDIR}/support -b ./compile/${__conf__} ${.CURDIR}/${__conf__}

${__conf__}-clean:
	@if [ -d compile/${__conf__} ]; then rm -Rf compile/${__conf__}; fi
.endfor
