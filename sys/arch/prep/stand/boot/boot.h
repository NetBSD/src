/*	$NetBSD: boot.h,v 1.6 2006/04/10 18:40:06 garbled Exp $	*/

#define TICKS_PER_SEC	(33000000 / 4)		/* 33MHz */
#define NS_PER_TICK	(1000000000 / TICKS_PER_SEC)

/*
 * srt0
 */
void run(void *, void *, void *, void *, void *);

/*
 * clock
 */
void delay(u_int);

/*
 * com
 */
int comspeed(long);

/*
 * console
 */
char *cninit(int *, int *);
int cngetc(void);
void cnputc(int);
int cnscan(void);

/*
 * inkernel
 */
void init_in(u_long loadaddr);
int instrategy(void *, int , daddr_t, size_t, void *, size_t *);
int inopen(struct open_file *);
int inclose(struct open_file *);

/*
 * io
 */
void outb(int, char);
u_char inb(int);
u_long local_to_PCI(u_long);

/*
 * kbd
 */
int kbd(int);
void kbdreset(void);
int kbd_getc(void);

/*
 * monitor
 */
int db_monitor(void);

/*
 * tgets
 */
int tgets(char *);

/*
 * vga
 */
void vga_init(u_char *);
void vga_putc(int);
void vga_puts(char *);

/*
 * vreset
 */
void vga_reset(u_char *);
