/*	$NetBSD: boot.h,v 1.7 2008/05/26 16:28:39 kiyohara Exp $	*/

#define	TICKS_PER_SEC	(33000000 / 4)		/* 33MHz */
#define	NS_PER_TICK	(1000000000 / TICKS_PER_SEC)

/*
 * srt0
 */
int endaddr(void);
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
 * CPU
 */
volatile int CPU1_alive;
void cpu1(void);
void resetCPU1(void);
unsigned long cpuState(void);
void runCPU1(void *);
void start_CPU1(void);
void wait_for(volatile int *);
int whichCPU(void);

/*
 * inkernel
 */
void init_in(void);

/*
 * io
 */
void outb(int, char);
void outw(int, u_short);
u_char inb(int);
u_long local_to_PCI(u_long);

/*
 * kbd
 */
int kbd(int);
void kbdreset(void);
int kbd_getc(void);
int kbd_test(void);

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
 * video
 */
void video_init(u_char *);
void video_putc(int);
