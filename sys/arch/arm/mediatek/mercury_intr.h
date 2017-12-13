/*-
 * Copyright (c) 2017 Mediatek Inc.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_MEDIATEK_INTR_H_
#define _ARM_MEDIATEK_INTR_H_

#define	PIC_MAXSOURCES			288
#define	PIC_MAXMAXSOURCES		300

#include <arm/cortex/gic_intr.h>
#include <arm/cortex/gtmr_intr.h>

#define MTK_SPI_COUNT		32

#define MERCURY_IRQ_USB0	(MTK_SPI_COUNT + 72)
#define MERCURY_IRQ_MMC0	(MTK_SPI_COUNT + 78)
#define MERCURY_IRQ_MMC1	(MTK_SPI_COUNT + 79)
#define MERCURY_IRQ_I2C0	(MTK_SPI_COUNT + 80)
#define MERCURY_IRQ_I2C1	(MTK_SPI_COUNT + 81)
#define MERCURY_IRQ_I2C2	(MTK_SPI_COUNT + 82)
#define MERCURY_IRQ_UART0	(MTK_SPI_COUNT + 84)
#define MERCURY_IRQ_UART1	(MTK_SPI_COUNT + 85)
#define MERCURY_IRQ_UART2	(MTK_SPI_COUNT + 211)
#define MERCURY_IRQ_SPI0	(MTK_SPI_COUNT + 104)
#define MERCURY_IRQ_TIMER	(MTK_SPI_COUNT + 132)
#define MERCURY_IRQ_PWRAP	(MTK_SPI_COUNT + 204)
#define MERCURY_IRQ_USB1	(MTK_SPI_COUNT + 210)

#endif /* _ARM_MEDIATEK_INTR_H_ */
