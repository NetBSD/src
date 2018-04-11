/*	$NetBSD: efinet.c,v 1.1.2.2 2018/04/11 14:51:43 martin Exp $	*/

/*-
 * Copyright (c) 2001 Doug Rabson
 * Copyright (c) 2002, 2006 Marcel Moolenaar
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: head/stand/efi/libefi/efinet.c 321621 2017-07-27 15:06:34Z andrew $");
#endif

#include "efiboot.h"

#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>

#include <bootinfo.h>
#include "devopen.h"

#ifndef ETHER_ALIGN
#define ETHER_ALIGN	2
#endif

#define ETHER_EXT_LEN	(ETHER_HDR_LEN + ETHER_CRC_LEN + ETHER_ALIGN)

#if defined(DEBUG) || defined(ARP_DEBUG) || defined(BOOTP_DEBUG) || \
    defined(NET_DEBUG) || defined(NETIF_DEBUG) || defined(NFS_DEBUG) || \
    defined(RARP_DEBUG) || defined(RPC_DEBUG)
int debug = 1;
#else
int debug = 0;
#endif

extern bool kernel_loaded;

struct efinetinfo {
	EFI_SIMPLE_NETWORK *net;
	bool bootdev;
	size_t pktbufsz;
	UINT8 *pktbuf;
	struct {
		int type;
		u_int tag;
	} bus;
};
static struct btinfo_netif bi_netif;

static int	efinet_match(struct netif *, void *);
static int	efinet_probe(struct netif *, void *);
static void	efinet_init(struct iodesc *, void *);
static int	efinet_get(struct iodesc *, void *, size_t, saseconds_t);
static int	efinet_put(struct iodesc *, void *, size_t);
static void	efinet_end(struct netif *);

struct netif_driver efinetif = {
	.netif_bname = "net",
	.netif_match = efinet_match,
	.netif_probe = efinet_probe,
	.netif_init = efinet_init,
	.netif_get = efinet_get,
	.netif_put = efinet_put,
	.netif_end = efinet_end,
	.netif_ifs = NULL,
	.netif_nifs = 0
};

#ifdef EFINET_DEBUG
static void
dump_mode(EFI_SIMPLE_NETWORK_MODE *mode)
{
	int i;

	printf("State                 = %x\n", mode->State);
	printf("HwAddressSize         = %u\n", mode->HwAddressSize);
	printf("MediaHeaderSize       = %u\n", mode->MediaHeaderSize);
	printf("MaxPacketSize         = %u\n", mode->MaxPacketSize);
	printf("NvRamSize             = %u\n", mode->NvRamSize);
	printf("NvRamAccessSize       = %u\n", mode->NvRamAccessSize);
	printf("ReceiveFilterMask     = %x\n", mode->ReceiveFilterMask);
	printf("ReceiveFilterSetting  = %u\n", mode->ReceiveFilterSetting);
	printf("MaxMCastFilterCount   = %u\n", mode->MaxMCastFilterCount);
	printf("MCastFilterCount      = %u\n", mode->MCastFilterCount);
	printf("MCastFilter           = {");
	for (i = 0; i < mode->MCastFilterCount; i++)
		printf(" %s", ether_sprintf(mode->MCastFilter[i].Addr));
	printf(" }\n");
	printf("CurrentAddress        = %s\n",
	    ether_sprintf(mode->CurrentAddress.Addr));
	printf("BroadcastAddress      = %s\n",
	    ether_sprintf(mode->BroadcastAddress.Addr));
	printf("PermanentAddress      = %s\n",
	    ether_sprintf(mode->PermanentAddress.Addr));
	printf("IfType                = %u\n", mode->IfType);
	printf("MacAddressChangeable  = %d\n", mode->MacAddressChangeable);
	printf("MultipleTxSupported   = %d\n", mode->MultipleTxSupported);
	printf("MediaPresentSupported = %d\n", mode->MediaPresentSupported);
	printf("MediaPresent          = %d\n", mode->MediaPresent);
}
#endif

static int
efinet_match(struct netif *nif, void *machdep_hint)
{
	struct devdesc *dev = machdep_hint;

	if (dev->d_unit != nif->nif_unit)
		return 0;

	return 1;
}

static int
efinet_probe(struct netif *nif, void *machdep_hint)
{

	return 0;
}

static int
efinet_put(struct iodesc *desc, void *pkt, size_t len)
{
	struct netif *nif = desc->io_netif;
	struct efinetinfo *eni = nif->nif_devdata;
	EFI_SIMPLE_NETWORK *net;
	EFI_STATUS status;
	void *buf;

	if (eni == NULL)
		return -1;
	net = eni->net;

	status = uefi_call_wrapper(net->Transmit, 7, net, 0, (UINTN)len, pkt, NULL,
	    NULL, NULL);
	if (EFI_ERROR(status))
		return -1;

	/* Wait for the buffer to be transmitted */
	do {
		buf = NULL;	/* XXX Is this needed? */
		status = uefi_call_wrapper(net->GetStatus, 3, net, NULL, &buf);
		/*
		 * XXX EFI1.1 and the E1000 card returns a different
		 * address than we gave.  Sigh.
		 */
	} while (!EFI_ERROR(status) && buf == NULL);

	/* XXX How do we deal with status != EFI_SUCCESS now? */
	return EFI_ERROR(status) ? -1 : len;
}

static int
efinet_get(struct iodesc *desc, void *pkt, size_t len, saseconds_t timeout)
{
	struct netif *nif = desc->io_netif;
	struct efinetinfo *eni = nif->nif_devdata;
	EFI_SIMPLE_NETWORK *net;
	EFI_STATUS status;
	UINTN bufsz, rsz;
	time_t t;
	char *buf, *ptr;
	int ret = -1;

	if (eni == NULL)
		return -1;
	net = eni->net;

	if (eni->pktbufsz < net->Mode->MaxPacketSize + ETHER_EXT_LEN) {
		bufsz = net->Mode->MaxPacketSize + ETHER_EXT_LEN;
		buf = alloc(bufsz);
		if (buf == NULL)
			return -1;
		dealloc(eni->pktbuf, eni->pktbufsz);
		eni->pktbufsz = bufsz;
		eni->pktbuf = buf;
	}
	ptr = eni->pktbuf + ETHER_ALIGN;

	t = getsecs();
	while ((getsecs() - t) < timeout) {
		rsz = eni->pktbufsz;
		status = uefi_call_wrapper(net->Receive, 7, net, NULL, &rsz, ptr,
		    NULL, NULL, NULL);
		if (!EFI_ERROR(status)) {
			rsz = min(rsz, len);
			memcpy(pkt, ptr, rsz);
			ret = (int)rsz;
			break;
		}
		if (status != EFI_NOT_READY)
			break;
	}

	return ret;
}

static void
efinet_init(struct iodesc *desc, void *machdep_hint)
{
	struct netif *nif = desc->io_netif;
	struct efinetinfo *eni;
	EFI_SIMPLE_NETWORK *net;
	EFI_STATUS status;
	UINT32 mask;

	if (nif->nif_driver->netif_ifs[nif->nif_unit].dif_unit < 0) {
		printf("Invalid network interface %d\n", nif->nif_unit);
		return;
	}

	eni = nif->nif_driver->netif_ifs[nif->nif_unit].dif_private;
	nif->nif_devdata = eni;
	net = eni->net;
	if (net->Mode->State == EfiSimpleNetworkStopped) {
		status = uefi_call_wrapper(net->Start, 1, net);
		if (EFI_ERROR(status)) {
			printf("net%d: cannot start interface (status=%"
			    PRIxMAX ")\n", nif->nif_unit, (uintmax_t)status);
			return;
		}
	}

	if (net->Mode->State != EfiSimpleNetworkInitialized) {
		status = uefi_call_wrapper(net->Initialize, 3, net, 0, 0);
		if (EFI_ERROR(status)) {
			printf("net%d: cannot init. interface (status=%"
			    PRIxMAX ")\n", nif->nif_unit, (uintmax_t)status);
			return;
		}
	}

	mask = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
	    EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;

	status = uefi_call_wrapper(net->ReceiveFilters, 6, net, mask, 0, FALSE,
	    0, NULL);
	if (EFI_ERROR(status)) {
		printf("net%d: cannot set rx. filters (status=%" PRIxMAX ")\n",
		    nif->nif_unit, (uintmax_t)status);
		return;
	}

	if (!kernel_loaded) {
		bi_netif.bus = eni->bus.type;
		bi_netif.addr.tag = eni->bus.tag;
		snprintf(bi_netif.ifname, sizeof(bi_netif.ifname), "net%d",
		    nif->nif_unit);
		BI_ADD(&bi_netif, BTINFO_NETIF, sizeof(bi_netif));
	}

#ifdef EFINET_DEBUG
	dump_mode(net->Mode);
#endif

	memcpy(desc->myea, net->Mode->CurrentAddress.Addr, 6);
	desc->xid = 1;
}

static void
efinet_end(struct netif *nif)
{
	struct efinetinfo *eni = nif->nif_devdata;
	EFI_SIMPLE_NETWORK *net;

	if (eni == NULL)
		return;
	net = eni->net;

	uefi_call_wrapper(net->Shutdown, 1, net);
}

static bool
efi_net_pci_probe(struct efinetinfo *eni, EFI_DEVICE_PATH *dp, EFI_DEVICE_PATH *pdp)
{
	PCI_DEVICE_PATH *pci = (PCI_DEVICE_PATH *)dp;
	int bus = -1;

	if (pdp != NULL &&
	    DevicePathType(pdp) == ACPI_DEVICE_PATH &&
	    (DevicePathSubType(pdp) == ACPI_DP ||
	     DevicePathSubType(pdp) == EXPANDED_ACPI_DP)) {
		ACPI_HID_DEVICE_PATH *acpi = (ACPI_HID_DEVICE_PATH *)pdp;
		/* PCI root bus */
		if (acpi->HID == EISA_PNP_ID(0x0A08) ||
		    acpi->HID == EISA_PNP_ID(0x0A03)) {
			bus = acpi->UID;
		}
	}
	if (bus < 0)
		return false;

	eni->bus.type = BI_BUS_PCI;
	eni->bus.tag = (bus & 0xff) << 8;
	eni->bus.tag |= (pci->Device & 0x1f) << 3;
	eni->bus.tag |= pci->Function & 0x7;
	return true;
}

void
efi_net_probe(void)
{
	struct efinetinfo *enis;
	struct netif_dif *dif;
	struct netif_stats *stats;
	EFI_DEVICE_PATH *dp0, *dp, *pdp;
	EFI_SIMPLE_NETWORK *net;
	EFI_HANDLE *handles;
	EFI_STATUS status;
	UINTN i, nhandles;
	int nifs;
	bool found;

	status = LibLocateHandle(ByProtocol, &SimpleNetworkProtocol, NULL,
	    &nhandles, &handles);
	if (EFI_ERROR(status) || nhandles == 0)
		return;

	enis = alloc(nhandles * sizeof(*enis));
	if (enis == NULL)
		return;
	memset(enis, 0, nhandles * sizeof(*enis));

	nifs = 0;
	for (i = 0; i < nhandles; i++) {
		status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i],
		    &DevicePathProtocol, (void **)&dp0);
		if (EFI_ERROR(status))
			continue;

		found = false;
		for (dp = dp0; !IsDevicePathEnd(dp); dp = NextDevicePathNode(dp)) {
			if (DevicePathType(dp) == MESSAGING_DEVICE_PATH &&
			    DevicePathSubType(dp) == MSG_MAC_ADDR_DP) {
				found = true;
				break;
			}
		}
		if (!found)
			continue;

		status = uefi_call_wrapper(BS->OpenProtocol, 6, handles[i],
		    &SimpleNetworkProtocol, (void **)&net, IH, NULL,
		    EFI_OPEN_PROTOCOL_EXCLUSIVE);
		if (EFI_ERROR(status)) {
			printf("Unable to open network interface %" PRIuMAX
			    " for exclusive access: %" PRIxMAX "\n",
			    (uintmax_t)i, (uintmax_t)status);
		}

		found = false;
		for (pdp = NULL, dp = dp0;
		    !IsDevicePathEnd(dp);
		    pdp = dp, dp = NextDevicePathNode(dp)) {
			if (DevicePathType(dp) == HARDWARE_DEVICE_PATH) {
				if (DevicePathSubType(dp) == HW_PCI_DP)
					found = efi_net_pci_probe(&enis[nifs],
					    dp, pdp);
				break;
			}
		}
		if (found) {
			enis[nifs].net = net;
			enis[nifs].bootdev = efi_pxe_match_booted_interface(
			    &net->Mode->CurrentAddress, net->Mode->HwAddressSize);
			enis[nifs].pktbufsz = net->Mode->MaxPacketSize +
			    ETHER_EXT_LEN;
			enis[nifs].pktbuf = alloc(enis[nifs].pktbufsz);
			if (enis[nifs].pktbuf == NULL) {
				while (i-- > 0) {
					dealloc(enis[i].pktbuf, enis[i].pktbufsz);
					if (i == 0)
						break;
				}
				dealloc(enis, nhandles * sizeof(*enis));
				FreePool(handles);
				return;
			}
			nifs++;
		}
	}

	FreePool(handles);

	if (nifs == 0)
		return;

	efinetif.netif_ifs = alloc(nifs * sizeof(*dif));
	stats = alloc(nifs * sizeof(*stats));
	if (efinetif.netif_ifs == NULL || stats == NULL) {
		if (efinetif.netif_ifs != NULL) {
			dealloc(efinetif.netif_ifs, nifs * sizeof(*dif));
			efinetif.netif_ifs = NULL;
		}
		if (stats != NULL)
			dealloc(stats, nifs * sizeof(*stats));
		for (i = 0; i < nifs; i++)
			dealloc(enis[i].pktbuf, enis[i].pktbufsz);
		dealloc(enis, nhandles * sizeof(*enis));
		return;
	}
	memset(efinetif.netif_ifs, 0, nifs * sizeof(*dif));
	memset(stats, 0, nifs * sizeof(*stats));
	efinetif.netif_nifs = nifs;

	for (i = 0; i < nifs; i++) {
		dif = &efinetif.netif_ifs[i];
		dif->dif_unit = i;
		dif->dif_nsel = 1;
		dif->dif_stats = &stats[i];
		dif->dif_private = &enis[i];
	}
}

void
efi_net_show(void)
{
	const struct netif_dif *dif;
	const struct efinetinfo *eni;
	EFI_SIMPLE_NETWORK *net;
	int i;

	for (i = 0; i < efinetif.netif_nifs; i++) {
		dif = &efinetif.netif_ifs[i];
		eni = dif->dif_private;
		net = eni->net;

		printf("net net%d", dif->dif_unit);
		if (net->Mode != NULL) {
			for (UINT32 x = 0; x < net->Mode->HwAddressSize; x++) {
				printf("%c%02x", x == 0 ? ' ' : ':',
				    net->Mode->CurrentAddress.Addr[x]);
			}
		}
		if (eni->bus.type == BI_BUS_PCI) {
			printf(" pci%d,%d,%d", (eni->bus.tag >> 8) & 0xff,
			    (eni->bus.tag >> 3) & 0x1f, eni->bus.tag & 0x7);
		}
		if (eni->bootdev)
			printf(" pxeboot");
		printf("\n");
	}
}

int
efi_net_get_booted_interface_unit(void)
{
	const struct netif_dif *dif;
	const struct efinetinfo *eni;
	int i;

	for (i = 0; i < efinetif.netif_nifs; i++) {
		dif = &efinetif.netif_ifs[i];
		eni = dif->dif_private;
		if (eni->bootdev)
			return dif->dif_unit;
	}
	return -1;
}
