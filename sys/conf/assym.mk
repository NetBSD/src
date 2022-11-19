# $NetBSD: assym.mk,v 1.8 2022/11/19 07:54:25 yamt Exp $

GENASSYM_FLAGS=${CFLAGS:N-Wa,*:N-fstack-usage*} ${CPPFLAGS} ${GENASSYM_CPPFLAGS} 

assym.h: ${GENASSYM_CONF} ${GENASSYM_EXTRAS} $S/conf/genassym.cf
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRAS} $S/conf/genassym.cf | \
	    ${GENASSYM} -- ${CC} ${GENASSYM_FLAGS} ${PROF} > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

.if !defined(___USE_SUFFIX_RULES___)
${SRCS:T:M*.[sS]:C|\.[Ss]|.o|}: assym.h
${SRCS:T:M*.[sS]:C|\.[Ss]|.d|}: assym.h
.else
${SRCS:M*.[sS]:C|\.[Ss]|.o|}: assym.h
${SRCS:M*.[sS]:C|\.[Ss]|.d|}: assym.h
.endif

# assym.dep in the below target looks like:
#
#   assym.o: \
#    /var/folders/74/hw1sphgx0lv63q6pq_n5grw00000gn/T//genassym.BCtq6a/assym.c \
#    opt_arm_start.h opt_execfmt.h opt_multiprocessor.h \
#      :
#      :
#
# The following sed modifies it to:
#
#   assym.h: \
#    opt_arm_start.h opt_execfmt.h opt_multiprocessor.h \
#      :
#      :

assym.d: assym.h
	${_MKTARGET_CREATE}
	cat ${GENASSYM_CONF} ${GENASSYM_EXTRAS} | \
	    ${GENASSYM} -- ${MKDEP} -f assym.dep -- ${GENASSYM_FLAGS}
	${TOOL_SED} -e '1{N;s/\\\n//;}' -e 's/.*\.o:.*\.c/assym.h:/' < assym.dep >${.TARGET}
	rm -f assym.dep

DEPS+=	assym.d

.if defined(___USE_SUFFIX_RULES___)
.SUFFIXES: .genassym .assym.h
.genassym.assym.h:
	${_MKTARGET_CREATE}
	${GENASSYM} -- ${CC} ${GENASSYM_FLAGS} ${PROF} < $< > $@
	mv -f $@.tmp $@
.endif # ___USE_SUFFIX_RULES___
