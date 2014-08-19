/*	$NetBSD: gayle.h,v 1.2.166.1 2014/08/20 00:02:42 tls Exp $	*/
#ifndef AMIGA_GAYLE_H_
#define AMIGA_GAYLE_H_

#include <sys/types.h>

#define GAYLE_IDE_BASE          0xDA0000
#define GAYLE_IDE_BASE_A4000	0xDD2020
#define GAYLE_IDE_INTREQ_A4000		0x1000	/* with stride of 1 */

#define GAYLE_REGS_BASE		0xDA8000

#define GAYLE_PCC_STATUS		0x0

/* Depending on the mode the card is in, most of the bits have different
   meanings */
#define GAYLE_CCMEM_DETECT			__BIT(6)	
#define GAYLE_CCMEM_BVD1			__BIT(5)
#define GAYLE_CCMEM_BVD2			__BIT(4)
#define GAYLE_CCMEM_WP				__BIT(3)
#define GAYLE_CCMEM_BUSY			__BIT(2)

#define GAYLE_CCIO_STSCHG			__BIT(5)
#define GAYLE_CCIO_SPKR				__BIT(4)	
#define GAYLE_CCIO_IREQ				__BIT(2)	

#define GAYLE_INTREQ			0x1 /* 0x1000 */
#define GAYLE_INTENA			0x2 /* 0x2000 */

#define GAYLE_INT_IDE				__BIT(7)	
#define GAYLE_INT_DETECT			__BIT(6)
#define GAYLE_INT_BVD1				__BIT(5)	
#define GAYLE_INT_STSCHG			__BIT(5)
#define GAYLE_INT_BVD2				__BIT(4)	
#define GAYLE_INT_SPKR				__BIT(4)	
#define GAYLE_INT_WP				__BIT(3)	
#define GAYLE_INT_BUSY				__BIT(2)	
#define GAYLE_INT_IREQ				__BIT(2)	
#define GAYLE_INT_IDEACK0			__BIT(1)
#define GAYLE_INT_IDEACK1			__BIT(0)

#define GAYLE_INT_IDEACK			(GAYLE_INT_IDEACK0 | GAYLE_INT_IDEACK1)

#define GAYLE_PCC_CONFIG		0x3 /* 0x3000 */	

#define GAYLE_PCMCIA_START	0xA00000
#define GAYLE_PCMCIA_ATTR_START	0xA00000
#define GAYLE_PCMCIA_ATTR_END	0xA20000

#define GAYLE_PCMCIA_IO_START	0xA20000
#define GAYLE_PCMCIA_IO_END	0xA40000

#define GAYLE_PCMCIA_RESET	0xA40000
#define GAYLE_PCMCIA_END	0xA42000
#define NPCMCIAPG		btoc(GAYLE_PCMCIA_END - GAYLE_PCMCIA_START)

/*
 * Convenience functions for expansions that have Gayle and even those that
 * don't have real Gayle but have implemented some portions of it for the sake
 * of compatibility.
 */
void gayle_init(void);
uint8_t gayle_intr_status(void);
uint8_t gayle_intr_enable_read(void);
void gayle_intr_enable_write(uint8_t);
void gayle_intr_enable_set(uint8_t);
void gayle_intr_ack(uint8_t);
uint8_t gayle_pcmcia_status_read(void);
void gayle_pcmcia_status_write(uint8_t);
uint8_t gayle_pcmcia_config_read(void);
void gayle_pcmcia_config_write(uint8_t);

#endif /* AMIGA_GAYLE_H_ */
