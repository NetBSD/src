/* $NetBSD: smbus_acpi.c,v 1.1 2010/02/06 20:10:18 pgoyette Exp $ */

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ACPI SMBus Controller driver
 *
 * See http://smbus.org/specs/smbus_cmi10.pdf for specifications
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smbus_acpi.c,v 1.1 2010/02/06 20:10:18 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/mutex.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/i2c/i2cvar.h>

#define _COMPONENT          ACPI_SMBUS_COMPONENT
ACPI_MODULE_NAME            ("acpi_smbus")

/*
 * ACPI SMBus CMI protocol codes
 */
#define	ACPI_SMBUS_RD_QUICK	0x03
#define	ACPI_SMBUS_RCV_BYTE	0x05
#define	ACPI_SMBUS_RD_BYTE	0x07
#define	ACPI_SMBUS_RD_WORD	0x09
#define	ACPI_SMBUS_RD_BLOCK	0x0B
#define	ACPI_SMBUS_WR_QUICK	0x02
#define	ACPI_SMBUS_SND_BYTE	0x04
#define	ACPI_SMBUS_WR_BYTE	0x06
#define	ACPI_SMBUS_WR_WORD	0x08
#define	ACPI_SMBUS_WR_BLOCK	0x0A
#define	ACPI_SMBUS_PROCESS_CALL	0x0C

struct acpi_smbus_softc {
	struct acpi_devnode 	*sc_devnode;
	struct callout		sc_callout;
	struct i2c_controller	sc_i2c_tag;
	device_t		sc_dv;
	kmutex_t		sc_i2c_mutex;
	int			sc_poll_alert;
};

static int	acpi_smbus_match(device_t, cfdata_t, void *);
static void	acpi_smbus_attach(device_t, device_t, void *);
static int	acpi_smbus_detach(device_t, int); 
static int	acpi_smbus_acquire_bus(void *, int);
static void	acpi_smbus_release_bus(void *, int);
static int	acpi_smbus_exec(void *, i2c_op_t, i2c_addr_t, const void *,
				size_t, void *, size_t, int);
static void	acpi_smbus_alerts(struct acpi_smbus_softc *);
static void	acpi_smbus_tick(void *);
static void	acpi_smbus_notify_handler(ACPI_HANDLE, UINT32, void *);

struct SMB_UDID {
	uint8_t		dev_cap;
	uint8_t		vers_rev;
	uint16_t	vendor;
	uint16_t	device;
	uint16_t	interface;
	uint16_t	subsys_vendor;
	uint16_t	subsys_device;
	uint8_t		reserved[4];
};

struct SMB_DEVICE {
	uint8_t		slave_addr;
	uint8_t		reserved;
	struct SMB_UDID	dev_id;
};

struct SMB_INFO {
	uint8_t		struct_ver;
	uint8_t		spec_ver;
	uint8_t		hw_cap;
	uint8_t		poll_int;
	uint8_t		dev_count;
	struct SMB_DEVICE device[1];
};

static const char * const smbus_acpi_ids[] = {
	"SMBUS01",	/* SMBus CMI v1.0 */
	NULL
};

static const char * const pcibus_acpi_ids[] = {
	"PNP0A03",
	"PNP0A08",
	NULL
};

CFATTACH_DECL_NEW(acpismbus, sizeof(struct acpi_smbus_softc),
    acpi_smbus_match, acpi_smbus_attach, acpi_smbus_detach, NULL);

/*
 * acpi_smbus_match: autoconf(9) match routine
 */
static int
acpi_smbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;
	int r = 0;
	ACPI_STATUS rv;
	ACPI_BUFFER smi_buf;
	ACPI_OBJECT *e, *p;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	if (acpi_match_hid(aa->aa_node->ad_devinfo, smbus_acpi_ids) == 0)
		return 0;

	/* Ensure that device's CMI version is supported */
	rv = acpi_eval_struct(aa->aa_node->ad_handle, "_SBI", &smi_buf);
	if (ACPI_FAILURE(rv))
		goto done;

	p = smi_buf.Pointer;
	if (p != NULL && p->Type == ACPI_TYPE_PACKAGE &&
	    p->Package.Count >= 1) {
		e = p->Package.Elements;
		if (e[0].Type == ACPI_TYPE_INTEGER &&
		    e[0].Integer.Value == 0x10)
			r = 1;
	}
done:
	if (smi_buf.Pointer != NULL)
		ACPI_FREE(smi_buf.Pointer);

	return r;
}

/*
 * acpitz_attach: autoconf(9) attach routine
 */
static void
acpi_smbus_attach(device_t parent, device_t self, void *aux)
{
	struct acpi_smbus_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct i2cbus_attach_args iba;
	ACPI_STATUS rv;
	ACPI_HANDLE native_dev, native_bus;
	ACPI_DEVICE_INFO *native_dev_info, *native_bus_info;
	ACPI_BUFFER smi_buf;
	ACPI_OBJECT *e, *p;
	struct SMB_INFO *info;
	int pci_bus, pci_dev, pci_func;

	aprint_naive("\n");

	sc->sc_devnode = aa->aa_node;
	sc->sc_dv = self;
	sc->sc_poll_alert = 2;

	/* Attach I2C bus */
	mutex_init(&sc->sc_i2c_mutex, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = acpi_smbus_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = acpi_smbus_release_bus;
	sc->sc_i2c_tag.ic_exec = acpi_smbus_exec;

	/* Retrieve polling interval for SMBus Alerts */
	rv = acpi_eval_struct(aa->aa_node->ad_handle, "_SBI", &smi_buf);
	if (!ACPI_FAILURE(rv)) {
		p = smi_buf.Pointer;
		if (p != NULL && p->Type == ACPI_TYPE_PACKAGE &&
		    p->Package.Count >= 2) {
			e = p->Package.Elements;
			if (e[1].Type == ACPI_TYPE_BUFFER) {
				info = (struct SMB_INFO *)(e[1].Buffer.Pointer);
				sc->sc_poll_alert = info->poll_int;
			}
		}
	}
	if (smi_buf.Pointer != NULL)
		ACPI_FREE(smi_buf.Pointer);

	/* Install notify handler if possible */
	rv = AcpiInstallNotifyHandler(sc->sc_devnode->ad_handle,
		ACPI_DEVICE_NOTIFY, acpi_smbus_notify_handler, self);
	if (ACPI_FAILURE(rv)) {
		aprint_error(": unable to install DEVICE NOTIFY handler: %s\n",
		    AcpiFormatException(rv));
		sc->sc_poll_alert = 2;	/* If failed, fall-back to polling */
	}

	callout_init(&sc->sc_callout, 0);
	callout_setfunc(&sc->sc_callout, acpi_smbus_tick, self);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error(": couldn't establish power handler\n");

	if (sc->sc_poll_alert != 0) {
		aprint_debug(" alert_poll %d sec", sc->sc_poll_alert);
		callout_schedule(&sc->sc_callout, sc->sc_poll_alert * hz);
	}
	aprint_normal("\n");

	/*
	 * Retrieve and display native controller info
	 */
	rv = AcpiGetParent(sc->sc_devnode->ad_handle, &native_dev);
	if (!ACPI_FAILURE(rv))
		rv = AcpiGetParent(native_dev, &native_bus);
	if (!ACPI_FAILURE(rv))
		rv = AcpiGetObjectInfo(native_bus, &native_bus_info);
	if (!ACPI_FAILURE(rv) &&
	    acpi_match_hid(native_bus_info, pcibus_acpi_ids) != 0) {
		rv = AcpiGetObjectInfo(native_dev, &native_dev_info);
		if (!ACPI_FAILURE(rv)) {
			pci_bus = native_bus_info->Address;
			pci_dev = (native_dev_info->Address >> 16) & 0xffff;
			pci_func = native_dev_info->Address & 0xffff;
			aprint_debug_dev(self, "Native i2c host controller"
			    " is on PCI bus %d dev %d func %d\n",
			    pci_bus, pci_dev, pci_func);
			/*
			 * XXX We really need a mechanism to prevent the
			 * XXX native controller from attaching
			 */
		}
	}

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c_tag;
	config_found_ia(self, "i2cbus", &iba, iicbus_print);
}

static int
acpi_smbus_detach(device_t self, int flags) 
{
	struct acpi_smbus_softc *sc = device_private(self);
	ACPI_STATUS rv;
 
	pmf_device_deregister(self);

	callout_halt(&sc->sc_callout, NULL);

	rv = AcpiRemoveNotifyHandler(sc->sc_devnode->ad_handle,
	    ACPI_DEVICE_NOTIFY, acpi_smbus_notify_handler);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self,
		    "unable to deregister DEVICE NOTIFY handler: %s\n",
		    AcpiFormatException(rv));
		return EBUSY;
	}

	mutex_destroy(&sc->sc_i2c_mutex);
   
	return 0;
}

static int
acpi_smbus_acquire_bus(void *cookie, int flags)
{
        struct acpi_smbus_softc *sc = cookie;
  
        mutex_enter(&sc->sc_i2c_mutex);
        return 0;
}       
                
static void
acpi_smbus_release_bus(void *cookie, int flags)
{       
        struct acpi_smbus_softc *sc = cookie;
                                
        mutex_exit(&sc->sc_i2c_mutex);
}
static int
acpi_smbus_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
	const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
        struct acpi_smbus_softc *sc = cookie;
	const uint8_t *c = cmdbuf;
	uint8_t *b = buf, *xb;
	int xlen;
	int r = 0;
	ACPI_BUFFER smbuf;
	ACPI_STATUS rv;
	ACPI_OBJECT_LIST args;
	ACPI_OBJECT arg[5];
	ACPI_OBJECT *p, *e;

	smbuf.Pointer = NULL;
	smbuf.Length = ACPI_ALLOCATE_LOCAL_BUFFER;
	args.Pointer = arg;
	arg[0].Type = ACPI_TYPE_INTEGER;	/* Protocol */
	arg[1].Type = ACPI_TYPE_INTEGER;	/* Slave Addr */
	arg[1].Integer.Value = addr;
	arg[2].Type = ACPI_TYPE_INTEGER;	/* Command */
	if (I2C_OP_READ_P(op)) {
		args.Count = 3;
		if (len == 0) {
			arg[2].Integer.Value = 0;
			if (cmdlen == 0)
				arg[0].Integer.Value = ACPI_SMBUS_RD_QUICK;
			else
				arg[0].Integer.Value = ACPI_SMBUS_RCV_BYTE;
		} else
			arg[2].Integer.Value = *c;
		if (len == 1)
			arg[0].Integer.Value = ACPI_SMBUS_RD_BYTE;
		else if (len == 2)
			arg[0].Integer.Value = ACPI_SMBUS_RD_WORD;
		else if (len > 2)
			arg[0].Integer.Value = ACPI_SMBUS_RD_BLOCK;
		rv = AcpiEvaluateObject(sc->sc_devnode->ad_handle, "_SBR",
				    &args, &smbuf);
	} else {
		args.Count = 5;
		arg[3].Type = ACPI_TYPE_INTEGER;	/* Data Len */
		arg[3].Integer.Value = len;
		arg[4].Type = ACPI_TYPE_INTEGER;	/* Data */
		if (len == 0) {
			arg[4].Integer.Value = 0;
			if (cmdlen == 0) {
				arg[2].Integer.Value = 0;
				arg[0].Integer.Value = ACPI_SMBUS_WR_QUICK;
			} else {
				arg[2].Integer.Value = *c;
				arg[0].Integer.Value = ACPI_SMBUS_SND_BYTE;
			}
		} else
			arg[2].Integer.Value = *c;
		if (len == 1) {
			arg[0].Integer.Value = ACPI_SMBUS_WR_BYTE;
			arg[4].Integer.Value = *b;
		} else if (len == 2) {
			arg[0].Integer.Value = ACPI_SMBUS_WR_WORD;
			arg[4].Integer.Value = *b++;
			arg[4].Integer.Value += (*b--) << 8;
		} else if (len > 2) {
			arg[0].Integer.Value = ACPI_SMBUS_WR_BLOCK;
			arg[4].Type = ACPI_TYPE_BUFFER;
			arg[4].Buffer.Pointer = buf;
			arg[4].Buffer.Length = (len < 32?len:32);
		}
		rv = AcpiEvaluateObject(sc->sc_devnode->ad_handle, "_SBW",
				    &args, &smbuf);
	}
	if (ACPI_FAILURE(rv))
		r = 1;
	else {
		p = smbuf.Pointer;
		if (p == NULL || p->Type != ACPI_TYPE_PACKAGE ||
		    p->Package.Count < 1)
			r = 1;
		else {
			e = p->Package.Elements;
			if (e->Type == ACPI_TYPE_INTEGER)
				r = e[0].Integer.Value;
			else
				r = 1;
		}
		if (r != 0)
			r = 1;

		/* For read operations, copy data to user buffer */
		if (r == 0 && I2C_OP_READ_P(op)) {
			if (p->Package.Count >= 3 &&
			    e[1].Type == ACPI_TYPE_INTEGER) {
				xlen = e[1].Integer.Value;
				if (xlen > len)
					xlen = len;
				if (xlen != 0 &&
				    e[2].Type == ACPI_TYPE_BUFFER) {
					xb = e[2].Buffer.Pointer;
					if (xb != NULL)
						memcpy(b, xb, xlen);
					else
						r = 1;
				} else if (e[2].Type == ACPI_TYPE_INTEGER) {
					if (xlen > 0)
						*b++ = e[2].Integer.Value &
							0xff;
					if (xlen > 1)
						*b = (e[2].Integer.Value >> 8);
				} else
					r = 1;
			} else
				r = 1;
		}
	}
	if (smbuf.Pointer)
		ACPI_FREE(smbuf.Pointer);

	return r;
}

/*
 * acpi_smbus_alerts
 *
 * Whether triggered by periodic polling or an AcpiNotify, retrieve
 * all pending SMBus device alerts
 */
static void
acpi_smbus_alerts(struct acpi_smbus_softc *sc)
{
	int status = 0;
	uint8_t slave_addr;
	ACPI_STATUS rv;
	ACPI_BUFFER alert;
	ACPI_OBJECT *e, *p;

	do {
		rv = acpi_eval_struct(sc->sc_devnode->ad_handle, "_SBA",
				      &alert);
		if (ACPI_FAILURE(rv)) {
			status = 1;
			goto done;
		}

		p = alert.Pointer;
		if (p != NULL && p->Type == ACPI_TYPE_PACKAGE &&
		    p->Package.Count >= 2) {
			e = p->Package.Elements;
			if (e[0].Type == ACPI_TYPE_INTEGER)
				status = e[0].Integer.Value;
			else
				status = 1;
			if (status == 0x0 && e[1].Type == ACPI_TYPE_INTEGER) {
				slave_addr = e[1].Integer.Value;
				aprint_debug_dev(sc->sc_dv, "Alert for 0x%x\n",
						 slave_addr);
				(void)iic_smbus_intr(&sc->sc_i2c_tag);
			}
		}
done:
		if (alert.Pointer != NULL)
			ACPI_FREE(alert.Pointer);
	} while (status == 0);
}

static void
acpi_smbus_tick(void *opaque)
{
	device_t dv = opaque;
	struct acpi_smbus_softc *sc = device_private(dv);

	acpi_smbus_alerts(sc);

	callout_schedule(&sc->sc_callout, sc->sc_poll_alert * hz);
}

static void
acpi_smbus_notify_handler(ACPI_HANDLE hdl, UINT32 notify, void *opaque)
{
	device_t dv = opaque;
	struct acpi_smbus_softc *sc = device_private(dv);

	aprint_debug_dev(dv, "received notify message 0x%x\n", notify);
	acpi_smbus_alerts(sc);
}
