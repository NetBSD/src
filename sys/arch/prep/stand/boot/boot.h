/*	$NetBSD: boot.h,v 1.10 2012/05/19 14:40:13 kiyohara Exp $	*/

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
void outb(int, char);
void outw(int, u_int16_t);
u_char inb(int);
u_char readb(u_long);
u_short readw(u_long);
u_long readl(u_long);
void writeb(u_long, u_char);
void writel(u_long, u_long);
void _wbinv(uint32_t, uint32_t);
void _inv(uint32_t, uint32_t);
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
 * pci
 */
void enablePCI(int, int, int, int);
int PCISlotnum(u_int, u_int, u_int);
int PCI_vendor(int);
u_long PCIAddress(int, u_int, int);
int scan_PCI(int);
void unlockVideo(int);

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
