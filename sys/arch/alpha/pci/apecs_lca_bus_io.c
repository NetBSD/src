
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/bus.h>

#include <alpha/pci/apecsreg.h>
#include <alpha/pci/lcareg.h>

#if (APECS_PCI_SIO != LCA_PCI_SIO)
#error Sparse I/O addresses do not match up?
#endif

#define	CHIP		apecs_lca
#define	CHIP_IO_BASE	APECS_PCI_SIO

#include "pcs_bus_io_common.c"
