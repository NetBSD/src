/*	$NetBSD: acpireg.h,v 1.2.8.3 2002/06/23 17:45:03 jdolecek Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file defines various ACPI event messages, etc.
 */

/*
 * 5.6.3: Device Object Notifications
 */

/* Device Object Notification Types */
#define	ACPI_NOTIFY_BusCheck		0x00
#define	ACPI_NOTIFY_DeviceCheck		0x01
#define	ACPI_NOTIFY_DeviceWake		0x02
#define	ACPI_NOTIFY_EjectRequest	0x03
#define	ACPI_NOTIFY_DeviceCheckLight	0x04
#define	ACPI_NOTIFY_FrquencyMismatch	0x05
#define	ACPI_NOTIFY_BusModeMismatch	0x06
#define	ACPI_NOTIFY_PowerFault		0x07
				/*	0x08 - 0x7f	reserved */

/* Control Method Battery Device Notification Types */
#define	ACPI_NOTIFY_BatteryStatusChanged	0x80
#define	ACPI_NOTIFY_BatteryInformationChanged	0x81

/* Power Source Object Notification Types */
#define	ACPI_NOTIFY_PowerSourceStatusChanged	0x80

/* Thermal Zone Object Notication Types */
#define	ACPI_NOTIFY_ThermalZoneStatusChanged	0x80
#define	ACPI_NOTIFY_ThermalZoneTripPointsChanged 0x81
#define	ACPI_NOTIFY_DeviceListsChanged		0x82

/* Control Method Power Button Notification Types */
#define	ACPI_NOTIFY_S0PowerButtonPressed	0x80

/* Control Method Sleep Button Notification Types */
#define	ACPI_NOTIFY_S0SleepButtonPressed	0x80

/* Control Method Lid Notification Types */
#define	ACPI_NOTIFY_LidStatusChanged		0x80

/* Processor Device Notification Values */
#define	ACPI_NOTIFY_PerformancePresentCapabiltitesChanged 0x80
#define	ACPI_NOTIFY_CStatesChanged		0x81

/*
 * 6: Configuration
 *
 * 6.1: Device Identification Objects
 *
 *	_ADR	Object that evaluates to a device's address on
 *		its parent bus.
 *
 *	_CID	Object that evaluates to a device's Plug and Play
 *		compatible ID list.
 *
 *	_DDN	Object that associates a logical software name
 *		(for example, COM1) with a device.
 *
 *	_HID	Object that evaluates to a device's Plug and Play
 *		hardware ID.
 *
 *	_SUN	Objcet that evaluates to the slot-unique ID number
 *		for a slot.
 *
 *	_STR	Object that contains a Unicode identifier for a device.
 *
 *	_UID	Object that specifies a device's unique persistent ID,
 *		or a control method that generates it.
 */

/*
 * 6.1.1: _ADR (Address)
 *
 *	EISA		EISA slot numnber 0-f
 *
 *	Floppy Bus	Drive select values used for programming
 *			the floppy controller to access the
 *			specified INT13 unit number.  The _ADR
 *			objects should be sorted based on drive
 *			select encoding from 0-3.
 *
 *	IDE controller	0 - primary channel, 1 - secondary channel
 *
 *	IDE channel	0 - master drive, 1 - slave drive
 *
 *	PCI		High word - Device #, Low word - Function #
 *			0xffff == all functions on a device
 *
 *	PCMCIA		Socket #; 0 == first socket
 *
 *	PC Card		Socket #; 0 == first socket
 *
 *	SMBus		Lowest slave address
 *
 *	USB Root Hub	Only one child of the host controller, must
 *			have an _ADR of 0.
 *
 *	USB ports	port number
 */
#define	ACPI_ADR_PCI_DEV(x)	(((x) >> 16) & 0xffff)
#define	ACPI_ADR_PCI_FUNC(x)	((x) & 0xffff)
#define	ACPI_ADR_PCI_ALLFUNCS	0xffff

/*
 * 6.1.2: _CID (Compatible ID)
 */

/*
 * 6.1.3: _DDN (Device Name)
 */

/*
 * 6.1.4: _HID (Hardware ID)
 */

/*
 * 6.1.5: _STR (String)
 */

/*
 * 6.1.6: _SUN (Slot User Number)
 */

/*
 * 6.1.7: _UID (Unique ID)
 */

/*
 * 6.2: Device Configuration Objects
 *
 *	_CRS	Object that specifies a device's *current* resource
 *		settings, or a control method that generates such
 *		an object.
 *
 *	_DIS	Control method that disables a device.
 *
 *	_DMA	Object that specifies a device's *current* resources
 *		for DMA transactions.
 *
 *	_FIX	Object used to provide correlation between the
 *		fixed-hardware register blocks defined in the FADT
 *		and the devices that implement these fixed-hardware
 *		registers.
 *
 *	_HPP	Object that specifies the cache-line size, latency
 *		timer, SERR enable, and PERR enable values to be
 *		used when configuring a PCI device inserted into
 *		a hot-plug slot or initial configuration of a PCI
 *		device at system boot.
 *
 *	_MAT	Object that evaluates to a buffer of MADT APIC
 *		structure entries.
 *
 *	_PRS	An object that specifies a device's *possible*
 *		resource settings, or a control method that
 *		generates such an object.
 *
 *	_PRT	Object that specifies the PCI interrupt routing
 *		table.
 *
 *	_PXM	Object that specifies a proximity domain for a device.
 *
 *	_SRS	Control method that sets a device's settings.
 */

/*
 * 6.2.1: _CRS (Current Resource Settings)
 */

/*
 * 6.2.2: _DIS (Disable)
 */

/*
 * 6.2.3: _DMA (Direct Memory AccesS)
 */

/*
 * 6.2.4: _FIX (Fixed Register Resource Provider)
 */

/*
 * 6.2.5: _HPP (Hot Plug Parameters)
 */

/*
 * 6.2.6: _MAT (Multiple APIC Table Entry)
 */

/*
 * 6.2.7: _PRS (Possible Resource Settings)
 */

/*
 * 6.2.8: _PRT (PCI Routing Table)
 */

/*
 * 6.2.9: _PXM (Proximity)
 */

/*
 * 6.2.10: _SRS (Set Resource Settings)
 */

/*
 * 6.3: Device Insertion and Removal Objects
 *
 *	_EDL	Object that evaluates to a package of namespace
 *		references of device objects that depend on
 *		the device containing _EDL.  Whenever the named
 *		devices is ejected, OSPM ejects all dependent
 *		devices.
 *
 *	_EJD	Object that evaluates to the name of a device object
 *		on which a device depends.  Whenever the named
 *		device is ejected, the dependent device must receive
 *		an ejection notification.
 *
 *	_EJx	Control method that ejects a device.
 *
 *	_LCK	Control method that locks or unlocks a device.
 *
 *	_RMV	Object that indicates that the given device is
 *		removable.
 *
 *	_STA	Control method that returns a device's status.
 */

/*
 * 6.3.1: _EDL (Eject Device List)
 */

/*
 * 6.3.2: _EJD (Ejection Dependent Device)
 */

/*
 * 6.3.3: _EJx (Eject)
 *
 *	x	Indicates sleeping state at which device
 *		can be ejected.
 */

/*
 * 6.3.4: _LCK (Lock)
 */

/*
 * 6.3.5: _RMV (Remove)
 */

/*
 * 6.3.6: _STA (Status) for device insertion/removal
 */
#define	ACPI_STA_DEV_PRESENT	0x00000001	/* device present */
#define	ACPI_STA_DEV_ENABLED	0x00000002	/* enabled (decoding res.) */
#define	ACPI_STA_DEV_SHOW	0x00000004	/* show device in UI */
#define	ACPI_STA_DEV_OK		0x00000008	/* functioning properly */
#define	ACPI_STA_DEV_BATT	0x00000010	/* battery present */

/*
 * 6.4: Resource Data Types for ACPI
 *
 *	Used by the _CRS, _PRS, and _SRS methods.
 */

/*
 * 7.1.4: _STA (Status) for power resource current state
 */
#define	ACPI_STA_POW_OFF	0		/* power resource off */
#define	ACPI_STA_POW_ON		1		/* power resource on */

/*
 * ACPI driver components
 */

#define	ACPI_BUS_COMPONENT	0x00010000
#define	ACPI_ACAD_COMPONENT	0x00020000
#define	ACPI_BAT_COMPONENT	0x00040000
#define	ACPI_BUTTON_COMPONENT	0x00080000
#define	ACPI_EC_COMPONENT	0x00100000
#define	ACPI_LID_COMPONENT	0x00200000
#define	ACPI_RESOURCE_COMPONENT	0x00400000
