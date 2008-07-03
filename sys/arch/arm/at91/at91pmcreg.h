/*	$Id: at91pmcreg.h,v 1.1.22.1 2008/07/03 18:37:51 simonb Exp $	*/

#ifndef	_AT91PMCREG_H_
#define	_AT91PMCREG_H_	1

/* Power Management Controller (PMC), at,
 * at91rm9200.pdf, page 271 */

#define	PMC_NUM_PCLOCKS	8

#define	PMC_SCER	0x00U		/* 00: System Clock Enable Reg	*/
#define	PMC_SCDR	0x04U		/* 04: System Clock Disable Reg */
#define	PMC_SCSR	0x08U		/* 08: System Clock Status Reg	*/
#define	PMC_PCER	0x10U		/* 10: Peripheral Clock Enable	*/
#define	PMC_PCDR	0x14U		/* 14: Peripheral Clock Disable	*/
#define	PMC_PCSR	0x18U		/* 18: Peripheral Clock Status	*/
#define	PMC_MOR		0x20U		/* 20: Main Oscillator Reg	*/
#define	PMC_MCFR	0x24U		/* 24: Main Clock Freq Reg	*/
#define	PMC_PLLAR	0x28U		/* 28: PLL A Register#define	PMC_*/
#define	PMC_PLLBR	0x2CU		/* 2C: PLL B Register		*/
#define	PMC_MCKR	0x30U		/* 30: Master Clock Register	*/
#define	PMC_PCK(num)	(0x40U + (num) * 4U)	/* 40: Programmable Clocks	*/
#define	PMC_IER		0x60U		/* 60: Interrupt Enable Reg	*/
#define	PMC_IDR		0x64U		/* 64: Interrupt Disable Reg	*/
#define	PMC_SR		0x68U		/* 68: Status Register		*/
#define	PMC_IMR		0x6CU		/* 6C: Interrupt Mask Reg	*/

/* System Clock Enable Register bits: */
#define	PMC_SCSR_PCK3	0x0800U
#define	PMC_SCSR_PCK2	0x0400U
#define	PMC_SCSR_PCK1	0x0200U
#define	PMC_SCSR_PCK0	0x0100U
#define	PMC_SCSR_UHP	0x0010U		/* 1 = Enable USB Host Port clks */
#define	PMC_SCSR_MCKUDP	0x0004U		/* 1 = enable Master Clock dis	*/
#define	PMC_SCSR_UDP	0x0002U		/* 1 = enable USB Device Port clk */
#define	PMC_SCSR_PCK	0x0001U		/* 1 = enable the processor clk	*/

/* Main Oscillator Register bits: */
#define	PMC_MOR_OSCOUNT	0xFF00U		/* start-up-time / 8		*/
#define	PMC_MOR_OSCOUNT_SHIFT 8U
#define	PMC_MOR_MOSCEN	0x1U		/* 1 = main oscillator enabled	*/

/* Main Clock Frequency Register bits: */
#define	PMC_MCFR_MAINRDY	0x10000U	/* 1= main clock ready		*/
#define	PMC_MCFR_MAINF		0x0FFFFU

/* PLL Register bits: */
#define	PMC_PLL_MUL		0x07FF0000U
#define	PMC_PLL_MUL_SHIFT	16U
#define	PMC_PLL_OUT		0x0000C000U
#define	PMC_PLL_OUT_80_TO_160	0x00000000U
#define	PMC_PLL_OUT_150_TO_240	0x00008000U
#define	PMC_PLL_PLLCOUNT	0x00003F00U
#define	PMC_PLL_DIV		0x000000FFU
#define	PMC_PLL_DIV_SHIFT	0U

/* PLL B Register bits: */
#define	PMC_PLLBR_USB_96M	0x10000000U /* 1 = USB clks = PLL B output / 2	*/

/* Master Clock Register bits: */
#define	PMC_MCKR_MDIV		0x300U
#define	PMC_MCKR_MDIV_1		0x000U
#define	PMC_MCKR_MDIV_2		0x100U
#define	PMC_MCKR_MDIV_3		0x200U
#define	PMC_MCKR_MDIV_4		0x300U
#define	PMC_MCKR_MDIV_SHIFT	8U

#define	PMC_MCKR_PRES		0x01CU
#define	PMC_MCKR_PRES_SHIFT	2U
#define	PMC_MCKR_PRES_1		(0U<<PMC_MCKR_PRES_SHIFT)
#define	PMC_MCKR_PRES_2		(1U<<PMC_MCKR_PRES_SHIFT)
#define	PMC_MCKR_PRES_4		(2U<<PMC_MCKR_PRES_SHIFT)
#define	PMC_MCKR_PRES_8		(3U<<PMC_MCKR_PRES_SHIFT)
#define	PMC_MCKR_PRES_16	(4U<<PMC_MCKR_PRES_SHIFT)
#define	PMC_MCKR_PRES_32	(5U<<PMC_MCKR_PRES_SHIFT)	
#define	PMC_MCKR_PRES_64	(6U<<PMC_MCKR_PRES_SHIFT)	

#define	PMC_MCKR_CSS		0x003U
#define	PMC_MCKR_CSS_SLOW_CLK	0x000U
#define	PMC_MCKR_CSS_MAIN_CLK	0x001U
#define	PMC_MCKR_CSS_PLLA	0x002U
#define	PMC_MCKR_CSS_PLLB	0x003U

/* Programmable Clock Register bits: */
#define	PMC_PCK_PRES		PMC_MCKR_PRES
#define	PMC_PCK_PRES_SHIFT	PMC_MCKR_PRES_SHIFT
#define	PMC_PCK_PRES_1		PMC_MCKR_PRES_1
#define	PMC_PCK_PRES_2		PMC_MCKR_PRES_2
#define	PMC_PCK_PRES_4		PMC_MCKR_PRES_4
#define	PMC_PCK_PRES_8		PMC_MCKR_PRES_8
#define	PMC_PCK_PRES_16		PMC_MCKR_PRES_16
#define	PMC_PCK_PRES_32		PMC_MCKR_PRES_32
#define	PMC_PCK_PRES_64		PMC_MCKR_PRES_64

#define	PMC_PCK_CSS		PMC_MCKR_CSS
#define	PMC_PCK_CSS_SLOW_CLK	PMC_MCKR_CSS_SLOW_CLK
#define	PMC_PCK_CSS_CLKC	PMC_MCKR_CSS_CLKC
#define	PMC_PCK_CSS_CLKA	PMC_MCKR_CSS_CLKA
#define	PMC_PCK_CSS_CLKB	PMC_MCKR_CSS_CLKB


/* Interrupt Enable Register bits: */
#define	PMC_SR_PCK7RDY		0x8000U
#define	PMC_SR_PCK6RDY		0x4000U
#define	PMC_SR_PCK5RDY		0x2000U
#define	PMC_SR_PCK4RDY		0x1000U
#define	PMC_SR_PCK3RDY		0x0800U
#define	PMC_SR_PCK2RDY		0x0400U
#define	PMC_SR_PCK1RDY		0x0200U
#define	PMC_SR_PCK0RDY		0x0100U
#define	PMC_SR_MCKRDY		0x0008U
#define	PMC_SR_LOCKB		0x0004U
#define	PMC_SR_LOCKA		0x0002U
#define	PMC_SR_MOSCS		0x0001U

#define	PMCREG(offset)		*((volatile uint32_t*)(0xfffffc00UL + (offset)))

#endif /* !_AT91PMCREG_H_ */
