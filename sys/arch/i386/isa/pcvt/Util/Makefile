SUBDIR= 	keycap cursor fontedit fonts kcon loadfont scon \
		userkeys vttest ispcvt mcon

.if make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=	vgaio kbdio set2061
SUBDIR+=	demo
.endif

#includes:
#	(cd ${.CURDIR}/..; ${MAKE} includes)

.include <bsd.subdir.mk>
