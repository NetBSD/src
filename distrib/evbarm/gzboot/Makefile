#	$NetBSD: Makefile,v 1.1 2002/03/28 22:03:00 thorpej Exp $

# List the kernels you want to Gzip Boot here.
SUBDIR+= kern_IQ80310
SUBDIR+= kern_IQ80321

# List the Gzip Boot ELF images you want to create here.
SUBDIR+= gzboot_IQ80310_flash_0x00080000
SUBDIR+= gzboot_IQ80321_flash_0xf0080000

SUBDIR+= .WAIT

# List the Gzip Boot binary images (which include the kernels) you
# want to create here.
SUBDIR+= gzimg_IQ80310_flash_0x00080000
SUBDIR+= gzimg_IQ80321_flash_0xf0080000

.include <bsd.subdir.mk>
