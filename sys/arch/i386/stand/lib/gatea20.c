/*	$NetBSD: gatea20.c,v 1.6 2005/06/24 13:00:01 manu Exp $	*/

/* extracted from freebsd:sys/i386/boot/biosboot/io.c */

#include <sys/types.h>
#include <machine/pio.h>

#include <lib/libsa/stand.h>

#include "libi386.h"
#include "biosmca.h"

#define K_RDWR 		0x60		/* keyboard data & cmds (read/write) */
#define K_STATUS 	0x64		/* keyboard status */
#define K_CMD	 	0x64		/* keybd ctlr command (write-only) */

#define K_OBUF_FUL 	0x01		/* output buffer full */
#define K_IBUF_FUL 	0x02		/* input buffer full */

#define KC_CMD_WIN	0xd0		/* read  output port */
#define KC_CMD_WOUT	0xd1		/* write output port */
#define KB_A20		0x9f		/* enable A20,
					   reset (!),
					   enable output buffer full interrupt
					   enable data line
					   disable clock line */

/*
 * Gate A20 for high memory
 */
static unsigned char	x_20 = KB_A20;
void gateA20()
{
	__asm("pushfl ; cli");
	/*
	 * Not all systems enable A20 via the keyboard controller.
	 *	* IBM PS/2 L40
	 *	* AMD Elan SC520-based systems
	 */
	if (
#ifdef SUPPORT_PS2
	    biosmca_ps2model == 0xf82 ||
#endif
	    (inb(K_STATUS) == 0xff && inb(K_RDWR) == 0xff)) {
		int data;

		data = inb(0x92);
		outb(0x92, data | 0x2);
	} else {
		while (inb(K_STATUS) & K_IBUF_FUL);

		while (inb(K_STATUS) & K_OBUF_FUL)
			(void)inb(K_RDWR);

		outb(K_CMD, KC_CMD_WOUT);

		delay(100);
		while (inb(K_STATUS) & K_IBUF_FUL);

		outb(K_RDWR, x_20);

		delay(100);
		while (inb(K_STATUS) & K_IBUF_FUL);

		while (inb(K_STATUS) & K_OBUF_FUL)
			(void)inb(K_RDWR);
	}
	__asm("popfl");
}
