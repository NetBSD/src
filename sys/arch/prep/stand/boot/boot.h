/*	$NetBSD: boot.h,v 1.3 2001/06/19 11:56:28 nonaka Exp $	*/

#define TICKS_PER_SEC	(33000000 / 4)		/* 33MHz */
#define NS_PER_TICK	(1000000000 / TICKS_PER_SEC)

/*
 * srt0
 */
void run __P((void *, void *, void *, void *, void *));

/*
 * clock
 */
void delay __P((u_int));

/*
 * com
 */
int comspeed __P((long));

/*
 * console
 */
char *cninit __P((int *, int *));
int cngetc __P((void));
void cnputc __P((int));
int cnscan __P((void));

/*
 * fd
 */
int fdstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int fdopen __P((struct open_file *, int, int, int));
int fdclose __P((struct open_file *));

/*
 * inkernel
 */
void init_in __P((void));
int instrategy __P((void *, int , daddr_t, size_t, void *, size_t *));
int inopen __P((struct open_file *));
int inclose __P((struct open_file *));

/*
 * io
 */
void outb __P((int, char));
u_char inb __P((int));
u_long local_to_PCI __P((u_long));

/*
 * kbd
 */
int kbd __P((int));
void kbdreset __P((void));
int kbd_getc __P((void));

/*
 * monitor
 */
int db_monitor __P((void));

/*
 * tgets
 */
int tgets __P((char *));

/*
 * video
 */
void video_init __P((u_char *));
void video_putc __P((int));

/*
 * vga
 */
void vga_init __P((u_char *));
void vga_putc __P((int));
void vga_puts __P((char *));

/*
 * vreset
 */
void vga_reset __P((u_char *));
