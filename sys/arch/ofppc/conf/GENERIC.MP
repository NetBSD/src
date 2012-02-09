# $NetBSD: GENERIC.MP,v 1.18 2012/02/09 19:04:04 matt Exp $
#
# GENERIC machine description file
# 
# This machine description file is used to generate the default NetBSD
# kernel.  The generic kernel does not include all options, subsystems
# and device drivers, but should be useful for most applications.
#
# The machine description file can be customised for your specific
# machine to reduce the kernel size and improve its performance.
#
# For further information on compiling NetBSD kernels, see the config(8)
# man page.
#
# For further information on hardware support for this architecture, see
# the intro(4) man page.  For further information about kernel options
# for this architecture, see the options(4) man page.  For an explanation
# of each device driver in this file see the section 4 man page for the
# device.

include 	"arch/ofppc/conf/GENERIC"

# SMP options
options 	MULTIPROCESSOR
options 	IPI_OPENPIC
options 	PMAP_NEED_FULL_MAPKERNEL
