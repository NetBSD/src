#include <sys/types.h>
#include <machine/busses.h>
#include <machine/cpu.h>
#include <sys/param.h>
#include <sys/device.h>

extern int ufs_mountroot();
int (*mountroot)() = ufs_mountroot;

extern struct bus_driver kbddriver;
extern struct bus_driver zsdriver;

struct bus_ctlr bus_master_init[] = {
	{0}
};

struct bus_device bus_device_init[] = {
	{&zsdriver,  "zs",  0, 0, 0, 0, 0, '?', 0, 0, -1},
	{&zsdriver,  "zs",  1, 0, 0, 0, 0, '?', 0, 0, -1},
	{0}
};

old_configure()
{
	struct bus_ctlr *master;
	struct bus_device *device;
	struct bus_driver *driver;
	caddr_t phys;
	int adpt;

	/* Go through the bus controllers and configure them */
	for( master = bus_master_init; master->driver != NULL; ++master ){
		driver = master->driver;
		phys = master->phys_address;
		if( phys == 0 )
			master->phys_address = phys = driver->addr[master->unit];
		master->address = (caddr_t) IIOV(phys);
		adpt = ((unsigned) phys - 0xE00000) >> 16;
		configure_bus_master(master->name, master->address,
				     phys, adpt, "iio");
	}

	/* Go through the bus devices and configure them */
	for( device = bus_device_init; device->driver; ++device ){
		driver = device->driver;
		phys = device->phys_address;
		if( phys == 0 )
			device->phys_address = phys = driver->addr[device->unit];
		device->address = (caddr_t) IIOV(phys);
		adpt = ((unsigned) phys - 0xE00000) >> 16;
		configure_bus_device(device->name, device->address,
				     phys, adpt, "iio");
	}
}

int
never_match(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    return 0;
}

struct cfdriver clockcd = {
    NULL, "clock", never_match, NULL, DV_DULL, sizeof(struct device), 0
};
struct cfdriver zscd = {
    NULL, "zs", never_match, NULL, DV_DULL, sizeof(struct device), 0
};
