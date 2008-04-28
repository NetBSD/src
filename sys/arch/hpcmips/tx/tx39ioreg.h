/*	$NetBSD: tx39ioreg.h,v 1.6 2008/04/28 20:23:21 martin Exp $ */

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 * TOSHIBA TX3912/TX3922 IO module
 */

#ifndef __TX39IO_PRIVATE
#error "don't include this file"
#else /* !__TX39IO_PRIVATE */
#define	TX39_IOCTRL_REG			0x180
#define	TX39_IOMFIODATAOUT_REG		0x184
#define	TX39_IOMFIODATADIR_REG		0x188
#define	TX39_IOMFIODATAIN_REG		0x18c
#define	TX39_IOMFIODATASEL_REG		0x190
#define	TX39_IOIOPOWERDWN_REG		0x194
#define	TX39_IOMFIOPOWERDWN_REG		0x198
#ifdef TX392X
#define TX392X_IODATAINOUT_REG		0x19c
#endif /* TX392X */

#define TX39_IO_MFIO_MAX		32
#ifdef TX391X
#define TX391X_IO_IO_MAX		7
#endif /* TX391X */
#ifdef TX392X
#define TX392X_IO_IO_MAX		16
#endif /* TX392X */

/*
 *	IO Control Register
 */
#ifdef TX391X
#define TX391X_IOCTRL_IODEBSEL_SHIFT	24
#define TX391X_IOCTRL_IODEBSEL_MASK	0x7f
#define TX391X_IOCTRL_IODEBSEL(cr)					\
	(((cr) >> TX391X_IOCTRL_IODEBSEL_SHIFT) &			\
	TX391X_IOCTRL_IODEBSEL_MASK)
#define TX391X_IOCTRL_IODEBSEL_SET(cr, val)				\
	((cr) | (((val) << TX391X_IOCTRL_IODEBSEL_SHIFT) &		\
	(TX391X_IOCTRL_IODEBSEL_MASK << TX391X_IOCTRL_IODEBSEL_SHIFT)))

#define TX391X_IOCTRL_IODIREC_SHIFT	16
#define TX391X_IOCTRL_IODIREC_MASK	0x7f
#define TX391X_IOCTRL_IODIREC(cr)					\
	(((cr) >> TX391X_IOCTRL_IODIREC_SHIFT) &			\
	TX391X_IOCTRL_IODIREC_MASK)
#define TX391X_IOCTRL_IODIREC_SET(cr, val)				\
	((cr) | (((val) << TX391X_IOCTRL_IODIREC_SHIFT) &		\
	(TX391X_IOCTRL_IODIREC_MASK << TX391X_IOCTRL_IODIREC_SHIFT)))

#define TX391X_IOCTRL_IODOUT_SHIFT	8
#define TX391X_IOCTRL_IODOUT_MASK	0x7f
#define TX391X_IOCTRL_IODOUT(cr)					\
	(((cr) >> TX391X_IOCTRL_IODOUT_SHIFT) &				\
	TX391X_IOCTRL_IODOUT_MASK)
#define TX391X_IOCTRL_IODOUT_CLR(cr)					\
	((cr) &= ~(TX391X_IOCTRL_IODOUT_MASK << TX391X_IOCTRL_IODOUT_SHIFT))
#define TX391X_IOCTRL_IODOUT_SET(cr, val)				\
	((cr) | (((val) << TX391X_IOCTRL_IODOUT_SHIFT) &		\
	(TX391X_IOCTRL_IODOUT_MASK << TX391X_IOCTRL_IODOUT_SHIFT)))

#define TX391X_IOCTRL_IODIN_SHIFT	0
#define TX391X_IOCTRL_IODIN_MASK	0x7f
#define TX391X_IOCTRL_IODIN(cr)						\
	(((cr) >> TX391X_IOCTRL_IODIN_SHIFT) &				\
	TX391X_IOCTRL_IODIN_MASK)
#endif /* TX391X */

#ifdef TX392X
#define TX392X_IOCTRL_IODEBSEL_SHIFT	16
#define TX392X_IOCTRL_IODEBSEL_MASK	0xffff
#define TX392X_IOCTRL_IODEBSEL(cr)					\
	(((cr) >> TX392X_IOCTRL_IODEBSEL_SHIFT) &			\
	TX392X_IOCTRL_IODEBSEL_MASK)
#define TX392X_IOCTRL_IODEBSEL_SET(cr, val)				\
	((cr) | (((val) << TX392X_IOCTRL_IODEBSEL_SHIFT) &		\
	(TX392X_IOCTRL_IODEBSEL_MASK << TX392X_IOCTRL_IODEBSEL_SHIFT)))

#define TX392X_IOCTRL_IODIREC_SHIFT	0
#define TX392X_IOCTRL_IODIREC_MASK	0xffff
#define TX392X_IOCTRL_IODIREC(cr)					\
	(((cr) >> TX392X_IOCTRL_IODIREC_SHIFT) &			\
	TX392X_IOCTRL_IODIREC_MASK)
#define TX392X_IOCTRL_IODIREC_SET(cr, val)				\
	((cr) | (((val) << TX392X_IOCTRL_IODIREC_SHIFT) &		\
	(TX392X_IOCTRL_IODIREC_MASK << TX392X_IOCTRL_IODIREC_SHIFT)))

#define TX392X_IODATAINOUT_DOUT_SHIFT	16
#define TX392X_IODATAINOUT_DOUT_MASK	0xffff
#define TX392X_IODATAINOUT_DOUT(cr)					\
	(((cr) >> TX392X_IODATAINOUT_DOUT_SHIFT) &			\
	TX392X_IODATAINOUT_DOUT_MASK)
#define TX392X_IODATAINOUT_DOUT_SET(cr, val)				\
	((cr) | (((val) << TX392X_IODATAINOUT_DOUT_SHIFT) &		\
	(TX392X_IODATAINOUT_DOUT_MASK << TX392X_IODATAINOUT_DOUT_SHIFT)))
#define TX392X_IODATAINOUT_DOUT_CLR(cr)				\
	((cr) &= ~(TX392X_IODATAINOUT_DOUT_MASK <<			\
		   TX392X_IODATAINOUT_DOUT_SHIFT))

#define TX392X_IODATAINOUT_DIN_SHIFT	0
#define TX392X_IODATAINOUT_DIN_MASK	0xffff
#define TX392X_IODATAINOUT_DIN(cr)					\
	(((cr) >> TX392X_IODATAINOUT_DIN_SHIFT) &			\
	TX392X_IODATAINOUT_DIN_MASK)
#define TX392X_IODATAINOUT_DIN_SET(cr, val)				\
	((cr) | (((val) << TX392X_IODATAINOUT_DIN_SHIFT) &		\
	(TX392X_IODATAINOUT_DIN_MASK << TX392X_IODATAINOUT_DIN_SHIFT)))
#endif /* TX392X */
/*
 *	MFIO Data Output Register
 */
#define TX39_IOMFIODATAOUT_MFIODOUT	0

/*
 *	MFIO Data Direction Register
 */
#define TX39_IOMFIODATADIR_MFIODIREC	0

/*
 *	MFIO Data Input Register
 */
#define TX39_IOMFIODATAIN_MFIODIN	0

/*
 *	MFIO Data Select Register
 */
#define TX39_IOMFIODATASEL_MFIOSEL		0
#define TX39_IOMFIODATASEL_MFIOSEL_RESET	0xf20f0fff

/*
 *	IO Power Down Register
 */
#ifdef TX391X
#define TX391X_IOIOPOWERDWN_IOPD_SHIFT	0
#define TX391X_IOIOPOWERDWN_IOPD_MASK	0x7f
#define TX391X_IOIOPOWERDWN_IOPD_RESET	0x7f
#define TX391X_IOIOPOWERDWN_IOPD(cr)					\
	(((cr) >> TX391X_IOIOPOWERDWN_IOPD_SHIFT) &			\
	TX391X_IOIOPOWERDWN_IOPD_MASK)
#define TX391X_IOIOPOWERDWN_IOPD_SET(cr, val)				\
	((cr) | (((val) << TX391X_IOIOPOWERDWN_IOPD_SHIFT) &		\
	(TX391X_IOIOPOWERDWN_IOPD_MASK << TX391X_IOIOPOWERDOWN_IOPD_SHIFT)))
#endif /* TX391X */
#ifdef TX392X
#define TX392X_IOIOPOWERDWN_IOPD_SHIFT	0
#define TX392X_IOIOPOWERDWN_IOPD_MASK	0xffff
#define TX392X_IOIOPOWERDWN_IOPD_RESET	0x0fff
#define TX392X_IOIOPOWERDWN_IOPD(cr)					\
	(((cr) >> TX392X_IOIOPOWERDWN_IOPD_SHIFT) &			\
	TX392X_IOIOPOWERDWN_IOPD_MASK)
#define TX392X_IOIOPOWERDWN_IOPD_SET(cr, val)				\
	((cr) | (((val) << TX392X_IOIOPOWERDWN_IOPD_SHIFT) &		\
	(TX392X_IOIOPOWERDWN_IOPD_MASK << TX392X_IOIOPOWERDOWN_IOPD_SHIFT)))
#endif /* TX392X */


/*
 *	MFIO Power Down Register
 */
#define	TX39_IOMFIOPOWERDWN_MFIOPD		0
#define	TX39_IOMFIOPOWERDWN_MFIOPD_RESET	0xfaf03ffc

/*
 *	MFIO mapping
 */
struct tx39io_mfio_map {
	const char *std_pin_name;
	int  std_type;
#define STD_IN		1
#define STD_OUT		2
#define STD_INOUT	3
};

#ifdef TX391X
static const struct tx39io_mfio_map
tx391x_mfio_map[TX39_IO_MFIO_MAX] = {
  [31] = {"CHIFS",	STD_INOUT},
  [30] = {"CHICLK",	STD_INOUT},
  [29] = {"CHIDOUT",	STD_OUT},
  [28] = {"CHIDIN",	STD_IN},
  [27] = {"DREQ",	STD_IN},
  [26] = {"DGRINT",	STD_OUT},
  [25] = {"BC32K",	STD_OUT},
  [24] = {"TXD",	STD_OUT},
  [23] = {"RXD",	STD_IN},
  [22] = {"CS1",	STD_OUT},
  [21] = {"CS2",	STD_OUT},
  [20] = {"CS3",	STD_OUT},
  [19] = {"MCS0",	STD_OUT},
  [18] = {"MCS1",	STD_OUT},
  [17] = {"MCS2",	STD_OUT},
  [16] = {"MCS3",	STD_OUT},
  [15] = {"SPICLK",	STD_OUT},
  [14] = {"SPIOUT",	STD_OUT},
  [13] = {"SPIN",	STD_IN},
  [12] = {"SIBMCLK",	STD_INOUT},
  [11] = {"CARDREG",	STD_OUT},
  [10] = {"CARDIOWR",	STD_OUT},
   [9] = {"CARDIORD",	STD_OUT},
   [8] = {"CARD1CSL",	STD_OUT},
   [7] = {"CARD1CSH",	STD_OUT},
   [6] = {"CARD2CSL",	STD_OUT},
   [5] = {"CARD2CSH",	STD_OUT},
   [4] = {"CARD1WAIT",	STD_IN},
   [3] = {"CARD2WAIT",	STD_IN},
   [2] = {"CARDDIR",	STD_OUT},
   [1] = {"MFIO[1]",	0},
   [0] = {"MFIO[0]",	0}
};
#endif /* TX391X */

#ifdef TX392X
static const struct tx39io_mfio_map
tx392x_mfio_map[TX39_IO_MFIO_MAX] = {
  [31] = {"CHIFS",	STD_INOUT},
  [30] = {"CHICLK",	STD_INOUT},
  [29] = {"CHIDOUT",	STD_OUT},
  [28] = {"CHIDIN",	STD_IN},
  [27] = {"DREQ",	STD_IN},
  [26] = {"DGRINT",	STD_OUT},
  [25] = {"BC32K",	STD_OUT},
  [24] = {"TXD",	STD_OUT},
  [23] = {"RXD",	STD_IN},
  [22] = {"CS1",	STD_OUT},
  [21] = {"CS2",	STD_OUT},
  [20] = {"CS3",	STD_OUT},
  [19] = {"MCS0",	STD_OUT},
  [18] = {"MCS1",	STD_OUT},
  [17] = {"RXPWR",	STD_OUT},
  [16] = {"IROUT",	STD_OUT},
  [15] = {"SPICLK",	STD_OUT},
  [14] = {"SPIOUT",	STD_OUT},
  [13] = {"SPIN",	STD_IN},
  [12] = {"SIBMCLK",	STD_INOUT},
  [11] = {"CARDREG",	STD_OUT},
  [10] = {"CARDIOWR",	STD_OUT},
   [9] = {"CARDIORD",	STD_OUT},
   [8] = {"CARD1CSL",	STD_OUT},
   [7] = {"CARD1CSH",	STD_OUT},
   [6] = {"CARD2CSL",	STD_OUT},
   [5] = {"CARD2CSH",	STD_OUT},
   [4] = {"CARD1WAIT",	STD_IN},
   [3] = {"CARD2WAIT",	STD_IN},
   [2] = {"CARDDIR",	STD_OUT},
   [1] = {"MCS1WAIT",	0},
   [0] = {"MCS0WAIT",	0}
};
#endif /* TX392X */

#if defined TX391X && defined TX392X
#define tx39io_get_mfio_map(c)						\
	(IS_TX391X(c) ? tx391x_mfio_map : tx392x_mfio_map)
#define tx39io_get_io_max(c)						\
	(IS_TX391X(c) ? TX391X_IO_IO_MAX : TX392X_IO_IO_MAX)
#elif defined TX391X
#define tx39io_get_mfio_map(c)		tx391x_mfio_map
#define tx39io_get_io_max(c)		TX391X_IO_IO_MAX
#elif defined TX392X
#define tx39io_get_mfio_map(c)		tx392x_mfio_map
#define tx39io_get_io_max(c)		TX392X_IO_IO_MAX
#endif

#endif /* !__TX39IO_PRIVATE */

