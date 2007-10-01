#	$NetBSD: largepages.inc,v 1.1.2.1 2007/10/01 14:47:31 yamt Exp $
#
# Options to create a kernel suitable for mapping with large
# pages.
#

makeoptions	KERN_LDSCRIPT="kern.ldscript.2MB"
options 	LARGEPAGES		# Use large pages for kernel image
