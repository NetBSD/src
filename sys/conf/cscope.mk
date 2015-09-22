# $NetBSD: cscope.mk,v 1.1.2.2 2015/09/22 12:05:56 skrll Exp $

##
## cscope
##

EXTRA_CLEAN+= cscope.out cscope.tmp
.if !target(cscope.out)
cscope.out: Makefile depend
	${_MKTARGET_CREATE}
	@${TOOL_SED} 's/[^:]*://;s/^ *//;s/ *\\ *$$//;' lib/kern/.depend \
	    | tr -s ' ' '\n' \
	    | ${TOOL_SED} ';s|^../../||;' \
	    > cscope.tmp
	@${TOOL_SED} 's/[^:]*://;s/^ *//;s/ *\\ *$$//;' lib/compat/.depend \
	    | tr -s ' ' '\n' \
	    | ${TOOL_SED} 's|^../../||;' \
	    >> cscope.tmp
	@echo ${SRCS} | cat - cscope.tmp | tr -s ' ' '\n' | sort -u | \
	    ${CSCOPE} -k -i - -b `echo ${INCLUDES} | ${TOOL_SED} s/-nostdinc//`
#	cscope doesn't write cscope.out if it's uptodate, so ensure
#	make doesn't keep calling cscope when not needed.
	@rm -f cscope.tmp; touch cscope.out
.endif

.if !target(cscope)
cscope: cscope.out
	@${CSCOPE} -d
.endif

EXTRA_CLEAN+= ID
.if !target(mkid)
.PHONY: mkid
mkid: ID

ID: Makefile depend
	${_MKTARGET_CREATE}
	@${MKID} \
	    `${TOOL_SED} 's/[^:]*://;s/^ *//;s/ *\\\\ *$$//;' \
			lib/kern/.depend lib/compat/.depend \
		    | tr ' ' '\n' \
		    | ${TOOL_SED} "s|^../../||" \
		    | sort -u` \
	    `${TOOL_SED} 's/[^:]*://;s/^ *//;s/ *\\\\ *$$//;' \
			.depend \
		    | tr ' ' '\n' \
		    | sort -u`

.endif
