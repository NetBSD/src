PROG=		set2061
SRCS=		main.c ICD2061Aalt.c
CFLAGS+= 	-DGCCUSESGAS -DPCVT_STANDALONE
MKMAN=	no

all: $(PROG)

.include <bsd.prog.mk>

$(PROG):	compiler.h 
