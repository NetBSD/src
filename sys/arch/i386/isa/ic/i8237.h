/*
 * Intel 8237 DMA Controller
 *
 *	$Id: i8237.h,v 1.2.4.1 1993/10/14 05:19:13 mycroft Exp $
 */

#define	DMA37MD_SINGLE	0x40	/* single pass mode */
#define	DMA37MD_CASCADE	0xc0	/* cascade mode */
#define	DMA37MD_WRITE	0x04	/* read the device, write memory operation */
#define	DMA37MD_READ	0x08	/* write the device, read memory operation */
	
#define	DMA37SM_CLEAR	0x00	/* clear mask bit */
#define	DMA37SM_SET	0x04	/* set mask bit */
