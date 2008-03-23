#	Makefile.landisk.inc,v 1.2 2007/01/13 22:53:54 freza Exp

.if ${OBJECT_FMT} == "ELF"
LINKFORMAT=	-N
.endif
