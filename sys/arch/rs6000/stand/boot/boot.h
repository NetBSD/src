/*	$NetBSD: boot.h,v 1.1.8.2 2008/01/21 09:39:00 yamt Exp $	*/

#define TICKS_PER_SEC	(66666667 / 4)		/* 66MHz */
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
int setup_iocc(void);
void outb(int, char);
inline void outw(int, u_int16_t);
u_char inb(int);

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

/*
 * iplcb.c
 */
void dump_iplcb(void *);
