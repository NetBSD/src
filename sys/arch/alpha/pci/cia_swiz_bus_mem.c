
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/bus.h>

#include <alpha/pci/ciareg.h>

#define	CHIP		cia
#define	CHIP_D_MEM_BASE	CIA_PCI_DENSE
#define	CHIP_S_MEM_BASE	CIA_PCI_SPARSE0

#include "pcs_bus_mem_common.c"
