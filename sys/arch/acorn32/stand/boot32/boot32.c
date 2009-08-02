/*	$NetBSD: boot32.c,v 1.37 2009/08/02 11:20:37 gavan Exp $	*/

/*-
 * Copyright (c) 2002 Reinoud Zandijk
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Thanks a bunch for Ben's framework for the bootloader and its suporting
 * libs. This file tries to actually boot NetBSD/acorn32 !
 *
 * XXX eventually to be partly merged back with boot26 ? XXX
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>
#include <riscoscalls.h>
#include <srt0.h>
#include <sys/boot_flag.h>
#include <machine/vmparam.h>
#include <arm/arm32/pte.h>
#include <machine/bootconfig.h>

extern char end[];

/* debugging flags */
int debug = 1;


/* constants */
#define PODRAM_START   (512*1024*1024)		/* XXX Kinetic cards XXX */

#define MAX_RELOCPAGES	4096

#define DEFAULT_ROOT	"/dev/wd0a"


#define IO_BLOCKS	 16	/* move these to the bootloader structure? */
#define ROM_BLOCKS	 16
#define PODRAM_BLOCKS	 16


/* booter variables */
char	 scrap[80], twirl_cnt;		/* misc				*/
char	 booted_file[80];

struct bootconfig *bconfig;		/* bootconfig passing		*/
u_long	 bconfig_new_phys;		/* physical address its bound	*/

/* computer knowledge		*/
u_int	 monitor_type, monitor_sync, ioeb_flags, lcd_flags;
u_int	 superio_flags, superio_flags_basic, superio_flags_extra;

/* sizes			*/
int	 nbpp, memory_table_size, memory_image_size;
/* relocate info		*/
u_long	 reloc_tablesize, *reloc_instruction_table;
u_long	*reloc_pos;			/* current empty entry		*/
int	 reloc_entries;			/* number of relocations	*/
int	 first_mapped_DRAM_page_index;	/* offset in RISC OS blob	*/
int	 first_mapped_PODRAM_page_index;/* offset in RISC OS blob	*/

struct page_info *mem_pages_info;	/* {nr, virt, phys}*		*/
struct page_info *free_relocation_page;	/* points to the page_info chain*/
struct page_info *relocate_code_page;	/* points to the copied code	*/
struct page_info *bconfig_page;		/* page for passing on settings	*/

unsigned char *memory_page_types;	/* packed array of 4 bit typeId	*/

u_long	*initial_page_tables;		/* pagetables to be booted from	*/


/* XXX rename *_BLOCKS to MEM_BLOCKS */
/* DRAM/VRAM/ROM/IO info */
/* where the display is		*/
u_long	 videomem_start, videomem_pages, display_size;

u_long	 pv_offset, top_physdram;	/* kernel_base - phys. diff	*/
u_long	 top_1Mb_dram;			/* the lower mapped top 1Mb	*/
u_long	 new_L1_pages_phys;		/* physical address of L1 pages	*/

/* for bootconfig passing	*/
u_long	 total_podram_pages, total_dram_pages, total_vram_pages;
int	 dram_blocks, podram_blocks;	/* number of mem. objects/type  */
int	 vram_blocks, rom_blocks, io_blocks;

u_long	 DRAM_addr[DRAM_BLOCKS],     DRAM_pages[DRAM_BLOCKS];
/* processor only RAM	*/
u_long	 PODRAM_addr[PODRAM_BLOCKS], PODRAM_pages[PODRAM_BLOCKS];
u_long	 VRAM_addr[VRAM_BLOCKS],     VRAM_pages[VRAM_BLOCKS];
u_long	 ROM_addr[ROM_BLOCKS],       ROM_pages[ROM_BLOCKS];
u_long	 IO_addr[IO_BLOCKS],         IO_pages[IO_BLOCKS];


/* RISC OS memory pages we claimed */
u_long	 firstpage, lastpage, totalpages; /* RISC OS pagecounters	*/
/* RISC OS memory		*/
char	*memory_image, *bottom_memory, *top_memory;

/* kernel info */
u_long	 marks[MARK_MAX];		/* loader mark pointers 	*/
u_long	 kernel_physical_start;		/* where does it get relocated	*/
u_long	 kernel_physical_maxsize;	/* Max allowed size of kernel	*/
u_long	 kernel_free_vm_start;		/* where does the free VM start	*/
/* some free space to mess with	*/
u_long	 scratch_virtualbase, scratch_physicalbase;


/* bootprogram identifiers */
extern const char bootprog_rev[];
extern const char bootprog_name[];
extern const char bootprog_date[];
extern const char bootprog_maker[];


/* predefines / prototypes */
void	 init_datastructures(void);
void	 get_memory_configuration(void);
void	 get_memory_map(void);
void	 create_initial_page_tables(void);
void	 add_pagetables_at_top(void);
int	 page_info_cmp(const void *a, const void *);
void	 add_initvectors(void);
void	 create_configuration(int argc, char **argv, int start_args);
void	 prepare_and_check_relocation_system(void);
void	 compact_relocations(void);
void	 twirl(void);
int	 vdu_var(int);
void	 process_args(int argc, char **argv, int *howto, char *file,
    int *start_args);

char		 *sprint0(int width, char prefix, char base, int value);
struct page_info *get_relocated_page(u_long destination, int size);

extern void start_kernel(
		int relocate_code_page,
		int relocation_pv_offset,
		int configuration_structure_in_flat_physical_space,
		int virtual_address_relocation_table,
		int physical_address_of_new_L1_pages,
		int kernel_entry_point
		);	/* asm */


/* the loader itself */
void
init_datastructures(void)
{

	/* Get number of pages and the memorytablesize */
	osmemory_read_arrangement_table_size(&memory_table_size, &nbpp);

	/* Allocate 99% - (small fixed amount) of the heap for memory_image */
	memory_image_size = (int)HIMEM - (int)end - 512 * 1024;
	memory_image_size /= 100;
	memory_image_size *= 99;
	if (memory_image_size <= 256*1024)
		panic("Insufficient memory");

	memory_image = alloc(memory_image_size);
	if (!memory_image)
		panic("Can't alloc get my memory image ?");

	bottom_memory = memory_image;
	top_memory    = memory_image + memory_image_size;

	firstpage  = ((int)bottom_memory / nbpp) + 1;	/* safety */
	lastpage   = ((int)top_memory    / nbpp) - 1;
	totalpages = lastpage - firstpage;

	printf("Allocated %ld memory pages, each of %d kilobytes.\n\n",
			totalpages, nbpp>>10 );

	/*
	 * Setup the relocation table. Its a simple array of 3 * 32 bit
	 * entries. The first word in the array is the number of relocations
	 * to be done
	 */
	reloc_tablesize = (MAX_RELOCPAGES+1)*3*sizeof(u_long);
	reloc_instruction_table = alloc(reloc_tablesize);
	if (!reloc_instruction_table)
		panic("Can't alloc my relocate instructions pages");

	reloc_entries = 0;
	reloc_pos     = reloc_instruction_table;
	*reloc_pos++  = 0;

	/*
	 * Set up the memory translation info structure. We need to allocate
	 * one more for the end of list marker. See get_memory_map.
	 */
	mem_pages_info = alloc((totalpages + 1)*sizeof(struct page_info));
	if (!mem_pages_info)
		panic("Can't alloc my phys->virt page info");

	/*
	 * Allocate memory for the memory arrangement table. We use this
	 * structure to retrieve memory page properties to clasify them.
	 */
	memory_page_types = alloc(memory_table_size);
	if (!memory_page_types)
		panic("Can't alloc my memory page type block");

	/*
	 * Initial page tables is 16 kb per definition since only sections are
	 * used.
	 */
	initial_page_tables = alloc(16*1024);
	if (!initial_page_tables)
		panic("Can't alloc my initial page tables");
}

void
compact_relocations(void)
{
	u_long *reloc_entry, current_length, length;
	u_long  src, destination, current_src, current_destination;
	u_long *current_entry;

	current_entry = reloc_entry = reloc_instruction_table + 1;

	/* prime the loop */
	current_src		= reloc_entry[0];
	current_destination	= reloc_entry[1];
	current_length		= reloc_entry[2];
	
	reloc_entry += 3;
	while (reloc_entry < reloc_pos) {
		src         = reloc_entry[0];
		destination = reloc_entry[1];
		length      = reloc_entry[2];

		if (src == (current_src + current_length) &&
		    destination == (current_destination + current_length)) {
			/* can merge */
			current_length += length;
		} else {
			/* nothing else to do, so save the length */
			current_entry[2] = current_length;
			/* fill in next entry */
			current_entry += 3;
			current_src = current_entry[0] = src;
			current_destination = current_entry[1] = destination;
			current_length = length;
		}
		reloc_entry += 3;
	}
	/* save last length */
	current_entry[2] = current_length;
	current_entry += 3;

	/* workout new count of entries */
	length = current_entry - (reloc_instruction_table + 1);
	printf("Compacted relocations from %d entries to %ld\n",
		       reloc_entries, length/3);

	/* update table to reflect new size */
	reloc_entries = length/3;
	reloc_instruction_table[0] = length/3;
	reloc_pos = current_entry;
}

void
get_memory_configuration(void)
{
	int loop, current_page_type, page_count, phys_page;
	int page, count, bank, top_bank, video_bank;
	int mapped_screen_memory;
	int one_mb_pages;
	u_long top;

	printf("Getting memory configuration ");

	osmemory_read_arrangement_table(memory_page_types);

	/* init counters */
	bank = vram_blocks = dram_blocks = rom_blocks = io_blocks =
	    podram_blocks = 0;

	current_page_type = -1;
	phys_page = 0;			/* physical address in pages	*/
	page_count = 0;			/* page counter in this block	*/
	loop = 0;			/* loop variable over entries	*/

	/* iterating over a packed array of 2 page types/byte i.e. 8 kb/byte */
	while (loop < 2*memory_table_size) {
		page = memory_page_types[loop / 2];	/* read	twice */
		if (loop & 1) page >>= 4;		/* take other nibble */

		/*
		 * bits 0-2 give type, bit3 means the bit page is
		 * allocatable
		 */
		page &= 0x7;			/* only take bottom 3 bits */
		if (page != current_page_type) {
			/* passed a boundary ... note this block	   */
			/*
			 * splitting in different vars is for
			 * compatability reasons
			 */
			switch (current_page_type) {
			case -1:
			case  0:
				break;
			case osmemory_TYPE_DRAM:
				if ((phys_page * nbpp)< PODRAM_START) {
					DRAM_addr[dram_blocks]  =
					    phys_page * nbpp;
					DRAM_pages[dram_blocks] =
					    page_count;
					dram_blocks++;
				} else {
					PODRAM_addr[podram_blocks]  =
					    phys_page * nbpp;
					PODRAM_pages[podram_blocks] =
					    page_count;
					podram_blocks++;
				}
				break;
			case osmemory_TYPE_VRAM:
				VRAM_addr[vram_blocks]  = phys_page * nbpp;
				VRAM_pages[vram_blocks] = page_count;
				vram_blocks++;
				break;
			case osmemory_TYPE_ROM:
				ROM_addr[rom_blocks]  = phys_page * nbpp;
				ROM_pages[rom_blocks] = page_count;
				rom_blocks++;
				break;
			case osmemory_TYPE_IO:
				IO_addr[io_blocks]  = phys_page * nbpp;
				IO_pages[io_blocks] = page_count;
				io_blocks++;
				break;
			default:
				printf("WARNING : found unknown "
				    "memory object %d ", current_page_type);
				printf(" at 0x%s",
				    sprint0(8,'0','x', phys_page * nbpp));
				printf(" for %s k\n",
				    sprint0(5,' ','d', (page_count*nbpp)>>10));
				break;
			}
			current_page_type = page;
			phys_page = loop;
			page_count = 0;
		}
		/*
		 * smallest unit we recognise is one page ... silly
		 * could be upto 64 pages i.e. 256 kb
		 */
		page_count += 1;
		loop       += 1;
		if ((loop & 31) == 0) twirl();
	}

	printf(" \n\n");

	if (VRAM_pages[0] == 0) {
		/* map DRAM as video memory */
		display_size	 =
		    vdu_var(os_VDUVAR_TOTAL_SCREEN_SIZE) & ~(nbpp-1);
#if 0
		mapped_screen_memory = 1024 * 1024; /* max allowed on RiscPC */
		videomem_pages   = (mapped_screen_memory / nbpp);
		videomem_start   = DRAM_addr[0];
		DRAM_addr[0]	+= videomem_pages * nbpp;
		DRAM_pages[0]	-= videomem_pages;
#else
		mapped_screen_memory = display_size;
		videomem_pages   = mapped_screen_memory / nbpp;
		one_mb_pages	 = (1024*1024)/nbpp;

		/*
		 * OK... we need one Mb at the top for compliance with current
		 * kernel structure. This ought to be abolished one day IMHO.
		 * Also we have to take care that the kernel needs to be in
		 * DRAM0a and even has to start there.
		 * XXX one Mb simms are the smallest supported XXX
		 */
		top_bank = dram_blocks-1;
		video_bank = top_bank;
		if (DRAM_pages[top_bank] == one_mb_pages) video_bank--;

		if (DRAM_pages[video_bank] < videomem_pages)
			panic("Weird memory configuration found; please "
			    "contact acorn32 portmaster.");

		/* split off the top 1Mb */
		DRAM_addr [top_bank+1]  = DRAM_addr[top_bank] +
		    (DRAM_pages[top_bank] - one_mb_pages)*nbpp;
		DRAM_pages[top_bank+1]  = one_mb_pages;
		DRAM_pages[top_bank  ] -= one_mb_pages;
		dram_blocks++;

		/* Map video memory at the end of the choosen DIMM */
		videomem_start          = DRAM_addr[video_bank] +
		    (DRAM_pages[video_bank] - videomem_pages)*nbpp;
		DRAM_pages[video_bank] -= videomem_pages;

		/* sanity */
		if (DRAM_pages[top_bank] == 0) {
			DRAM_addr [top_bank] = DRAM_addr [top_bank+1];
			DRAM_pages[top_bank] = DRAM_pages[top_bank+1];
			dram_blocks--;
		}
#endif
	} else {
		/* use VRAM */
		mapped_screen_memory = 0;
		videomem_start	 = VRAM_addr[0];
		videomem_pages	 = VRAM_pages[0];
		display_size	 = videomem_pages * nbpp;
	}

	if (mapped_screen_memory) {
		printf("Used %d kb DRAM ", mapped_screen_memory / 1024);
		printf("at 0x%s for video memory\n",
		    sprint0(8,'0','x', videomem_start));
	}

	/* find top of (PO)DRAM pages */
	top_physdram = 0;
	for (loop = 0; loop < podram_blocks; loop++) {
		top = PODRAM_addr[loop] + PODRAM_pages[loop]*nbpp;
		if (top > top_physdram) top_physdram = top;
	}
	for (loop = 0; loop < dram_blocks; loop++) {
		top = DRAM_addr[loop] + DRAM_pages[loop]*nbpp;
		if (top > top_physdram) top_physdram = top;
	}
	if (top_physdram == 0)
		panic("reality check: No DRAM in this machine?");
	if (((top_physdram >> 20) << 20) != top_physdram)
		panic("Top is not not aligned on a Mb; "
		    "remove very small DIMMS?");

	/* pretty print the individual page types */
	for (count = 0; count < rom_blocks; count++) {
		printf("Found ROM  (%d)", count);
		printf(" at 0x%s", sprint0(8,'0','x', ROM_addr[count]));
		printf(" for %s k\n",
		    sprint0(5,' ','d', (ROM_pages[count]*nbpp)>>10));
	}

	for (count = 0; count < io_blocks; count++) {
		printf("Found I/O  (%d)", count);
		printf(" at 0x%s", sprint0(8,'0','x', IO_addr[count]));
		printf(" for %s k\n",
		    sprint0(5,' ','d', (IO_pages[count]*nbpp)>>10));
	}

	/* for DRAM/VRAM also count the number of pages */
	total_dram_pages = 0;
	for (count = 0; count < dram_blocks; count++) {
		total_dram_pages += DRAM_pages[count];
		printf("Found DRAM (%d)", count);
		printf(" at 0x%s", sprint0(8,'0','x', DRAM_addr[count]));
		printf(" for %s k\n",
		    sprint0(5,' ','d', (DRAM_pages[count]*nbpp)>>10));
	}

	total_vram_pages = 0;
	for (count = 0; count < vram_blocks; count++) {
		total_vram_pages += VRAM_pages[count];
		printf("Found VRAM (%d)", count);
		printf(" at 0x%s", sprint0(8,'0','x', VRAM_addr[count]));
		printf(" for %s k\n",
		    sprint0(5,' ','d', (VRAM_pages[count]*nbpp)>>10));
	}

	total_podram_pages = 0;
	for (count = 0; count < podram_blocks; count++) {
		total_podram_pages += PODRAM_pages[count];
		printf("Found Processor only (S)DRAM (%d)", count);
		printf(" at 0x%s", sprint0(8,'0','x', PODRAM_addr[count]));
		printf(" for %s k\n",
		    sprint0(5,' ','d', (PODRAM_pages[count]*nbpp)>>10));
	}
}


void
get_memory_map(void)
{
	struct page_info *page_info;
	int	page, inout;
	int	phys_addr;

	printf("\nGetting actual memorymapping");
	for (page = 0, page_info = mem_pages_info;
	     page < totalpages;
	     page++, page_info++) {
		page_info->pagenumber = 0;	/* not used */
		page_info->logical    = (firstpage + page) * nbpp;
		page_info->physical   = 0;	/* result comes here */
		/* to avoid triggering a `bug' in RISC OS 4, page it in */
		*((int *)page_info->logical) = 0;
	}
	/* close list */
	page_info->pagenumber = -1;

	inout = osmemory_GIVEN_LOG_ADDR | osmemory_RETURN_PAGE_NO |
	    osmemory_RETURN_PHYS_ADDR;
	osmemory_page_op(inout, mem_pages_info, totalpages);

	printf(" ; sorting ");
	qsort(mem_pages_info, totalpages, sizeof(struct page_info),
	    &page_info_cmp);
	printf(".\n");

	/*
	 * get the first DRAM index and show the physical memory
	 * fragments we got
	 */
	printf("\nFound physical memory blocks :\n");
	first_mapped_DRAM_page_index = -1;
	first_mapped_PODRAM_page_index = -1;
	for (page=0; page < totalpages; page++) {
		phys_addr = mem_pages_info[page].physical;
		printf("[0x%x", phys_addr);
		while (mem_pages_info[page+1].physical - phys_addr == nbpp) {
			if (first_mapped_DRAM_page_index < 0 &&
			    phys_addr >= DRAM_addr[0])
				first_mapped_DRAM_page_index = page;
			if (first_mapped_PODRAM_page_index < 0 &&
			    phys_addr >= PODRAM_addr[0])
				first_mapped_PODRAM_page_index = page;
			page++;
			phys_addr = mem_pages_info[page].physical;
		}
		printf("-0x%x]  ", phys_addr + nbpp -1);
	}
	printf("\n\n");

	if (first_mapped_PODRAM_page_index < 0 && PODRAM_addr[0])
		panic("Found no (S)DRAM mapped in the bootloader");
	if (first_mapped_DRAM_page_index < 0)
		panic("No DRAM mapped in the bootloader");
}


void
create_initial_page_tables(void)
{
	u_long page, section, addr, kpage;

	/* mark a section by the following bits and domain 0, AP=01, CB=0 */
	/*         A         P         C        B        section
	           domain		*/
	section = (0<<11) | (1<<10) | (0<<3) | (0<<2) | (1<<4) | (1<<1) |
	    (0) | (0 << 5);

	/* first of all a full 1:1 mapping */
	for (page = 0; page < 4*1024; page++)
		initial_page_tables[page] = (page<<20) | section;

	/*
	 * video memory is mapped 1:1 in the DRAM section or in VRAM
	 * section
	 *
	 * map 1Mb from top of DRAM memory to bottom 1Mb of virtual memmap
	 */
	top_1Mb_dram = (((top_physdram - 1024*1024) >> 20) << 20);

	initial_page_tables[0] = top_1Mb_dram | section;

	/*
	 * map 16 Mb of kernel space to KERNEL_BASE
	 * i.e. marks[KERNEL_START]
	 */
	for (page = 0; page < 16; page++) {
		addr  = (kernel_physical_start >> 20) + page;
		kpage = (marks[MARK_START]     >> 20) + page;
		initial_page_tables[kpage] = (addr << 20) | section;
	}
}


void
add_pagetables_at_top(void)
{
	int page;
	u_long src, dst, fragaddr;

	/* Special : destination must be on a 16 Kb boundary */
	/* get 4 pages on the top of the physical memory and copy PT's in it */
	new_L1_pages_phys = top_physdram - 4 * nbpp;

	/*
	 * If the L1 page tables are not 16 kb aligned, adjust base
	 * until it is
	 */
	while (new_L1_pages_phys & (16*1024-1))
		new_L1_pages_phys -= nbpp;
	if (new_L1_pages_phys & (16*1024-1))
		panic("Paranoia : L1 pages not on 16Kb boundary");

	dst = new_L1_pages_phys;
	src = (u_long)initial_page_tables;

	for (page = 0; page < 4; page++) {
		/* get a page for a fragment */
		fragaddr = get_relocated_page(dst, nbpp)->logical;
		memcpy((void *)fragaddr, (void *)src, nbpp);

		src += nbpp;
		dst += nbpp;
	}
}


void
add_initvectors(void)
{
	u_long *pos;
	u_long  vectoraddr, count;

	/* the top 1Mb of the physical DRAM pages is mapped at address 0 */
	vectoraddr = get_relocated_page(top_1Mb_dram, nbpp)->logical;

	/* fill the vectors with `movs pc, lr' opcodes */
	pos = (u_long *)vectoraddr; memset(pos, 0, nbpp);
	for (count = 0; count < 128; count++) *pos++ = 0xE1B0F00E;
}

/*
 * Work out the display's vertical sync rate.  One might hope that there
 * would be a simpler way than by counting vsync interrupts for a second,
 * but if there is, I can't find it.
 */
static int
vsync_rate(void)
{
	uint8_t count0;
	unsigned int time0;

	count0 = osbyte_read(osbyte_VAR_VSYNC_TIMER);
	time0 = os_read_monotonic_time();
	while (os_read_monotonic_time() - time0 < 100)
		continue;
	return (u_int8_t)(count0 - osbyte_read(osbyte_VAR_VSYNC_TIMER));
}

void
create_configuration(int argc, char **argv, int start_args)
{
	int   i, root_specified, id_low, id_high;
	char *pos;

	bconfig_new_phys = kernel_free_vm_start - pv_offset;
	bconfig_page = get_relocated_page(bconfig_new_phys, nbpp);
	bconfig = (struct bootconfig *)(bconfig_page->logical);
	kernel_free_vm_start += nbpp;

	/* get some miscelanious info for the bootblock */
	os_readsysinfo_monitor_info(NULL, (int *)&monitor_type, (int *)&monitor_sync);
	os_readsysinfo_chip_presence((int *)&ioeb_flags, (int *)&superio_flags, (int *)&lcd_flags);
	os_readsysinfo_superio_features((int *)&superio_flags_basic,
	    (int *)&superio_flags_extra);
	os_readsysinfo_unique_id(&id_low, &id_high);

	/* fill in the bootconfig *bconfig structure : generic version II */
	memset(bconfig, 0, sizeof(*bconfig));
	bconfig->magic		= BOOTCONFIG_MAGIC;
	bconfig->version	= BOOTCONFIG_VERSION;
	strcpy(bconfig->kernelname, booted_file);

	/*
	 * get the kernel base name and update the RiscOS name to a
	 * Unix name
	 */
	i = strlen(booted_file);
	while (i >= 0 && booted_file[i] != '.') i--;
	if (i) {
		strcpy(bconfig->kernelname, "/");
		strcat(bconfig->kernelname, booted_file+i+1);
	}

	pos = bconfig->kernelname+1;
	while (*pos) {
		if (*pos == '/') *pos = '.';
		pos++;
	}

	/* set the machine_id */
	memcpy(&(bconfig->machine_id), &id_low, 4);

	/* check if the `root' is specified */
	root_specified = 0;
	strcpy(bconfig->args, "");
	for (i = start_args; i < argc; i++) {
		if (strncmp(argv[i], "root=",5) ==0) root_specified = 1;
		if (i > start_args)
			strcat(bconfig->args, " ");
		strcat(bconfig->args, argv[i]);
	}
	if (!root_specified) {
		if (start_args < argc)
			strcat(bconfig->args, " ");
		strcat(bconfig->args, "root=");
		strcat(bconfig->args, DEFAULT_ROOT);
	}

	/* mark kernel pointers */
	bconfig->kernvirtualbase	= marks[MARK_START];
	bconfig->kernphysicalbase	= kernel_physical_start;
	bconfig->kernsize		= kernel_free_vm_start -
					    marks[MARK_START];
	bconfig->ksym_start		= marks[MARK_SYM];
	bconfig->ksym_end		= marks[MARK_SYM] + marks[MARK_NSYM];

	/* setup display info */
	bconfig->display_phys		= videomem_start;
	bconfig->display_start		= videomem_start;
	bconfig->display_size		= display_size;
	bconfig->width			= vdu_var(os_MODEVAR_XWIND_LIMIT);
	bconfig->height			= vdu_var(os_MODEVAR_YWIND_LIMIT);
	bconfig->log2_bpp		= vdu_var(os_MODEVAR_LOG2_BPP);
	bconfig->framerate		= vsync_rate();

	/* fill in memory info */
	bconfig->pagesize		= nbpp;
	bconfig->drampages		= total_dram_pages +
					    total_podram_pages;	/* XXX */
	bconfig->vrampages		= total_vram_pages;
	bconfig->dramblocks		= dram_blocks + podram_blocks; /*XXX*/
	bconfig->vramblocks		= vram_blocks;

	for (i = 0; i < dram_blocks; i++) {
		bconfig->dram[i].address = DRAM_addr[i];
		bconfig->dram[i].pages   = DRAM_pages[i];
		bconfig->dram[i].flags   = PHYSMEM_TYPE_GENERIC;
	}
	for (; i < dram_blocks + podram_blocks; i++) {
		bconfig->dram[i].address = PODRAM_addr[i-dram_blocks];
		bconfig->dram[i].pages   = PODRAM_pages[i-dram_blocks];
		bconfig->dram[i].flags   = PHYSMEM_TYPE_PROCESSOR_ONLY;
	}
	for (i = 0; i < vram_blocks; i++) {
		bconfig->vram[i].address = VRAM_addr[i];
		bconfig->vram[i].pages   = VRAM_pages[i];
		bconfig->vram[i].flags   = PHYSMEM_TYPE_GENERIC;
	}
}


int
main(int argc, char **argv)
{
	int howto, start_args, ret;
	int class;

	printf("\n\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);
	printf(">> Booting NetBSD/acorn32 on a RiscPC/A7000/NC\n");
	printf("\n");

	process_args(argc, argv, &howto, booted_file, &start_args);

	printf("Booting %s (howto = 0x%x)\n", booted_file, howto);

	init_datastructures();
	get_memory_configuration();
	get_memory_map();

	/*
	 * point to the first free DRAM page guaranteed to be in
	 * strict order up
	 */
	if (podram_blocks != 0) {
		free_relocation_page =
		    mem_pages_info + first_mapped_PODRAM_page_index;
		kernel_physical_start = PODRAM_addr[0];
		kernel_physical_maxsize = PODRAM_pages[0] * nbpp;
	} else {
		free_relocation_page =
		    mem_pages_info + first_mapped_DRAM_page_index;
		kernel_physical_start = DRAM_addr[0];
		kernel_physical_maxsize = DRAM_pages[0] * nbpp;
	}

	printf("\nLoading %s ", booted_file);

	/* first count the kernel to get the markers */
	ret = loadfile(booted_file, marks, COUNT_KERNEL);
	if (ret == -1) panic("Kernel load failed"); /* lie to the user ... */
	close(ret);

	if (marks[MARK_END] - marks[MARK_START] > kernel_physical_maxsize) 
	{
		panic("\nKernel is bigger than the first DRAM module, unable to boot\n");
	}

	/*
	 * calculate how much the difference is between physical and
	 * virtual space for the kernel
	 */
	pv_offset = ((u_long)marks[MARK_START] - kernel_physical_start);
	/* round on a page	*/
	kernel_free_vm_start = (marks[MARK_END] + nbpp-1) & ~(nbpp-1);

	/* we seem to be forced to clear the marks[] ? */
	memset(marks, 0, sizeof(marks));

	/* really load it ! */
	ret = loadfile(booted_file, marks, LOAD_KERNEL);
	if (ret == -1) panic("Kernel load failed");
	close(ret);

	/* finish off the relocation information */
	create_initial_page_tables();
	add_initvectors();
	add_pagetables_at_top();
	create_configuration(argc, argv, start_args);

	/*
	 * done relocating and creating information, now update and
	 * check the relocation mechanism
	 */
	compact_relocations();

	/*
	 * grab a page to copy the bootstrap code into
	 */
	relocate_code_page = free_relocation_page++;
	
	printf("\nStarting at 0x%lx, p@0x%lx\n", marks[MARK_ENTRY], kernel_physical_start);
	printf("%ld entries, first one is 0x%lx->0x%lx for %lx bytes\n",
			reloc_instruction_table[0],
			reloc_instruction_table[1],
			reloc_instruction_table[2],
			reloc_instruction_table[3]);

	printf("Will boot in a few secs due to relocation....\n"
	    "bye bye from RISC OS!");

	/* dismount all filesystems */
	xosfscontrol_shutdown();

	os_readsysinfo_platform_class(&class, NULL, NULL);
	if (class != osreadsysinfo_Platform_Pace) {
		/* reset devices, well they try to anyway */
		service_pre_reset();
	}

	start_kernel(
		/* r0 relocation code page (V)	*/ relocate_code_page->logical,
		/* r1 relocation pv offset	*/
		relocate_code_page->physical-relocate_code_page->logical,
		/* r2 configuration structure	*/ bconfig_new_phys,
		/* r3 relocation table (l)	*/ 
		(int)reloc_instruction_table,	/* one piece! */
		/* r4 L1 page descriptor (P)	*/ new_L1_pages_phys,
		/* r5 kernel entry point	*/ marks[MARK_ENTRY]
	);
	return 0;
}


ssize_t
boot32_read(int f, void *addr, size_t size)
{
	void *fragaddr;
	size_t fragsize;
	ssize_t bytes_read, total;

	/* printf("read at %p for %ld bytes\n", addr, size); */
	total = 0;
	while (size > 0) {
		fragsize = nbpp;		/* select one page	*/
		if (size < nbpp) fragsize = size;/* clip to size left	*/

		/* get a page for a fragment */
		fragaddr = (void *)get_relocated_page((u_long) addr -
		    pv_offset, fragsize)->logical;

		bytes_read = read(f, fragaddr, fragsize);
		if (bytes_read < 0) return bytes_read;	/* error!	*/
		total += bytes_read;		/* account read bytes	*/

		if (bytes_read < fragsize)
			return total;		/* does this happen?	*/

		size -= fragsize;		/* advance		*/
		addr += fragsize;
	}
	return total;
}


void *
boot32_memcpy(void *dst, const void *src, size_t size)
{
	void *fragaddr;
	size_t fragsize;

	/* printf("memcpy to %p from %p for %ld bytes\n", dst, src, size); */
	while (size > 0) {
		fragsize = nbpp;		/* select one page	*/
		if (size < nbpp) fragsize = size;/* clip to size left	*/

		/* get a page for a fragment */
		fragaddr = (void *)get_relocated_page((u_long) dst -
		    pv_offset, fragsize)->logical;
		memcpy(fragaddr, src, size);

		src += fragsize;		/* account copy		*/
		dst += fragsize;
		size-= fragsize;
	}
	return dst;
}


void *
boot32_memset(void *dst, int c, size_t size)
{
	void *fragaddr;
	size_t fragsize;

	/* printf("memset %p for %ld bytes with %d\n", dst, size, c); */
	while (size > 0) {
		fragsize = nbpp;		/* select one page	*/
		if (size < nbpp) fragsize = size;/* clip to size left	*/

		/* get a page for a fragment */
		fragaddr = (void *)get_relocated_page((u_long)dst - pv_offset,
		    fragsize)->logical;
		memset(fragaddr, c, fragsize);

		dst += fragsize;		/* account memsetting	*/
		size-= fragsize;

	}
	return dst;
}


/* We can rely on the fact that two entries never have identical ->physical */
int
page_info_cmp(const void *a, const void *b)
{

	return (((struct page_info *)a)->physical <
	    ((struct page_info *)b)->physical) ? -1 : 1;
}

struct page_info *
get_relocated_page(u_long destination, int size)
{
	struct page_info *page;

	/* get a page for a fragment */
	page = free_relocation_page;
	if (free_relocation_page->pagenumber < 0) panic("\n\nOut of pages");
	reloc_entries++;
	if (reloc_entries >= MAX_RELOCPAGES)
		panic("\n\nToo many relocations! What are you loading ??");

	/* record the relocation */
	if (free_relocation_page->physical & 0x3)
		panic("\n\nphysical address is not aligned!");

	if (destination & 0x3)
		panic("\n\ndestination address is not aligned!");

	*reloc_pos++ = free_relocation_page->physical;
	*reloc_pos++ = destination;
	*reloc_pos++ = size;
	free_relocation_page++;			/* advance 		*/

	return page;
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


void
twirl(void)
{

	printf("%c%c", "|/-\\"[(int) twirl_cnt], 8);
	twirl_cnt++;
	twirl_cnt &= 3;
}


void
process_args(int argc, char **argv, int *howto, char *file, int *start_args)
{
	int i, j;
	static char filename[80];

	*howto = 0;
	*file = NULL; *start_args = 1;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			for (j = 1; argv[i][j]; j++)
				BOOT_FLAG(argv[i][j], *howto);
		else {
			if (*file)
				*start_args = i;
			else {
				strcpy(file, argv[i]);
				*start_args = i+1;
			}
			break;
		}
	}
	if (*file == NULL) {
		if (*howto & RB_ASKNAME) {
			printf("boot: ");
			gets(filename);
			strcpy(file, filename);
		} else
			strcpy(file, "netbsd");
	}
}


char *
sprint0(int width, char prefix, char base, int value)
{
	static char format[50], scrap[50];
	char *pos;
	int length;

	for (pos = format, length = 0; length<width; length++) *pos++ = prefix;
	*pos++ = '%';
	*pos++ = base;
	*pos++ = (char) 0;
	
	sprintf(scrap, format, value);
	length = strlen(scrap);

	return scrap+length-width;
}

