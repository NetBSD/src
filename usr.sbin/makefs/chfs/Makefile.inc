CHFS=	${NETBSDSRCDIR}/sys/ufs/chfs

.PATH:	${.CURDIR}/chfs ${NETBSDSRCDIR}/sys/ufs/chfs

CPPFLAGS+= -I${CHFS}

SRCS+=	chfs_mkfs.c
LDADD+= -lz
DPADD+= ${LIBZ}
