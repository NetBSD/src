#	$NetBSD: Makefile,v 1.106 2024/06/08 09:09:19 rillig Exp $

.include <bsd.own.mk>

PROG=		lint1
SRCS=		cgram.y \
		ckbool.c ckctype.c ckgetopt.c cksnprintb.c debug.c \
		decl.c emit.c emit1.c err.c func.c init.c inittyp.c lex.c \
		main1.c mem.c mem1.c oper.c scan.l tree.c tyname.c

MAN=		lint.7
YHEADER=
#DBG=		-g
#CPPFLAGS+=	-DYYDEBUG=1
#YFLAGS+=	-v
#LFLAGS+=	-d

CWARNFLAGS.clang+=	-Wno-error=implicit-int-float-conversion
LOBJS.${PROG}+=		${SRCS:M*.y:.y=.ln}
LOBJS.${PROG}+=		${SRCS:M*.l:.l=.ln}
LINTFLAGS.cgram.c+=	-X 351		# 'extern' declaration in .c file
LINTFLAGS.scan.c+=	-X 107,126,330,331,332,333	# strict bool mode
LINTFLAGS.scan.c+=	-X 192,214	# due to suppressed bool errors
LINTFLAGS.scan.c+=	-X 307		# static variable unused
LINTFLAGS.scan.c+=	-X 351		# 'extern' declaration in .c file

CPPFLAGS+=	-DIS_LINT1
CPPFLAGS+=	-I${.CURDIR} -I${.OBJDIR}
CPPFLAGS+=	${DEBUG:D-DDEBUG -DYYDEBUG}
CPPFLAGS+=	${DEBUG_MEM:D-DDEBUG_MEM}

COPTS.err.c+=	${${ACTIVE_CC} == "clang":? -Wno-format-nonliteral :}

BINDIR=		/usr/libexec

CLEANFILES+=	${MAN} ${MAN}.date

${MAN}.date:	err.c
	${_MKTARGET_CREATE}
	${TOOL_SED} -E \
	    -e 's,.*(....)/([0-9]{2})/0?([0-9]+).*,\2 \3\, \1,' \
	    -e 's,^01,January,' \
	    -e 's,^02,February,' \
	    -e 's,^03,March,' \
	    -e 's,^04,April,' \
	    -e 's,^05,May,' \
	    -e 's,^06,June,' \
	    -e 's,^07,July,' \
	    -e 's,^08,August,' \
	    -e 's,^09,September,' \
	    -e 's,^10,October,' \
	    -e 's,^11,November,' \
	    -e 's,^12,December,' \
	    -e 1q \
	    ${.ALLSRC} > ${.TARGET}

# Extra -UYYDEBUG for lint's strict bool mode, since cgram.c contains 'int
# yydebug; if (yydebug)'.
cgram.ln: cgram.c
	${LINT} ${LINTFLAGS} ${LINTFLAGS.${.IMPSRC}} \
	    ${CPPFLAGS:C/-([IDUW]) */-\1/Wg:M-[IDUW]*} \
	    -i -UYYDEBUG ${.IMPSRC}

${MAN}:		makeman err.c Makefile ${MAN}.date
	${_MKTARGET_CREATE}
	SED=${TOOL_SED:Q} ${HOST_SH} ${.ALLSRC:M*makeman} \
	    "$$(cat ${.ALLSRC:M*.date})" ${.ALLSRC:M*err.c} \
	    >${.TARGET}

LDADD+=		-lm
.ifndef HOSTPROG
DPADD+=		${LIBM}

LDADD+=		-ll
DPADD+=		${LIBL}
.endif

err-msgs.h: err.c
	${_MKTARGET_CREATE}
	sp='[[:space:]]*'; \
	from="^$$sp\(\"[^\"].*\"\)\,$$sp// \(Q*[0-9][0-9]*\)\$$"; \
	${TOOL_SED} -n -e "s,$$from,#define MSG_\2 \1,p" < ${.ALLSRC:M*err.c} > ${.TARGET}.tmp
	mv -f ${.TARGET}.tmp ${.TARGET}

CLEANFILES+=	err-msgs.h
DPSRCS+=	err-msgs.h
CPPFLAGS+=	-I.

add-test: .PHONY
	@cd '../../../tests/usr.bin/xlint/lint1' && ${MAKE} add-test

.include <bsd.prog.mk>
