/* $NetBSD: ahcireg.h,v 1.1.2.2 2007/03/24 14:54:52 yamt Exp $ */

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
#ifndef	_AHCIREG_H
#define	_AHCIREG_H

#define ADMHCD_REG_CONTROL              0x00
#define ADMHCD_REG_INTSTATUS            0x04
#define ADMHCD_REG_INTENABLE            0x08
#define ADMHCD_REG_HOSTCONTROL          0x10
#define ADMHCD_REG_FMINTERVAL           0x18
#define ADMHCD_REG_FMNUMBER             0x1c
#define ADMHCD_REG_LSTHRESH             0x70
#define ADMHCD_REG_RHDESCR              0x74
#define ADMHCD_REG_PORTSTATUS0          0x78
#define ADMHCD_REG_PORTSTATUS1          0x7c
#define ADMHCD_REG_HOSTHEAD             0x80
#define ADMHCD_NUMPORTS         2

#define ADMHCD_HOST_EN          0x00000001      /* Host enable */
#define ADMHCD_SW_INTREQ        0x00000002      /* request software int */
#define ADMHCD_SW_RESET         0x00000008      /* Reset */

#define ADMHCD_INT_TD           0x00100000      /* TD completed */
#define ADMHCD_INT_SW           0x20000000      /* software interrupt */
#define ADMHCD_INT_FATAL        0x40000000      /* Fatal interrupt */
#define ADMHCD_INT_ACT          0x80000000      /* Interrupt active */

#define ADMHCD_STATE_RST        0x00000000      /* bus state reset */
#define ADMHCD_STATE_RES        0x00000001      /* bus state resume */
#define ADMHCD_STATE_OP         0x00000002      /* bus state operational */
#define ADMHCD_STATE_SUS        0x00000003      /* bus state suspended */
#define ADMHCD_DMA_EN           0x00000004      /* enable dma engine */

#define ADMHCD_NPS              0x00000020      /* No Power Switch */
#define ADMHCD_LPSC             0x04000000      /* Local power switch change */

#define ADMHCD_CCS              0x00000001      /* current connect status */
#define ADMHCD_PES              0x00000002      /* port enable status */
#define ADMHCD_PSS              0x00000004      /* port suspend status */
#define ADMHCD_POCI             0x00000008      /* port overcurrent indicator */
#define ADMHCD_PRS              0x00000010      /* port reset status */
#define ADMHCD_PPS              0x00000100      /* port power status */
#define ADMHCD_LSDA             0x00000200      /* low speed device attached */
#define ADMHCD_CSC              0x00010000      /* connect status change */
#define ADMHCD_PESC             0x00020000      /* enable status change */
#define ADMHCD_PSSC             0x00040000      /* suspend status change */
#define ADMHCD_OCIC             0x00080000      /* overcurrent change*/
#define ADMHCD_PRSC             0x00100000      /* reset status change */

struct admhcd_ed {
        /* Don't change first four, they used for DMA */
        volatile u_int32_t                       control;
        volatile struct admhcd_td               *tail;
        volatile struct admhcd_td               *head;
        volatile struct admhcd_ed               *next;
        /* the rest is for the driver only: */
        u_int32_t                       unused[4];
} __attribute__ ((packed));

#define ADMHCD_ED_EPSHIFT       7               /* Shift for endpoint number */
#define ADMHCD_ED_INT           0x00000800      /* Is this an int endpoint */
#define ADMHCD_ED_SPEED         0x00002000      /* Is it a high speed dev? */
#define ADMHCD_ED_SKIP          0x00004000      /* Skip this ED */
#define ADMHCD_ED_FORMAT        0x00008000      /* Is this an isoc endpoint */
#define ADMHCD_ED_MAXSHIFT      16              /* Shift for max packet size */

struct admhcd_td {
        /* Don't change first four, they are used for DMA */
        volatile u_int32_t               control;
        volatile u_int32_t               buffer;
        volatile u_int32_t               buflen;
        volatile struct admhcd_td       *next;
        /* the rest is for the driver only: */
        /* struct urb           *urb;
        struct admhcd_td        *real; */
        u_int32_t                       len;
        u_int32_t                       unused[3];
} __attribute__ ((packed));

#define ADMHCD_TD_OWN           0x80000000
#define ADMHCD_TD_TOGGLE        0x00000000
#define ADMHCD_TD_DATA0         0x01000000
#define ADMHCD_TD_DATA1         0x01800000
#define ADMHCD_TD_OUT           0x00200000
#define ADMHCD_TD_IN            0x00400000
#define ADMHCD_TD_SETUP         0x00000000
#define ADMHCD_TD_ISO           0x00010000
#define ADMHCD_TD_R             0x00040000
#define ADMHCD_TD_INTEN         0x00010000

#define ADMHCD_TD_ERRMASK       0x78000000
#define ADMHCD_TD_ERRSHIFT      27

#endif	/* _AHCIREG_H */
