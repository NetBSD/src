#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <machine/segments.h>

extern int i386_get_ldt(int, union descriptor *, int);
extern int i386_set_ldt(int, union descriptor *, int);

inline void set_fs(unsigned long val)
{
	__asm__ __volatile__("mov %0,%%fs"::"r" ((unsigned short) val));
}

inline unsigned char get_fs_byte(const char * addr)
{
	unsigned register char _v;

	__asm__ ("movb %%fs:%1,%0":"=q" (_v):"m" (*addr));
	return _v;
}

inline unsigned short get_cs(void)
{
	unsigned register short _v;

	__asm__ ("movw %%cs,%0"::"r" ((unsigned short) _v));
	return _v;
}

int
check_desc(unsigned int desc)
{
   desc = LSEL(desc, SEL_UPL);
   set_fs(desc);
   return(get_fs_byte((char *) 0));
}

void
gated_call()
{
   printf("Called from call gate\n");
   __asm__ __volatile__("popl %ebp");
   __asm__ __volatile__(".byte 0xcb");
}

struct segment_descriptor *
make_sd(unsigned base, unsigned limit, int type, int dpl, int seg32, int inpgs)
{
        static struct segment_descriptor d;

        d.sd_lolimit = limit & 0x0000ffff;
        d.sd_lobase  = base & 0x00ffffff;
        d.sd_type    = type & 0x01f;
        d.sd_dpl     = dpl & 0x3;
        d.sd_p       = 1;
        d.sd_hilimit = (limit & 0x00ff0000) >> 16;
        d.sd_xx      = 0;
        d.sd_def32   = seg32?1:0;
        d.sd_gran    = inpgs?1:0;
        d.sd_hibase  = (base & 0xff000000) >> 24;

        return (&d);
}

struct gate_descriptor *
make_gd(unsigned offset, unsigned int sel, unsigned stkcpy, int type, int dpl)
{
        static struct gate_descriptor d;

        d.gd_looffset = offset & 0x0000ffff;
        d.gd_selector = sel & 0xffff;
        d.gd_stkcpy   = stkcpy & 0x1ff;
        d.gd_type     = type & 0x1ff;
        d.gd_dpl      = dpl & 0x3;
        d.gd_p        = 1;
        d.gd_hioffset = (offset & 0xffff0000) >> 16;

        return(&d);
}

void
print_ldt(union descriptor *dp)
{
    unsigned long base_addr, limit, offset, selector, stack_copy;
    int type, dpl, i;
    unsigned long *lp = (unsigned long *)dp;
    
    /* First 32 bits of descriptor */
    selector = base_addr = (*lp >> 16) & 0x0000FFFF;
    offset = limit = *lp & 0x0000FFFF;
    lp++;
	
    /* First 32 bits of descriptor */
    base_addr |= (*lp & 0xFF000000) | ((*lp << 16) & 0x00FF0000);
    limit |= (*lp & 0x000F0000);
    type = dp->sd.sd_type;
    dpl = dp->sd.sd_dpl;
    stack_copy = dp->gd.gd_stkcpy;
    offset |= (*lp >> 16) & 0x0000FFFF;
    
    if (type == SDT_SYS386CGT || type == SDT_SYS286CGT)
            printf("LDT: Gate Off %08.8x, Sel   %05.5x, Stkcpy %d DPL %d, Type %d\n",
                   offset, selector, stack_copy, dpl, type);
    else
            printf("LDT: Seg Base %08.8x, Limit %05.5x, DPL %d, Type %d\n",
                   base_addr, limit, dpl, type);
    printf("          ");
    if (*lp & 0x100)
            printf("Accessed, ");
    if (*lp & 8000)
            printf("Present, ");
    if (type != SDT_SYS386CGT && type != SDT_SYS286CGT) {
            if (*lp & 0x100000)
                    printf("User, ");
            if (*lp & 0x200000)
                    printf("X, ");
            if (*lp & 0x400000)
                    printf("32, ");
            else
                    printf("16, ");
            if (*lp & 0x800000)
                    printf("page limit, ");
            else
                    printf("byte limit, ");
    }
    printf("\n");
    printf("          %08.8x %08.8x\n", *(lp), *(lp-1));
}
#define MAX_USER_LDT 1024
main()
{
        union descriptor ldt[MAX_USER_LDT];
        int num, n;
        unsigned int cs = get_cs();
	char *data;
        struct segment_descriptor *sd;
        struct gate_descriptor *gd;
        
        printf("Before call\n");
        if ((num = i386_get_ldt(0, ldt, MAX_USER_LDT)) < 0) {
                perror("get_ldt");
                exit(2);
        }
        printf("Got %d (default) LDTs\n", num);
        for (n=0; n < num; n++) {
                printf("Entry %d: ", n);
                print_ldt(&ldt[n]);
        }
#if 0        
	data = (char *)malloc(4096);
#else
	data = (void *) mmap( (char *)0x005f0000, 0x0fff,
                             PROT_EXEC | PROT_READ | PROT_WRITE,
                             MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
	if (data == NULL) {
		printf("Error mallocing\n");
		exit(1);
	}
	printf("Malloc address: %8.8x\n", data);
	*data = 0x97;

	/* Get the next free LDT and set it to the allocated data. */
        sd = make_sd((unsigned)data, 4096, SDT_MEMRW, SEL_UPL, 0, 0);
        if ((num = i386_set_ldt(6, (union descriptor *)sd, 1)) < 0) {
                perror("set_ldt");
                exit(1);
        }
	printf("setldt returned: %d\n", num);
        if ((n = i386_get_ldt(num, ldt, 1)) < 0) {
                perror("get_ldt");
                exit(1);
        }
        printf("Entry %d: ", num);
        print_ldt(&ldt[0]);

#if 1
	printf("Checking desc (should be 0x97): 0x%x\n", check_desc(num));
        if (check_desc(num) != 0x97)
                exit(1);
#endif
#if 1
        gd = make_gd((unsigned)gated_call, cs, 0, SDT_SYS386CGT, SEL_UPL);
        if ((num = i386_set_ldt(5, (union descriptor *)gd, 1)) < 0) {
                perror("set_ldt: call gate");
                exit(1);
        }
	printf("setldt returned: %d\n", num);
        printf("Call gate sel = 0x%x\n", LSEL(num, SEL_UPL));
#endif
#if 1
        if ((n = i386_get_ldt(num, ldt, 1)) < 0) {
                perror("get_ldt");
                exit(1);
        }
        printf("Entry %d: ", num);
        print_ldt(&ldt[0]);
#endif        
#if 0
	err = setldt(5,
                     gated_call,	/* Offset */
		     0x0001,		/* This selector is for the executable segment descriptor.  It
					   is the standard linux text descriptor. */
		     0x00008c00);	/* Descriptor flags (you can't set all, the OS protects some) */
	printf("setldt returned: %d\n", err);
#endif
#if 1
	__asm__ __volatile__(".byte 0x9a"); /* This is a call to a call gate. */
	__asm__ __volatile__(".byte 0x00"); /* Value is ignored in a call gate but can be used. */
	__asm__ __volatile__(".byte 0x00"); /* by the called procedure. */
	__asm__ __volatile__(".byte 0x00");
	__asm__ __volatile__(".byte 0x00");
	__asm__ __volatile__(".byte 0x2f"); /* Selector 0x002f.  This is index = 5 (the call gate), */
	__asm__ __volatile__(".byte 0x00"); /* and a requestor priveledge level of 3. */

	printf("Gated call returned\n");
#endif
        exit (0);
}
