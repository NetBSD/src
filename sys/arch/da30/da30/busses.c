/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 *	File: busses.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	4/90
 *
 *	Generic autoconfiguration functions,
 *	usable to probe and attach devices
 *	on any bus that suits the generic bus
 *	structure, such as VME, TURBOChannel,
 *	and all the VAX busses.
 *
 *	$Id: busses.c,v 1.1 1994/02/22 23:49:24 paulus Exp $
 */

#include <sys/types.h>
#include <machine/busses.h>


/*
 * configure_bus_master
 *
 *	Given the name of a bus_ctlr, look it up in the
 *	init table.  If found, probe it.  If there can be
 *	slaves attached, walk the device's init table
 *	for those that might be attached to this controller.
 *	Call the 'slave' function on each one to see if
 *	ok, then the 'attach' one.
 *
 * Returns 0 if the controller is not there.
 *
 */
configure_bus_master( name, virt, phys, adpt_no, bus_name)
	char		*name;
	caddr_t		 virt;
	caddr_t		 phys;
	int		 adpt_no;
	char		*bus_name;
{
	register struct bus_device *device;
	register struct bus_ctlr *master;
	register struct bus_driver *driver;

	int             found = 0;

	/*
	 * Match the name in the table, then pick the entry that has the
	 * right adaptor number, or one that has it wildcarded.  Entries
	 * already allocated are marked alive, skip them. 
	 */
	for (master = bus_master_init; master->driver; master++) {
		if (master->alive)
			continue;
		if (((master->adaptor == adpt_no) || (master->adaptor == '?')) &&
		    (strcmp(master->name, name) == 0)) {
			found = 1;
			break;
		}
	}

	if (!found)
		return 0;

	/*
	 * Found a match, probe it 
	 */
	driver = master->driver;
	if ((*driver->probe) (virt, master) == 0)
		return 0;

	master->alive = 1;
	master->adaptor = adpt_no;

	/*
	 * Remember which controller this device is attached to 
	 */
	driver->minfo[master->unit] = master;

	printf("%s%d: at %s%d\n", master->name, master->unit, bus_name, adpt_no);

	/*
	 * Now walk all devices to check those that might be attached to this
	 * controller.  We match the unallocated ones that have the right
	 * controller number, or that have a widcarded controller number. 
	 */
	for (device = bus_device_init; device->driver; device++) {
		int	ctlr;
		if (device->alive || device->driver != driver ||
		    (device->adaptor != '?' && device->adaptor != adpt_no))
			continue;
		ctlr = device->ctlr;
		if (ctlr == '?') device->ctlr = master->unit;
		/*
		 * A matching entry. See if the slave-probing routine is
		 * happy. 
		 */
		if ((device->ctlr != master->unit) ||
		    ((*driver->slave) (device, virt) == 0)) {
			device->ctlr = ctlr;
			continue;
		}

		device->alive = 1;
		device->adaptor = adpt_no;
		device->ctlr = master->unit;

		/*
		 * Save a backpointer to the controller 
		 */
		device->mi = master;

		/*
		 * ..and to the device 
		 */
		driver->dinfo[device->unit] = device;

		if (device->slave >= 0)
			printf(" %s%d: at %s%d slave %d",
			       device->name, device->unit,
			       driver->mname, master->unit, device->slave);
		else
			printf(" %s%d: at %s%d",
			       device->name, device->unit,
			       driver->mname, master->unit);

		/*
		 * Now attach this slave 
		 */
		(*driver->attach) (device);
		printf("\n");
	}
	return 1;
}

/*
 * configure_bus_device
 *
 *	Given the name of a bus_device, look it up in the
 *	init table.  If found, probe it.  If it is present,
 *	call the driver's 'attach' function.
 *
 * Returns 0 if the device is not there.
 *
 */
configure_bus_device( name, virt, phys, adpt_no, bus_name)
	char		*name;
	caddr_t		 virt;
	caddr_t		 phys;
	u_long 		 adpt_no;
	char		*bus_name;
{
	register struct bus_device *device;
	register struct bus_ctlr *master;
	register struct bus_driver *driver;

	int             found = 0;

	/*
	 * Walk all devices to find one with the right name
	 * and adaptor number (or wildcard).  The entry should
	 * be unallocated, and also the slave number should
	 * be wildcarded.
	 */
	for (device = bus_device_init; device->driver; device++) {
		if (device->alive)
			continue;
		if (((device->adaptor == adpt_no) || (device->adaptor == '?')) &&
		    (device->slave == -1) &&
		    ((!device->phys_address) ||
		     ((device->phys_address == phys) && (device->address == virt))) &&
		    (strcmp(device->name, name) == 0)) {
			found = 1;
			break;
		}
	}

	if (!found)
		return 0;

	/*
	 * Found an entry, probe the device
	 */
	driver = device->driver;
	if ((*driver->probe) (virt, device) == 0)
		return 0;

	device->alive = 1;
	device->adaptor = adpt_no;

	printf("%s%d: at %s%d", device->name, device->unit, bus_name, adpt_no);

	/*
	 * Remember which driver this device is attached to 
	 */
	driver->dinfo[device->unit] = device;

	/*
	 * Attach the device
	 */
	(*driver->attach) (device);
	printf("\n");

	return 1;
}

