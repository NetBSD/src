/*
 * $Id: if_ie507.h,v 1.1 1993/11/08 20:15:57 mycroft Exp $
 * Definitions for 3C507
 */

#define	IE507_CONTROL	6	/* control port */
#define IE507_ATTN 	11	/* any write here sends a Chan attn */
#define	IE507_MADDR	14	/* shared memory configuration */
#define	IE507_IRQ	15	/* IRQ configuration */

#define	EL_CTRL_WIDTH	0x10	/* bus width; clear = 8-bit, set = 16-bit */
#define	EL_CTRL_ONLINE	0x80	/* turn off to reset */

