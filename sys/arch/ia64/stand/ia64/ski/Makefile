S=	${.CURDIR}/../../../../..

.include "../Makefile.booters"

NOPIE=		yes
PROG=		skiload
MAN=#
BINDIR?=	/usr/mdec

STRIP=		# We must not strip skiload at install time.

SRCS=		conf.c copy.c delay.c devicename.c efi_stub.c exit.c \
		main.c skiconsole.c pal_stub.S sal_stub.c skifs.c ssc.c \
		start.S time.c acpi_stub.c bootinfo.c exec.c

CPPFLAGS+=	-DLOADER -DSKIFS
CPPFLAGS+=	-I${S}
CPPFLAGS+=	-I${IA64_STAND_DIR}/common

CPPFLAGS+= 	-DLIBKERN_OPTIMISE_SPACE \
		-DHEAP_VARIABLE

# Always add MI sources
.PATH: ${.CURDIR}/../../common
.include "${.CURDIR}/../../common/Makefile.inc"

LDSCRIPT=	${.CURDIR}/ldscript.${MACHINE_ARCH}
LDFLAGS=	-Wl,-T${LDSCRIPT} -symbolic -nostdlib #-shared

${PROG}: ${LDSCRIPT}

CLEANFILES+=	skiload ${OBJS}

NEWVERSWHAT=	"ia64 SKI boot" ${MACHINE_ARCH}

DPADD=		${LIBLIST}

# Cross dependencies between LIB(SA|KERN). Need to elaborate below.
LDADD=		${LIBLIST} ${LIBLIST}

.include "${S}/conf/newvers_stand.mk"

.include <bsd.prog.mk>
