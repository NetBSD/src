
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/bus.h>

#include <alpha/pci/ciareg.h>

#define	CHIP		cia
#define	CHIP_IO_BASE	CIA_PCI_SIO0

#include "pcs_bus_io_common.c"
