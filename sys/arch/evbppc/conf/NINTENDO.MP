# $NetBSD: NINTENDO.MP,v 1.1.2.2 2026/11/04 23:20:35 perseant Exp $
#
# Nintendo Wii U
#

include 	"arch/evbppc/conf/NINTENDO"

# SMP options
options 	MULTIPROCESSOR

# Enable Espresso specific workarounds
options 	PPC_IBMESPRESSO

# The workaround for Espresso's stwcx. errata is not compatible with
# PaX MPROTECT.
no options 	PAX_MPROTECT
