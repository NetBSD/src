# $NetBSD: assym.mk,v 1.5.2.2 2015/09/22 12:05:56 skrll Exp $

assym.h: ${GENASSYM_CONF} ${GENASSYM_EXTRAS} $S/conf/genassym.cf
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRAS} $S/conf/genassym.cf | \
	    ${GENASSYM} -- ${CC} ${CFLAGS:N-Wa,*} ${CPPFLAGS} ${PROF} \
	    ${GENASSYM_CPPFLAGS} > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

.if !defined(___USE_SUFFIX_RULES___)
${SRCS:T:M*.[sS]:C|\.[Ss]|.o|}: assym.h
${SRCS:T:M*.[sS]:C|\.[Ss]|.d|}: assym.h
.else
${SRCS:M*.[sS]:C|\.[Ss]|.o|}: assym.h
${SRCS:M*.[sS]:C|\.[Ss]|.d|}: assym.h
.endif

assym.d: assym.h
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRAS} | \
	    ${GENASSYM} -- ${MKDEP} -f assym.dep -- \
	    ${CFLAGS:N-Wa,*} ${CPPFLAGS} ${GENASSYM_CPPFLAGS}
	${TOOL_SED} -e 's/.*\.o:.*\.c/assym.h:/' < assym.dep >${.TARGET}
	rm -f assym.dep

DEPS+=	assym.d

.if defined(___USE_SUFFIX_RULES___)
.SUFFIXES: .genassym .assym.h
.genassym.assym.h:
	${_MKTARGET_CREATE}
	${GENASSYM} -- ${CC} ${CFLAGS:N-Wa,*} ${CPPFLAGS} ${PROF} \
	    ${GENASSYM_CPPFLAGS} < $< > $@
	mv -f $@.tmp $@
.endif # ___USE_SUFFIX_RULES___
