/*	$NetBSD: hydrareg.h,v 1.2.6.1 2002/10/26 10:26:38 bjh21 Exp $	*/

/*
 * This file is in the Public Domain
 */

/* Simtec Hydra register definitions */

/*
 * Documentation is at
 * <http://www.simtec.co.uk/products/AUHYDRA/files/api.txt>.
 */

#define HYDRA_PHYS_BASE		0x03800000
#define HYDRA_PHYS_SIZE		0x40

/* Registers are 4 bits wide at 1-word intervals. */

/* Write-only registers */
#define HYDRA_FIQ_SET		0
#define HYDRA_FIQ_CLR		1
#define HYDRA_FORCEFIQ_CLR	2
#define HYDRA_MMU_LSN		4
#define HYDRA_MMU_MSN		5
#define HYDRA_MMU_SET		6
#define HYDRA_MMU_CLR		7
#define HYDRA_IRQ_SET		8
#define HYDRA_IRQ_CLR		9
#define HYDRA_FORCEIRQ_CLR	10
#define HYDRA_RESET		12
#define HYDRA_X86_KILLER	13
#define HYDRA_HALT_SET		14
#define HYDRA_HALT_CLR		15

/* Read-only registers */
#define HYDRA_FIQ_STATUS	0
#define HYDRA_FIQ_READBACK	1
#define HYDRA_HARDWAREVER	2
#define HYDRA_MMU_STATUS	6
#define HYDRA_ID_STATUS		7
#define HYDRA_IRQ_STATUS	8
#define HYDRA_IRQ_READBACK	9
#define HYDRA_RST_STATUS	12
#define HYDRA_HALT_STATUS	14

#define HYDRA_NSLAVES	4

#define HYDRA_ID_ISSLAVE	0x4
#define HYDRA_ID_SLAVE_MASK	0x3
