/* $NetBSD: iplcb.c,v 1.1.4.2 2008/01/02 21:50:05 bouyer Exp $ */

#include <lib/libsa/stand.h>
#include <sys/param.h>

#include "boot.h"
#include <machine/iplcb.h>

extern struct ipl_directory ipldir;

static void dump_sysinfo(void *);
/* dump the sysinfo structure */
static void
dump_sysinfo(void *sysinfo_p)
{
	struct sys_info *sysinfo = (struct sys_info *)sysinfo_p;

	printf("  System Info: %p\n", sysinfo_p);
	printf("    Processors: %d\n", sysinfo->nrof_procs);
	printf("    Coherence Block size: 0x%x\n", sysinfo->coherency_size);
	printf("    Reservation Granule size: 0x%x\n", sysinfo->resv_size);
	printf("    Arbiter Controller RA: %p\n", sysinfo->arb_ctrl_addr);
	printf("    Physical ID Register RA: %p\n", sysinfo->phys_id_addr);
	printf("    # of BOS Slot Reset Registers: %d\n", sysinfo->nrof_bsrr);
	printf("    BSSR RA: %p\n", sysinfo->bssr_addr);
	printf("    Time of day type: %d\n", sysinfo->tod_type);
	printf("    TOD Register RA: %p\n", sysinfo->todr_addr);
	printf("    Reset Status Register RA: %p\n", sysinfo->rsr_addr);
	printf("    Power/Keylock Status RA: %p\n", sysinfo->pksr_addr);
	printf("    Power/Reset Control RA: %p\n", sysinfo->prcr_addr);
	printf("    System Specific Register RA: %p\n", sysinfo->sssr_addr);
	printf("    System Interrupt Regs RA: %p\n", sysinfo->sir_addr);
	printf("    Standard Config Register RA: %p\n", sysinfo->scr_addr);
	printf("    Device Special Config Register RA: %p\n", sysinfo->dscr_addr);
	printf("    NVRAM size: %d\n", sysinfo->nvram_size);
	printf("    NVRAM RA: %p\n", sysinfo->nvram_addr);
	printf("    VPD ROM RA: %p\n", sysinfo->vpd_rom_addr);
	printf("    IPL ROM size: %d\n", sysinfo->ipl_rom_size);
	printf("    IPL ROM RA: %p\n", sysinfo->ipl_rom_addr);
	printf("    Global MFFR Register RA: %p\n", sysinfo->g_mfrr_addr);
	printf("    Global Timebase RA: %p\n", sysinfo->g_tb_addr);
	printf("    Global Timebase type: %d\n", sysinfo->g_tb_type);
	printf("    Global Timebase multiplier: %d\n", sysinfo->g_tb_mult);
	printf("    SP Errorlog Offset: 0x%x\n", sysinfo->sp_errorlog_off);
	printf("    Connectivity Config Register RA: %p\n", sysinfo->pcccr_addr);
	printf("    Software Poweroff Register RA: %p\n", sysinfo->spocr_addr);
	printf("    EPOW intr register RA: %p\n", sysinfo->pfeivr_addr);
	printf("    Access ID waddr: %d\n", sysinfo->access_id_waddr);
	printf("    APM write space RA: %p\n", sysinfo->loc_waddr);
	printf("    Access ID raddr: %d\n", sysinfo->access_id_raddr);
	printf("    APM read space RA: %p\n", sysinfo->loc_raddr);
	printf("    Architecture: %d\n", sysinfo->architecture);
	printf("    Implementation: %d\n", sysinfo->implementation);
	printf("    Machine Description: %s\n", sysinfo->pkg_desc);
}

/* Dump the IPL control block */
void
dump_iplcb(void *iplcb_p)
{
	u_char *p;
	uint32_t i;
	struct ipl_directory *ipldir;
	struct ipl_cb *iplcb_ptr;

	iplcb_ptr = iplcb_p;
	ipldir = &(iplcb_ptr->dir);

	printf("IPL Control Block\n");
	printf("  IPL Control Block Address: %p\n", iplcb_p);
	printf("IPL Directory Block\n");
	printf("  IPLCB_ID: %s\n", ipldir->iplcb_id);
	printf("  GPR offset: 0x%x\n", ipldir->gpr_save_off);
	printf("  Bitmap size: 0x%x\n", ipldir->cb_bitmap_size);
	printf("  Bitmap offset: 0x%x\n", ipldir->bitmap_off);
	printf("  Bitmap size: 0x%x\n", ipldir->bitmap_size);
	printf("  IPL info offset: 0x%x\n", ipldir->iplinfo_off);
	printf("  IPL info size: 0x%x\n", ipldir->iplinfo_size);
	printf("  IOCC POST offset: 0x%x\n", ipldir->iocc_post_off);
	printf("  IOCC POST size: 0x%x\n", ipldir->iocc_post_size);
	printf("  NIO POST offset: 0x%x\n", ipldir->nio_post_off);
	printf("  NIO POST size: 0x%x\n", ipldir->nio_post_size);
	printf("  SJL POST offset: 0x%x\n", ipldir->sjl_post_off);
	printf("  SJL POST size: 0x%x\n", ipldir->sjl_post_size);
	printf("  SCSI POST offset: 0x%x\n", ipldir->scsi_post_off);
	printf("  SCSI POST size: 0x%x\n", ipldir->scsi_post_size);
	printf("  Ethernet POST offset: 0x%x\n", ipldir->eth_post_off);
	printf("  Ethernet POST size: 0x%x\n", ipldir->eth_post_size);
	printf("  Token Ring POST offset: 0x%x\n", ipldir->tok_post_off);
	printf("  Token Ring POST size: 0x%x\n", ipldir->tok_post_size);
	printf("  Serial POST offset: 0x%x\n", ipldir->ser_post_off);
	printf("  Serial POST size: 0x%x\n", ipldir->ser_post_size);
	printf("  Parallel POST offset: 0x%x\n", ipldir->par_post_off);
	printf("  Parallel POST size: 0x%x\n", ipldir->par_post_size);
	printf("  RSC POST offset: 0x%x\n", ipldir->rsc_post_off);
	printf("  RSC POST size: 0x%x\n", ipldir->rsc_post_size);
	printf("  Legacy POST offset: 0x%x\n", ipldir->lega_post_off);
	printf("  Legacy POST size: 0x%x\n", ipldir->lega_post_size);
	printf("  Keyboard POST offset: 0x%x\n", ipldir->kbd_post_off);
	printf("  Keyboard POST size: 0x%x\n", ipldir->kbd_post_size);
	printf("  RAM POST offset: 0x%x\n", ipldir->ram_post_off);
	printf("  RAM POST size: 0x%x\n", ipldir->ram_post_size);
	printf("  SGA POST offset: 0x%x\n", ipldir->sga_post_off);
	printf("  SGA POST size: 0x%x\n", ipldir->sga_post_size);
	printf("  Family2 POST offset: 0x%x\n", ipldir->fm2_post_off);
	printf("  Family2 POST size: 0x%x\n", ipldir->fm2_post_size);
	printf("  Netboot result offset: 0x%x\n", ipldir->net_boot_result_off);
	printf("  Netboot result size: 0x%x\n", ipldir->net_boot_result_size);
	printf("  Core Sequence Controller result offset: 0x%x\n", ipldir->csc_result_off);
	printf("  Core Sequence Controller result size: 0x%x\n", ipldir->csc_result_size);
	printf("  Menu result offset: 0x%x\n", ipldir->menu_result_off);
	printf("  Menu result size: 0x%x\n", ipldir->menu_result_size);
	printf("  Console result offset: 0x%x\n", ipldir->cons_result_off);
	printf("  Console result size: 0x%x\n", ipldir->cons_result_size);
	printf("  Diag result offset: 0x%x\n", ipldir->diag_result_off);
	printf("  Diag result size: 0x%x\n", ipldir->diag_result_size);
	printf("  ROM scan offset: 0x%x\n", ipldir->rom_scan_off);
	printf("  ROM scan size: 0x%x\n", ipldir->rom_scan_size);
	printf("  SKY POST offset: 0x%x\n", ipldir->sky_post_off);
	printf("  SKY POST size: 0x%x\n", ipldir->sky_post_size);
	printf("  Global offset: 0x%x\n", ipldir->global_off);
	printf("  Global size: 0x%x\n", ipldir->global_size);
	printf("  Mouse offset: 0x%x\n", ipldir->mouse_off);
	printf("  Mouse size: 0x%x\n", ipldir->mouse_size);
	printf("  VRS offset: 0x%x\n", ipldir->vrs_off);
	printf("  VRS size: 0x%x\n", ipldir->vrs_size);
	printf("  Taur offset: 0x%x\n", ipldir->taur_post_off);
	printf("  Taur size: 0x%x\n", ipldir->taur_post_size);
	printf("  ENT offset: 0x%x\n", ipldir->ent_post_off);
	printf("  ENT size: 0x%x\n", ipldir->ent_post_size);
	printf("  VRS40 offset: 0x%x\n", ipldir->vrs40_off);
	printf("  VRS40 size: 0x%x\n", ipldir->vrs40_size);
	printf("  Sysinfo offset: 0x%x\n", ipldir->sysinfo_offset);
	printf("  Sysinfo size: 0x%x\n", ipldir->sysinfo_size);
	printf("  BUCinfo offset: 0x%x\n", ipldir->bucinfo_off);
	printf("  BUCinfo size: 0x%x\n", ipldir->bucinfo_size);
	printf("  Processor info offset: 0x%x\n", ipldir->procinfo_off);
	printf("  Processor info size: 0x%x\n", ipldir->procinfo_size);
	printf("  Family 2 IO info offset: 0x%x\n", ipldir->fm2_ioinfo_off);
	printf("  Family 2 IO info size: 0x%x\n", ipldir->fm2_ioinfo_size);
	printf("  Processor POST offset: 0x%x\n", ipldir->proc_post_off);
	printf("  Processor POST size: 0x%x\n", ipldir->proc_post_size);
	printf("  System VPD offset: 0x%x\n", ipldir->sysvpd_off);
	printf("  System VPD size: 0x%x\n", ipldir->sysvpd_size);
	printf("  Memory Data offset: 0x%x\n", ipldir->memdata_off);
	printf("  Memory Data size: 0x%x\n", ipldir->memdata_size);
	printf("  L2 data offset: 0x%x\n", ipldir->l2data_off);
	printf("  L2 data size: 0x%x\n", ipldir->l2data_size);
	printf("  FDDI POST offset: 0x%x\n", ipldir->fddi_post_off);
	printf("  FDDI POST size: 0x%x\n", ipldir->fddi_post_size);
	printf("  Golden VPD offset: 0x%x\n", ipldir->golden_vpd_off);
	printf("  Golden VPD size: 0x%x\n", ipldir->golden_vpd_size);
	printf("  NVRAM Cache offset: 0x%x\n", ipldir->nvram_cache_off);
	printf("  NVRAM Cache size: 0x%x\n", ipldir->nvram_cache_size);
	printf("  User struct offset: 0x%x\n", ipldir->user_struct_off);
	printf("  User struct size: 0x%x\n", ipldir->user_struct_size);
	printf("  Residual offset: 0x%x\n", ipldir->residual_off);
	printf("  Residual size: 0x%x\n", ipldir->residual_size);

	dump_sysinfo(iplcb_p + ipldir->sysinfo_offset);
	p = (u_char *)(iplcb_p + ipldir->procinfo_off);
	for (i=0; i < ipldir->procinfo_size; i+=4) {
		printf(" 0x%x\n", *p);
		p += i;
	}

}
