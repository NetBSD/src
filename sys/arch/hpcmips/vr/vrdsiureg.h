/*
 * Copyright (c) 2001, 2002 Greg Hughes. All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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

/*
 * DSIU (debug serial interface unit) register definitions
 */

/* Port Change Register */
#define DSIUPORT_REG_W		0x00
#define		DSIUPORT_CDDIN		(1 << 3)
#define		DSIUPORT_CDDOUT		(1 << 2)
#define		DSIUPORT_CDRTS		(1 << 1)
#define		DSIUPORT_CDCTS		(1 << 0)

/* Modem Control Register */
#define DSIUMODEM_REG_W		0x02
#define		DSIUMODEM_DRTS		(1 << 1)
#define		DSIUMODEM_DCTS		(1 << 0)

/* Asynchronous Mode 0 Register */
#define DSIUASIM00_REG_W	0x04
#define		DSIUASIM00_RXE0		(1 << 6)
#define		DSIUASIM00_PS0_MASK	(3 << 4)
#define			DSIUASIM00_PS00		(1 << 4)
#define			DSIUASIM00_PS01		(1 << 5)
#define		DSIUASIM00_CL0		(1 << 3)
#define		DSIUASIM00_SL0		(1 << 2)

/* Asynchronous Mode 1 Register */
#define DSIUASIM01_REG_W	0x06
#define		DSIUASIM01_EBS0		(1 << 0)

/* Recceive Buffer Register (Extended) */
#define DSIURXB0R_REG_W		0x08
#define		DSIURXB0R_RXB0_MASK		(0x1FF << 0)

/* Receive Buffer Register */
#define DSIURXB0L_REG_W		0x0A
#define		DSIURXB0L_RXB0L_MASK	(0xFF << 0)

/* Transmit Data Register (Extended) */
#define DSIUTXS0R_REG_W		0x0C
#define		DSIUTXS0R_TXS0_MASK		(0x1FF << 0)

/* Transmit Data Register */
#define DSIUTXS0L_REG_W		0x0E
#define		DSIUTXS0L_TXS0L_MASK	(0xFF << 0)

/* Status Register */
#define DSIUASIS0_REG_W		0x10
#define		DSIUASIS0_SOT0		(1 << 7)
#define		DSIUASIS0_PE0		(1 << 2)
#define		DSIUASIS0_FE0		(1 << 1)
#define		DSIUASIS0_OVE0		(1 << 0)

/* Debug SIU Interrupt Register */
#define DSIUINTR0_REG_W		0x12
#define		DSIUINTR0_INTDCD	(1 << 3)
#define		DSIUINTR0_INTSER0	(1 << 2)
#define		DSIUINTR0_INTSR0	(1 << 1)
#define		DSIUINTR0_INTST0	(1 << 0)

/* Baud rate Generator Prescaler Mode Register */
#define DSIUBPRM0_REG_W		0x16
#define		DSIUBPRM0_BRCE0		(1 << 7)
#define		DSIUBPRM0_BPR0_MASK	(7 << 0)
#define			DSIUBPRM0_BPR00		(1 << 0)
#define			DSIUBPRM0_BPR01		(1 << 1)
#define			DSIUBPRM0_BPR02		(1 << 2)

/* Debug SIU Reset Register */
#define DSIURESET_REG_W		0x18
#define		DSIURESET_DSIURST	(1 << 0)
