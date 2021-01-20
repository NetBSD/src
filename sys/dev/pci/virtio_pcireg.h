/*	$NetBSD: virtio_pcireg.h,v 1.1 2021/01/20 19:46:48 reinoud Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * Copyright (c) 2010 Minoura Makoto.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Part of the file derived from `Virtio PCI Card Specification v0.8.6 DRAFT'
 * Appendix A.
 */
/* An interface for efficient virtio implementation.
 *
 * This header is BSD licensed so anyone can use the definitions
 * to implement compatible drivers/servers.
 *
 * Copyright 2007, 2009, IBM Corporation
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
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef _DEV_PCI_VIRTIO_PCIREG_H_
#define	_DEV_PCI_VIRTIO_PCIREG_H_

#include <sys/types.h>

/*
 * Virtio PCI v0.9 config space
 */
#define VIRTIO_CONFIG_DEVICE_FEATURES		 0 /* 32 bit */
#define VIRTIO_CONFIG_GUEST_FEATURES		 4 /* 32 bit */
#define VIRTIO_CONFIG_QUEUE_ADDRESS		 8 /* 32 bit */
#define VIRTIO_CONFIG_QUEUE_SIZE		12 /* 16 bit */
#define VIRTIO_CONFIG_QUEUE_SELECT		14 /* 16 bit */
#define VIRTIO_CONFIG_QUEUE_NOTIFY		16 /* 16 bit */
#define VIRTIO_CONFIG_DEVICE_STATUS		18 /*  8 bit */
#define VIRTIO_CONFIG_ISR_STATUS		19 /*  8 bit */
#define VIRTIO_CONFIG_CONFIG_VECTOR		20 /* 16 bit, optional */
#define VIRTIO_CONFIG_DEVICE_CONFIG_NOMSI	20 /* start of config space */
#define VIRTIO_CONFIG_DEVICE_CONFIG_MSI		24 /* start of config space */
/* MSI/MSI-X */
#define VIRTIO_CONFIG_MSI_CONFIG_VECTOR		20 /* 16 bit, optional */
#define VIRTIO_CONFIG_MSI_QUEUE_VECTOR		22 /* 16 bit, optional */


/*
 * Virtio PCI 1.0 PCI cap space
 */

#define VIRTIO_PCI_CAP_COMMON_CFG	1 /* Common configuration */
#define VIRTIO_PCI_CAP_NOTIFY_CFG	2 /* Notifications */
#define VIRTIO_PCI_CAP_ISR_CFG		3 /* ISR Status */
#define VIRTIO_PCI_CAP_DEVICE_CFG	4 /* Device specific configuration */
#define VIRTIO_PCI_CAP_PCI_CFG		5 /* PCI configuration access */

struct virtio_pci_cap {
	uint8_t cap_vndr;	/* Generic PCI field: PCI_CAP_ID_VNDR */
	uint8_t cap_next;	/* Generic PCI field: next ptr */
	uint8_t cap_len;	/* Generic PCI field: capability length */
	uint8_t cfg_type;	/* Identifies the structure */
	uint8_t bar;		/* Where to find it */
	uint8_t padding[3];	/* Pad to full dword */
	uint32_t offset;	/* Offset within bar */
	uint32_t length;	/* Length of the structure, in bytes */
} __packed;


struct virtio_pci_notify_cap {
	struct virtio_pci_cap cap;
	uint32_t notify_off_multiplier;	/* Multiplier for queue_notify_off. */
} __packed;

/*
 * Virtio PCI v1.0 config space
 */

#define VIRTIO_CONFIG1_DEVICE_FEATURE_SELECT	 0 /* 32 bit RW */
#define VIRTIO_CONFIG1_DEVICE_FEATURE		 4 /* 32 bit RO */
#define VIRTIO_CONFIG1_DRIVER_FEATURE_SELECT	 8 /* 32 bit RW */
#define VIRTIO_CONFIG1_DRIVER_FEATURE		12 /* 32 bit RW */
#define VIRTIO_CONFIG1_CONFIG_MSIX_VECTOR	16 /* 16 bit RW */
#define VIRTIO_CONFIG1_NUM_QUEUES		18 /* 16 bit RO */
#define VIRTIO_CONFIG1_DEVICE_STATUS		20 /*  8 bit RW */
#define VIRTIO_CONFIG1_CONFIG_GENERATION	21 /*  8 bit RO */

/* about a specific virtqueue: */
#define VIRTIO_CONFIG1_QUEUE_SELECT		22 /* 16 bit RW */
#define VIRTIO_CONFIG1_QUEUE_SIZE		24 /* 16 bit RO, power of 2 */
#define VIRTIO_CONFIG1_QUEUE_MSIX_VECTOR	26 /* 16 bit RW */
#define VIRTIO_CONFIG1_QUEUE_ENABLE		28 /* 16 bit RW */
#define VIRTIO_CONFIG1_QUEUE_NOTIFY_OFF		30 /* 16 bit RO */
#define VIRTIO_CONFIG1_QUEUE_DESC		32 /* 64 bit RW */
#define VIRTIO_CONFIG1_QUEUE_AVAIL		40 /* 64 bit RW */
#define VIRTIO_CONFIG1_QUEUE_USED		48 /* 64 bit RW */


#endif /* _DEV_PCI_VIRTIO_PCIREG_H_ */
