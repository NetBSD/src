#	$Id: Makefile,v 1.1.1.2 2000/05/03 09:27:19 itojun Exp $

DIR=	smm/08.sendmailop
SRCS=	op.me
OBJS=	op.ps
MACROS=	-me
ROFF_CMD=	groff
PIC_CMD=	pic
EQN_CMD=	eqn
PIC=		${PIC_CMD} -C
EQN=		${EQN_CMD} -C -Tps
ROFF=		${ROFF_CMD} -Tps -mps ${MACROS}

all: ${OBJS}

${OBJS}: ${SRCS}
	rm -f $@
	${PIC} ${SRCS} | ${EQN} | ${ROFF} > $@

clean:
	rm -f ${OBJS}

install: ${OBJS}
