
#ifndef _EVBPPC_EXPLORA_H_
#define _EVBPPC_EXPLORA_H_

/*
 * Base addresses of external peripherals
 */
#define BASE_PCKBC	0x740000c0
#define BASE_PCKBC2	0x740000c8
#define BASE_COM	0x740005f0
#define BASE_LPT	0x740006f0
#define BASE_FB		0x70000000
#define BASE_FB2	0x71000000
#define BASE_LE		0x70800000

#define SIZE_FB		(2*1024*1024)

void consinit(void);

#endif /* _EVBPPC_EXPLORA_H_ */
