/*	$NetBSD: gayle.h,v 1.1.10.1 2002/02/11 20:06:45 jdolecek Exp $	*/
#ifndef AMIGA_GAYLE_H_
#define AMIGA_GAYLE_H_

#include <sys/types.h>

struct gayle_struct {
	volatile u_int8_t	pcc_status;
/* Depending on the mode the card is in, most of the bits have different
   meanings */
#define GAYLE_CCMEM_DETECT	0x40
#define GAYLE_CCMEM_BVD1	0x20
#define GAYLE_CCMEM_BVD2	0x10
#define GAYLE_CCMEM_WP		0x08
#define GAYLE_CCMEM_BUSY	0x04

#define GAYLE_CCIO_STSCHG	0x20
#define GAYLE_CCIO_SPKR		0x10
#define GAYLE_CCIO_IREQ		0x04

	u_int8_t __pad0[0xfff];
	volatile u_int8_t	intreq;

	u_int8_t __pad1[0xfff];
	volatile u_int8_t	intena;
#define GAYLE_INT_IDE		0x80
#define GAYLE_INT_DETECT	0x40
#define GAYLE_INT_BVD1		0x20
#define GAYLE_INT_STSCHG	0x20
#define GAYLE_INT_BVD2		0x10
#define GAYLE_INT_SPKR		0x10
#define GAYLE_INT_WP		0x08
#define GAYLE_INT_BUSY		0x04
#define GAYLE_INT_IREQ		0x04
#define GAYLE_INT_IDEACK0	0x02
#define GAYLE_INT_IDEACK1	0x01

	u_int8_t __pad2[0xfff];
	volatile u_int8_t	pcc_config;
};

#define GAYLE_PCMCIA_START	0xa00000
#define GAYLE_PCMCIA_ATTR_START	0xa00000
#define GAYLE_PCMCIA_ATTR_END	0xa20000

#define GAYLE_PCMCIA_IO_START	0xa20000
#define GAYLE_PCMCIA_IO_END	0xa40000

#define GAYLE_PCMCIA_RESET	0xa40000
#define GAYLE_PCMCIA_END	0xa42000
#define NPCMCIAPG		btoc(GAYLE_PCMCIA_END - GAYLE_PCMCIA_START)

extern struct gayle_struct *gayle_base_virtual_address;
#define gayle (*gayle_base_virtual_address)

void gayle_init(void);

#endif /* AMIGA_GAYLE_H_ */
