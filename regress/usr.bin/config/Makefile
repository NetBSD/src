# $NetBSD: Makefile,v 1.5 2005/12/18 19:44:07 cube Exp $

# Those configurations only have to be understood by config(1) without
# errors.

SIMPLE_SUCCESS=	SHADOW_INSTANCE LOOP LOOP2 PSEUDO_PARENT
SIMPLE_FAILURE=	POSTPONED_ORPHAN

SIMPLE=	${SIMPLE_SUCCESS} ${SIMPLE_FAILURE}

CONFIG_OPTS.POSTPONED_ORPHAN=	-v

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

.if !empty(SIMPLE_SUCCESS:M${__conf__})
CONFIG_TEST.${__conf__}=	&& echo "Test passed."
.else
CONFIG_TEST.${__conf__}=	|| echo "Test passed."
.endif

CONFIG_LINE.${__conf__}=	${TOOL_CONFIG} ${CONFIG_OPTS.${__conf__}} \
			    -s ${.CURDIR}/support \
			    -b ./compile/${__conf__} ${.CURDIR}/${__conf__} \
			    ${CONFIG_TEST.${__conf__}}

.PHONY: ${__conf__}-regress ${__conf__}-clean

${__conf__}-regress:
	@mkdir -p compile/${__conf__}
	@${CONFIG_LINE.${__conf__}}

${__conf__}-clean:
	@if [ -d compile/${__conf__} ]; then rm -Rf compile/${__conf__}; fi
.endfor
