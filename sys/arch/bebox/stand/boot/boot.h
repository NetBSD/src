/*	$NetBSD: boot.h,v 1.2 1999/06/28 01:20:44 sakamoto Exp $	*/

#define	TICKS_PER_SEC	(33000000 / 4)		/* 33MHz */
#define	NS_PER_TICK	(1000000000 / TICKS_PER_SEC)

/*
 * srt0
 */
extern int endaddr __P((void));
extern void run __P((void *, void *, void *, void *, void *));

/*
 * clock
 */
extern void delay __P((u_int));

/*
 * com
 */
extern int comspeed __P((long));

/*
 * console
 */
extern char *cninit __P((int *, int *));
extern int cngetc __P((void));
extern void cnputc __P((int));
extern int cnscan __P((void));

/*
 * cpu
 */
extern int CPU1_alive;
extern void cpu1 __P((void));
extern void resetCPU1 __P(());
extern void runCPU1 __P((void *));
extern void start_CPU1 __P((void));
extern void wait_for __P((volatile int *));
extern int whichCPU __P((void));

/*
 * inkernel
 */
extern void init_in __P((void));

/*
 * io
 */
extern void outb __P((int, char));
extern u_char inb __P((int));
extern u_long local_to_PCI __P((u_long));

/*
 * kbd
 */
extern int kbd __P((int));
extern void kbdreset __P((void));
extern int kbd_getc __P((void));

/*
 * video
 */
extern void video_init __P((u_char *));
extern void video_putc __P((int));

/*
 * vga
 */
extern void vga_init __P((u_char *));
extern void vga_putc __P((int));

/*
 * vreset
 */
extern void vga_reset __P((u_char *));
