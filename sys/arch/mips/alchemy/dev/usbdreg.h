/* $NetBSD: usbdreg.h,v 1.1.10.2 2006/04/22 11:37:42 simonb Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#ifndef	_MIPS_ALCHEMY_DEV_USBDREG_H
#define	_MIPS_ALCHEMY_DEV_USBDREG_H

#define USBD_EP0RD		0x00		/* Read from endpoint 0 */
#define USBD_EP0WR		0x04		/* Write to endpoint 0 */
#define USBD_EP1WR		0x08		/* Write to endpoint 1 */
#define USBD_EP2WR		0x0c		/* Write to endpoint 2 */
#define USBD_EP3RD		0x10		/* Read from endpoint 3 */
#define USBD_EP4RD		0x14		/* Read from endpoint 4 */
#define USBD_INTEN		0x18		/* Interrupt Enable Register */
#define USBD_INTSTAT		0x1c		/* Interrupt Status Register */
#define USBD_CONFIG		0x20		/* Write Configuration Register */
#define USBD_EP0CS		0x24		/* Endpoint 0 control and status */
#define USBD_EP1CS		0x28		/* Endpoint 1 control and status */
#define USBD_EP2CS		0x2c		/* Endpoint 2 control and status */
#define USBD_EP3CS		0x30		/* Endpoint 3 control and status */
#define USBD_EP4CS		0x34		/* Endpoint 4 control and status */
#define USBD_FRAMENUM		0x38		/* Current frame number */
#define USBD_EP0RDSTAT		0x40		/* EP0 Read FIFO Status */
#define USBD_EP0WRSTAT		0x44		/* EP0 Write FIFO Status */
#define USBD_EP1WRSTAT		0x48		/* EP1 Write FIFO Status */
#define USBD_EP2WRSTAT		0x4c		/* EP2 Write FIFO Status */
#define USBD_EP3RDSTAT		0x50		/* EP3 Read FIFO Status */
#define USBD_EP4RDSTAT		0x54		/* EP4 Read FIFO Status */
#define USBD_ENABLE		0x58		/* USB Device Controller Enable */

#endif	/* _MIPS_ALCHEMY_DEV_USBDREG_H */
