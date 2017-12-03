# $NetBSD: mdroot.mk,v 1.1.18.2 2017/12/03 11:36:57 jdolecek Exp $

.if defined(MEMORY_DISK_IMAGE)
md_root_image.h: ${MEMORY_DISK_IMAGE}
	${_MKTARGET_CREATE}
	${TOOL_HEXDUMP} -v -e '"\t" 8/1 "0x%02x, " "\n"' ${.ALLSRC} > ${.TARGET}
md_root.o: md_root_image.h
.endif
