/*	$NetBSD: acpireg.h,v 1.1 2001/09/28 02:09:23 thorpej Exp $	*/

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
 * 6.3.6: _STA (Status) for device insertion/removal
 * 7.1.4: _STA (Status) for power resource current state
 */
#define	ACPI_STA_DEV_PRESENT	0x00000001	/* device present */
#define	ACPI_STA_DEV_ENABLED	0x00000002	/* enabled (decoding res.) */
#define	ACPI_STA_DEV_SHOW	0x00000004	/* show device in UI */
#define	ACPI_STA_DEV_OK		0x00000008	/* functioning properly */
#define	ACPI_STA_DEV_BATT	0x00000010	/* battery present */

#define	ACPI_STA_POW_OFF	0		/* power resource off */
#define	ACPI_STA_POW_ON		1		/* power resource on */
