
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/bus.h>

#include <alpha/pci/apecsreg.h>
#include <alpha/pci/lcareg.h>

#if (APECS_PCI_SPARSE != LCA_PCI_SPARSE) || (APECS_PCI_DENSE != LCA_PCI_DENSE)
#error Memory addresses do not match up?
#endif

#define	CHIP		apecs_lca
#define	CHIP_D_MEM_BASE	APECS_PCI_DENSE
#define	CHIP_S_MEM_BASE	APECS_PCI_SPARSE

#include "pcs_bus_mem_common.c"
