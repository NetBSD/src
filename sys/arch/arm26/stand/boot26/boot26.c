/*	$NetBSD: boot26.c,v 1.1 2001/07/27 23:13:50 bjh21 Exp $	*/

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <riscoscalls.h>
#include <sys/boot_flag.h>
#include <machine/boot.h>

extern const char bootprog_rev[];
extern const char bootprog_name[];
extern const char bootprog_date[];
extern const char bootprog_maker[];

int debug = 1;

enum pgstatus {	FREE, USED_RISCOS, USED_KERNEL, USED_BOOT };

int nbpp;
struct os_mem_map_request *pginfo;
enum pgstatus *pgstatus;

u_long marks[MARK_MAX];

char fbuf[80];

void get_mem_map(struct os_mem_map_request *, enum pgstatus *, int);
int vdu_var(int);

extern void start_kernel(struct bootconfig *, u_int, u_int);

int
main(int argc, char **argv)
{
	int npages, howto;
	int i, j;
	char *file;
	struct bootconfig bootconfig;
	int crow;
	int ret;

	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	os_read_mem_map_info(&nbpp, &npages);
	if (debug)
		printf("Machine has %d pages of %d KB each.  "
		    "Total RAM: %d MB\n", npages, nbpp >> 10,
		    (npages * nbpp) >> 20);

	/* Need one extra for teminator in OS_ReadMemMapEntries. */
	pginfo = alloc((npages + 1) * sizeof(*pginfo));
	if (pginfo == NULL)
		panic("cannot alloc pginfo array");
	memset(pginfo, 0, npages * sizeof(*pginfo));
	pgstatus = alloc(npages * sizeof(*pgstatus));
	if (pgstatus == NULL)
		panic("cannot alloc pgstatus array");
	memset(pgstatus, 0, npages * sizeof(*pgstatus));

	get_mem_map(pginfo, pgstatus, npages);

	howto = 0;
	file = NULL;
	for (i = 1; i < argc; i++)
		if (argv[i][0] == '-')
			for (j = 1; argv[i][j]; j++)
				BOOT_FLAG(argv[i][j], howto);
		else
			if (file)
				panic("Too many files!");
			else
				file = argv[i];
	if (file == NULL) {
		if (howto & RB_ASKNAME) {
			printf("boot: ");
			gets(fbuf);
			file = fbuf;
		} else
			file = "netbsd";
	}
	printf("Booting %s (howto = 0x%x)\n", file, howto);

	ret = loadfile(file, marks, LOAD_KERNEL);
	if (ret == -1)
		panic("Kernel load failed");
	close(ret);

	printf("Starting at 0x%lx\n", marks[MARK_ENTRY]);

	memset(&bootconfig, 0, sizeof(bootconfig));
	bootconfig.magic = BOOT_MAGIC;
	bootconfig.version = 0;
	bootconfig.boothowto = howto;
	bootconfig.bootdev = -1;
	bootconfig.ssym = marks[MARK_SYM] - 0x02000000;
	bootconfig.esym = marks[MARK_END] - 0x02000000;
	bootconfig.nbpp = nbpp;
	bootconfig.npages = npages;
	bootconfig.freebase = marks[MARK_END] - 0x02000000;
	bootconfig.xpixels = vdu_var(os_MODEVAR_XWIND_LIMIT) + 1;
	bootconfig.ypixels = vdu_var(os_MODEVAR_YWIND_LIMIT) + 1;
	bootconfig.bpp = 1 << vdu_var(os_MODEVAR_LOG2_BPP);
	bootconfig.screenbase = vdu_var(os_VDUVAR_DISPLAY_START) +
	    vdu_var(os_VDUVAR_TOTAL_SCREEN_SIZE) - 0x02000000;
	bootconfig.screensize = vdu_var(os_VDUVAR_TOTAL_SCREEN_SIZE);
	os_byte(osbyte_OUTPUT_CURSOR_POSITION, 0, 0, NULL, &crow);
	bootconfig.cpixelrow = crow * vdu_var(os_VDUVAR_TCHAR_SPACEY);

	if (bootconfig.bpp < 8)
		printf("WARNING: Current screen mode has fewer than eight "
							"bits per pixel.\n"
		    "         Console display may not work correctly "
		    					"(or at all).\n");
	/* Tear down RISC OS... */

	/* NetBSD will want the cache off initially. */
	xcache_control(0, 0, NULL);

	/* Dismount all filesystems. */
	xosfscontrol_shutdown();

	/* Ask device drivers to reset devices. */
	service_pre_reset();

	/* Disable interrupts. */
	os_int_off();

	start_kernel(&bootconfig, marks[MARK_ENTRY], 0x02090000);

	return 0;
}

void
get_mem_map(struct os_mem_map_request *pginfo, enum pgstatus *pgstatus,
    int npages)
{
	int i;

	for (i = 0; i < npages; i++)
		pginfo[i].page_no = i;
	pginfo[npages].page_no = -1;
	os_read_mem_map_entries(pginfo);

	if (debug)
		printf("--------/-------/-------/-------\n");
	for (i = 0; i < npages; i++) {
		pgstatus[i] = USED_RISCOS;
		if (pginfo[i].access == os_AREA_ACCESS_NONE) {
			if (debug) printf(".");
		} else {
			if (pginfo[i].map < (caddr_t)0x0008000) {
				if (debug) printf("0");
			} else if (pginfo[i].map < (caddr_t)HIMEM) {
				pgstatus[i] = USED_BOOT;
				if (debug) printf("+");
			} else if (pginfo[i].map < (caddr_t)0x1000000) {
				if (pginfo[i].access ==
				    os_AREA_ACCESS_READ_WRITE) {
					pgstatus[i] = FREE;
					if (debug) printf("*");
				} else {
					if (debug) printf("a");
				}
			} else if (pginfo[i].map < (caddr_t)0x1400000) {
				if (debug) printf("d");
			} else if (pginfo[i].map < (caddr_t)0x1800000) {
				if (debug) printf("s");
			} else if (pginfo[i].map < (caddr_t)0x1c00000) {
				if (debug) printf("m");
			} else if (pginfo[i].map < (caddr_t)0x1e00000) {
				if (debug) printf("h");
			} else if (pginfo[i].map < (caddr_t)0x1f00000) {
				if (debug) printf("f");
			} else if (pginfo[i].map < (caddr_t)0x2000000) {
				if (debug) printf("S");
			}
		}
		if (i % 32 == 31 && debug)
			printf("\n");
	}
}

ssize_t
boot26_read(int f, void *addr, size_t size)
{
	int ppn;
	caddr_t fragaddr;
	size_t fragsize;
	ssize_t retval, total;

	total = 0;
	while (size > 0) {
		ppn = ((u_int)addr - 0x02000000) / nbpp;
		if (pgstatus[ppn] != FREE)
			panic("Page %d not free", ppn);
		fragaddr = pginfo[ppn].map + ((u_int)addr % nbpp);
		fragsize = nbpp - ((u_int)addr % nbpp);
		if (fragsize > size)
			fragsize = size;
		retval = read(f, fragaddr, fragsize);
		if (retval < 0)
			return retval;
		total += retval;
		if (retval < fragsize)
			return total;
		addr += fragsize;
		size -= fragsize;
	}
	return total;
}

void *
boot26_memcpy(void *dst, const void *src, size_t size)
{
	int ppn;
	caddr_t fragaddr;
	size_t fragsize;
	void *addr = dst;

	while (size > 0) {
		ppn = ((u_int)addr - 0x02000000) / nbpp;
		if (pgstatus[ppn] != FREE)
			panic("Page %d not free", ppn);
		fragaddr = pginfo[ppn].map + ((u_int)addr % nbpp);
		fragsize = nbpp - ((u_int)addr % nbpp);
		if (fragsize > size)
			fragsize = size;
		memcpy(fragaddr, src, fragsize);
		addr += fragsize;
		src += fragsize;
		size -= fragsize;
	}
	return dst;
}

void *
boot26_memset(void *dst, int c, size_t size)
{
	int ppn;
	caddr_t fragaddr;
	size_t fragsize;
	void *addr = dst;

	while (size > 0) {
		ppn = ((u_int)addr - 0x02000000) / nbpp;
		if (pgstatus[ppn] != FREE)
			panic("Page %d not free", ppn);
		fragaddr = pginfo[ppn].map + ((u_int)addr % nbpp);
		fragsize = nbpp - ((u_int)addr % nbpp);
		if (fragsize > size)
			fragsize = size;
		memset(fragaddr, c, fragsize);
		addr += fragsize;
		size -= fragsize;
	}
	return dst;
}

int
vdu_var(int var)
{
	int varlist[2], vallist[2];

	varlist[0] = var;
	varlist[1] = -1;
	os_read_vdu_variables(varlist, vallist);
	return vallist[0];
}
