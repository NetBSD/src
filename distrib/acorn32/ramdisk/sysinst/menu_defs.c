#include "menu_defs.h"


#include <stdio.h>
#include <time.h>
#include <curses.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"


void menu_0_postact(void);
void menu_0_postact(void)
{	 toplevel(); 
}
int opt_act_0_0(menudesc *m);
int opt_act_0_0(menudesc *m)
{	 do_install(); 
	return 0;
}

int opt_act_0_1(menudesc *m);
int opt_act_0_1(menudesc *m)
{	 do_upgrade(); 
	return 0;
}

int opt_act_0_2(menudesc *m);
int opt_act_0_2(menudesc *m)
{	 do_reinstall_sets(); 
	return 0;
}

int opt_act_0_3(menudesc *m);
int opt_act_0_3(menudesc *m)
{	 run_prog(0, NULL, "/sbin/reboot"); 
	return 1;
}

int opt_act_1_0(menudesc *m);
int opt_act_1_0(menudesc *m)
{	 system("/bin/sh"); 
	return 0;
}

int opt_act_1_1(menudesc *m);
int opt_act_1_1(menudesc *m)
{	
			set_timezone();
		
	return 0;
}

int opt_act_1_2(menudesc *m);
int opt_act_1_2(menudesc *m)
{	
			extern int network_up;

			network_up = 0;
			config_network();
		
	return 0;
}

int opt_act_1_3(menudesc *m);
int opt_act_1_3(menudesc *m)
{	 do_logging(); 
	return 0;
}

int opt_act_1_4(menudesc *m);
int opt_act_1_4(menudesc *m)
{	 run_prog(0, NULL, "/sbin/halt"); 
	return 1;
}

int opt_act_2_0(menudesc *m);
int opt_act_2_0(menudesc *m)
{	yesno = 1;
	return 1;
}

int opt_act_2_1(menudesc *m);
int opt_act_2_1(menudesc *m)
{	yesno = 0;
	return 1;
}

int opt_act_3_0(menudesc *m);
int opt_act_3_0(menudesc *m)
{	yesno = 0;
	return 1;
}

int opt_act_3_1(menudesc *m);
int opt_act_3_1(menudesc *m)
{	yesno = 1;
	return 1;
}

int opt_act_5_0(menudesc *m);
int opt_act_5_0(menudesc *m)
{	 layoutkind = 1; md_set_no_x(); 
	return 1;
}

int opt_act_5_1(menudesc *m);
int opt_act_5_1(menudesc *m)
{	 layoutkind = 2; 
	return 1;
}

int opt_act_5_2(menudesc *m);
int opt_act_5_2(menudesc *m)
{	 layoutkind = 3; 
	return 1;
}

int opt_act_5_3(menudesc *m);
int opt_act_5_3(menudesc *m)
{	 layoutkind = 4; 
	return 1;
}

void menu_6_postact(void);
void menu_6_postact(void)
{	 show_cur_filesystems (); 
}
int opt_act_6_0(menudesc *m);
int opt_act_6_0(menudesc *m)
{	 layout_swap = (layout_swap & 1) - 1; 
	return 0;
}

int opt_act_6_1(menudesc *m);
int opt_act_6_1(menudesc *m)
{	 layout_usr = (layout_usr & 1) - 1; 
	return 0;
}

int opt_act_6_2(menudesc *m);
int opt_act_6_2(menudesc *m)
{	 layout_var = (layout_var & 1) - 1; 
	return 0;
}

int opt_act_6_3(menudesc *m);
int opt_act_6_3(menudesc *m)
{	 layout_home = (layout_home & 1) - 1; 
	return 0;
}

int opt_act_6_4(menudesc *m);
int opt_act_6_4(menudesc *m)
{	 layout_tmp = (layout_tmp & 1) - 1; 
	return 0;
}

int opt_act_7_0(menudesc *m);
int opt_act_7_0(menudesc *m)
{	 sizemult = MEG / sectorsize;
		  multname = msg_string(MSG_megname);
		
	return 1;
}

int opt_act_7_1(menudesc *m);
int opt_act_7_1(menudesc *m)
{	 sizemult = current_cylsize; 
		  multname = msg_string(MSG_cylname);
		
	return 1;
}

int opt_act_7_2(menudesc *m);
int opt_act_7_2(menudesc *m)
{	 sizemult = 1; 
		  multname = msg_string(MSG_secname);
		
	return 1;
}

void menu_8_postact(void);
void menu_8_postact(void)
{	
		msg_display(MSG_fspart, multname);
		disp_cur_fspart(-1, 0);
	
}
void menu_9_postact(void);
void menu_9_postact(void)
{	
			ask_sizemult(dlcylsize);
			msg_display(MSG_arm32fspart, disk->dd_name, multname);
			disp_cur_fspart(-1, 1);
		
}
int opt_act_9_0(menudesc *m);
int opt_act_9_0(menudesc *m)
{	 editpart = A;
	return 0;
}

int opt_act_9_1(menudesc *m);
int opt_act_9_1(menudesc *m)
{	 editpart = B;
	return 0;
}

int opt_act_9_3(menudesc *m);
int opt_act_9_3(menudesc *m)
{	 editpart = D;
	return 0;
}

int opt_act_9_4(menudesc *m);
int opt_act_9_4(menudesc *m)
{	 editpart = E;
	return 0;
}

int opt_act_9_5(menudesc *m);
int opt_act_9_5(menudesc *m)
{	 editpart = F;
	return 0;
}

int opt_act_9_6(menudesc *m);
int opt_act_9_6(menudesc *m)
{	 editpart = G;
	return 0;
}

int opt_act_9_7(menudesc *m);
int opt_act_9_7(menudesc *m)
{	 editpart = H;
	return 0;
}

int opt_act_9_8(menudesc *m);
int opt_act_9_8(menudesc *m)
{	 reask_sizemult(dlcylsize); 
	return 0;
}

void menu_10_postact(void);
void menu_10_postact(void)
{	
		msg_display (MSG_edfspart, 'a'+editpart);
		disp_cur_fspart(editpart, 1);
	
}
int opt_act_10_0(menudesc *m);
int opt_act_10_0(menudesc *m)
{	
			if (check_lfs_progs())
				process_menu (MENU_selfskindlfs);
			else
				process_menu (MENU_selfskind);
		
	return 0;
}

int opt_act_10_1(menudesc *m);
int opt_act_10_1(menudesc *m)
{		int start, size;
			msg_display_add(MSG_defaultunit, multname);
			start = getpartoff(MSG_offset, 0);
			size = getpartsize(MSG_size, start, 0);
			if (size == -1)
				size = dlsize - start;
			bsdlabel[editpart].pi_offset = start;
			bsdlabel[editpart].pi_size = size;
		
	return 0;
}

int opt_act_10_2(menudesc *m);
int opt_act_10_2(menudesc *m)
{		char buf[40]; int i;

			if (!PI_ISBSDFS(&bsdlabel[editpart])) {
				msg_display (MSG_not42bsd, 'a'+editpart);
				process_menu (MENU_ok);
				return FALSE;
			}
			msg_prompt_add (MSG_bsize, NULL, buf, 40);
			i = atoi(buf);
			bsdlabel[editpart].pi_bsize = i;
			msg_prompt_add (MSG_fsize, NULL, buf, 40);
			i = atoi(buf);
			bsdlabel[editpart].pi_fsize = i;
		
	return 0;
}

int opt_act_10_3(menudesc *m);
int opt_act_10_3(menudesc *m)
{		if (PI_ISBSDFS(&bsdlabel[editpart]) ||
			    bsdlabel[editpart].pi_fstype == FS_MSDOS)
				msg_prompt_add (MSG_mountpoint, NULL,
					fsmount[editpart], 20);
			else {
				msg_display (MSG_nomount, 'a'+editpart);
				process_menu (MENU_ok);
			}
		
	return 0;
}

int opt_act_10_4(menudesc *m);
int opt_act_10_4(menudesc *m)
{		preservemount[editpart] = 1 - preservemount[editpart];
		
	return 0;
}

int opt_act_11_0(menudesc *m);
int opt_act_11_0(menudesc *m)
{	 bsdlabel[editpart].pi_fstype = FS_BSDFFS;
			  bsdlabel[editpart].pi_bsize  = 8192;
			  bsdlabel[editpart].pi_fsize  = 1024;
			
	return 1;
}

int opt_act_11_1(menudesc *m);
int opt_act_11_1(menudesc *m)
{	 bsdlabel[editpart].pi_fstype = FS_UNUSED;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			
	return 1;
}

int opt_act_11_2(menudesc *m);
int opt_act_11_2(menudesc *m)
{	 bsdlabel[editpart].pi_fstype = FS_SWAP;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			
	return 1;
}

int opt_act_11_3(menudesc *m);
int opt_act_11_3(menudesc *m)
{	 bsdlabel[editpart].pi_fstype = FS_MSDOS;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			
	return 1;
}

int opt_act_12_0(menudesc *m);
int opt_act_12_0(menudesc *m)
{	 bsdlabel[editpart].pi_fstype = FS_BSDFFS;
			  bsdlabel[editpart].pi_bsize  = 8192;
			  bsdlabel[editpart].pi_fsize  = 1024;
			
	return 1;
}

int opt_act_12_1(menudesc *m);
int opt_act_12_1(menudesc *m)
{	 bsdlabel[editpart].pi_fstype = FS_UNUSED;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			
	return 1;
}

int opt_act_12_2(menudesc *m);
int opt_act_12_2(menudesc *m)
{	 bsdlabel[editpart].pi_fstype = FS_SWAP;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			
	return 1;
}

int opt_act_12_3(menudesc *m);
int opt_act_12_3(menudesc *m)
{	 bsdlabel[editpart].pi_fstype = FS_MSDOS;
			  bsdlabel[editpart].pi_bsize  = 0;
			  bsdlabel[editpart].pi_fsize  = 0;
			
	return 1;
}

int opt_act_12_4(menudesc *m);
int opt_act_12_4(menudesc *m)
{	 bsdlabel[editpart].pi_fstype = FS_BSDLFS;
			  bsdlabel[editpart].pi_bsize  = 8192;
			  bsdlabel[editpart].pi_fsize  = 1024;
			
	return 1;
}

void menu_13_postact(void);
void menu_13_postact(void)
{	 msg_display (MSG_distmedium); nodist = 0; 
}
int opt_act_13_0(menudesc *m);
int opt_act_13_0(menudesc *m)
{	
				  got_dist = get_via_ftp();
			        
	return 1;
}

int opt_act_13_1(menudesc *m);
int opt_act_13_1(menudesc *m)
{	 
				  got_dist = get_via_nfs();
			 	
	return 1;
}

int opt_act_13_2(menudesc *m);
int opt_act_13_2(menudesc *m)
{	
				  got_dist = get_via_cdrom();
				
	return 1;
}

int opt_act_13_3(menudesc *m);
int opt_act_13_3(menudesc *m)
{	
			          got_dist = get_via_floppy(); 
				
	return 1;
}

int opt_act_13_4(menudesc *m);
int opt_act_13_4(menudesc *m)
{	
				  got_dist = get_via_localfs(); 
				
	return 1;
}

int opt_act_13_5(menudesc *m);
int opt_act_13_5(menudesc *m)
{	
				   got_dist = get_via_localdir();
				 
	return 1;
}

int opt_act_13_6(menudesc *m);
int opt_act_13_6(menudesc *m)
{	 nodist = 1; 
	return 1;
}

void menu_14_postact(void);
void menu_14_postact(void)
{	 msg_display (MSG_distset); 
}
void menu_15_postact(void);
void menu_15_postact(void)
{	 show_cur_distsets (); 
}
int opt_act_15_0(menudesc *m);
int opt_act_15_0(menudesc *m)
{	 toggle_getit (0); 
	return 0;
}

int opt_act_15_1(menudesc *m);
int opt_act_15_1(menudesc *m)
{	 toggle_getit (1); 
	return 0;
}

int opt_act_15_2(menudesc *m);
int opt_act_15_2(menudesc *m)
{	 toggle_getit (2); 
	return 0;
}

int opt_act_15_3(menudesc *m);
int opt_act_15_3(menudesc *m)
{	 toggle_getit (3); 
	return 0;
}

int opt_act_15_4(menudesc *m);
int opt_act_15_4(menudesc *m)
{	 toggle_getit (4); 
	return 0;
}

int opt_act_15_5(menudesc *m);
int opt_act_15_5(menudesc *m)
{	 toggle_getit (5); 
	return 0;
}

int opt_act_15_6(menudesc *m);
int opt_act_15_6(menudesc *m)
{	 toggle_getit (6); 
	return 0;
}

int opt_act_15_7(menudesc *m);
int opt_act_15_7(menudesc *m)
{	 toggle_getit (7); 
	return 0;
}

int opt_act_15_8(menudesc *m);
int opt_act_15_8(menudesc *m)
{	 toggle_getit (8); 
	return 0;
}

int opt_act_15_9(menudesc *m);
int opt_act_15_9(menudesc *m)
{	 toggle_getit (9); 
	return 0;
}

int opt_act_15_10(menudesc *m);
int opt_act_15_10(menudesc *m)
{	 toggle_getit (10); 
	return 0;
}

int opt_act_15_11(menudesc *m);
int opt_act_15_11(menudesc *m)
{	 toggle_getit (11); 
	return 0;
}

int opt_act_15_12(menudesc *m);
int opt_act_15_12(menudesc *m)
{	 toggle_getit (12); 
	return 0;
}

int opt_act_15_13(menudesc *m);
int opt_act_15_13(menudesc *m)
{	 toggle_getit (13); 
	return 0;
}

void menu_16_postact(void);
void menu_16_postact(void)
{	 msg_clear();
		  msg_table_add (MSG_ftpsource, ftp_host, ftp_dir, ftp_user,
		     strcmp(ftp_user, "ftp") == 0 ? ftp_pass :
		       strlen(ftp_pass) != 0 ? "** hidden **" : "", ftp_proxy);
		
}
int opt_act_16_0(menudesc *m);
int opt_act_16_0(menudesc *m)
{	 msg_prompt (MSG_host, ftp_host, ftp_host, 255); 
	return 0;
}

int opt_act_16_1(menudesc *m);
int opt_act_16_1(menudesc *m)
{	 msg_prompt (MSG_dir, ftp_dir, ftp_dir, 255); 
	return 0;
}

int opt_act_16_2(menudesc *m);
int opt_act_16_2(menudesc *m)
{	 msg_prompt (MSG_user, ftp_user, ftp_user, 255); 
	return 0;
}

int opt_act_16_3(menudesc *m);
int opt_act_16_3(menudesc *m)
{	 if (strcmp(ftp_user, "ftp") == 0)
			msg_prompt (MSG_email, ftp_pass, ftp_pass, 255);
		  else {
			msg_prompt_noecho (MSG_passwd, "", ftp_pass, 255);
		  }
		
	return 0;
}

int opt_act_16_4(menudesc *m);
int opt_act_16_4(menudesc *m)
{	 msg_prompt (MSG_proxy, ftp_proxy, ftp_proxy, 255);
		  if (strcmp(ftp_proxy, "") == 0)
			unsetenv("ftp_proxy");
		  else
			setenv("ftp_proxy", ftp_proxy, 1);
		
	return 0;
}

void menu_17_postact(void);
void menu_17_postact(void)
{	 msg_display (MSG_nfssource, nfs_host, nfs_dir); 
}
int opt_act_17_0(menudesc *m);
int opt_act_17_0(menudesc *m)
{	 msg_prompt (MSG_host, NULL, nfs_host, 255); 
	return 0;
}

int opt_act_17_1(menudesc *m);
int opt_act_17_1(menudesc *m)
{	 msg_prompt (MSG_dir, NULL, nfs_dir, 255); 
	return 0;
}

int opt_act_18_0(menudesc *m);
int opt_act_18_0(menudesc *m)
{	 yesno = 1; ignorerror = 0; 
	return 1;
}

int opt_act_18_1(menudesc *m);
int opt_act_18_1(menudesc *m)
{	 yesno = 0; ignorerror = 0; 
	return 1;
}

int opt_act_18_2(menudesc *m);
int opt_act_18_2(menudesc *m)
{	 yesno = 1; ignorerror = 1; 
	return 1;
}

int opt_act_19_0(menudesc *m);
int opt_act_19_0(menudesc *m)
{	 yesno = 1; 
	return 1;
}

int opt_act_19_1(menudesc *m);
int opt_act_19_1(menudesc *m)
{	 yesno = 0; 
	return 1;
}

int opt_act_20_0(menudesc *m);
int opt_act_20_0(menudesc *m)
{	 yesno = 1; 
	return 1;
}

int opt_act_20_1(menudesc *m);
int opt_act_20_1(menudesc *m)
{	 yesno = 0; 
	return 1;
}

void menu_21_postact(void);
void menu_21_postact(void)
{	 msg_display (MSG_cdromsource, cdrom_dev, cdrom_dir); 
}
int opt_act_21_0(menudesc *m);
int opt_act_21_0(menudesc *m)
{	 msg_prompt (MSG_dev, cdrom_dev, cdrom_dev, SSTRSIZE); 
	return 0;
}

int opt_act_21_1(menudesc *m);
int opt_act_21_1(menudesc *m)
{	 msg_prompt (MSG_dir, cdrom_dir, cdrom_dir, STRSIZE); 
	return 0;
}

int opt_act_22_0(menudesc *m);
int opt_act_22_0(menudesc *m)
{	 yesno = 1; ignorerror = 0; 
	return 1;
}

int opt_act_22_1(menudesc *m);
int opt_act_22_1(menudesc *m)
{	 yesno = 0; ignorerror = 0; 
	return 1;
}

int opt_act_22_2(menudesc *m);
int opt_act_22_2(menudesc *m)
{	 yesno = 1; ignorerror = 1; 
	return 1;
}

void menu_23_postact(void);
void menu_23_postact(void)
{	 msg_display (MSG_localfssource, localfs_dev, localfs_fs, localfs_dir); 
}
int opt_act_23_0(menudesc *m);
int opt_act_23_0(menudesc *m)
{	 msg_prompt (MSG_dev, localfs_dev, localfs_dev, SSTRSIZE); 
	return 0;
}

int opt_act_23_1(menudesc *m);
int opt_act_23_1(menudesc *m)
{	 msg_prompt (MSG_filesys, localfs_fs, localfs_fs, STRSIZE); 
	return 0;
}

int opt_act_23_2(menudesc *m);
int opt_act_23_2(menudesc *m)
{	 msg_prompt (MSG_dir, localfs_dir, localfs_dir, STRSIZE); 
	return 0;
}

int opt_act_24_0(menudesc *m);
int opt_act_24_0(menudesc *m)
{	 yesno = 1; ignorerror = 0; 
	return 1;
}

int opt_act_24_1(menudesc *m);
int opt_act_24_1(menudesc *m)
{	 yesno = 0; ignorerror = 0; 
	return 1;
}

int opt_act_24_2(menudesc *m);
int opt_act_24_2(menudesc *m)
{	 yesno = 1; ignorerror = 1; 
	return 1;
}

void menu_25_postact(void);
void menu_25_postact(void)
{	 msg_display(MSG_localdir, localfs_dir); 
}
int opt_act_25_0(menudesc *m);
int opt_act_25_0(menudesc *m)
{	 msg_prompt (MSG_dir, localfs_dir, localfs_dir, STRSIZE); 
	return 1;
}

int opt_act_26_0(menudesc *m);
int opt_act_26_0(menudesc *m)
{	 yesno = 1;
	          msg_prompt(MSG_localdir, localfs_dir, localfs_dir, STRSIZE);
		
	return 1;
}

int opt_act_26_1(menudesc *m);
int opt_act_26_1(menudesc *m)
{	 yesno = 0; ignorerror = 0; 
	return 1;
}

int opt_act_26_2(menudesc *m);
int opt_act_26_2(menudesc *m)
{	 yesno = 1; ignorerror = 1; 
	return 1;
}

int opt_act_27_0(menudesc *m);
int opt_act_27_0(menudesc *m)
{	
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:501:4819::42",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		
	return 1;
}

int opt_act_27_1(menudesc *m);
int opt_act_27_1(menudesc *m)
{	
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:501:410:100:5254:ff:feda:48bf",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		
	return 1;
}

int opt_act_27_2(menudesc *m);
int opt_act_27_2(menudesc *m)
{	
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:507:0:1:260:97ff:fe07:69ea",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		
	return 1;
}

int opt_act_27_3(menudesc *m);
int opt_act_27_3(menudesc *m)
{	
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:508:0:1::53",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		
	return 1;
}

int opt_act_27_4(menudesc *m);
int opt_act_27_4(menudesc *m)
{	
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:1800:1000::1",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		
	return 1;
}

int opt_act_27_5(menudesc *m);
int opt_act_27_5(menudesc *m)
{	
#ifdef INET6
		  strncpy(net_namesvr6, "3ffe:505:0:1:2a0:c9ff:fe61:6521",
		      sizeof(net_namesvr6));
		  yesno = 1;
#else
		  yesno = 0;
#endif
		
	return 1;
}

int opt_act_27_6(menudesc *m);
int opt_act_27_6(menudesc *m)
{	 yesno = 0; 
	return 1;
}

int opt_act_28_0(menudesc *m);
int opt_act_28_0(menudesc *m)
{	yesno = 1;
	return 1;
}

int opt_act_28_1(menudesc *m);
int opt_act_28_1(menudesc *m)
{	yesno = 0;
	return 1;
}

int opt_act_29_0(menudesc *m);
int opt_act_29_0(menudesc *m)
{	yesno = 1;
	return 1;
}

int opt_act_29_1(menudesc *m);
int opt_act_29_1(menudesc *m)
{	yesno = 0;
	return 1;
}

static menu_ent optent0[] = {
	{"Install NetBSD to hard disk",-1,0,opt_act_0_0},
	{"Upgrade NetBSD on a hard disk",-1,0,opt_act_0_1},
	{"Re-install sets or install additional sets",-1,0,opt_act_0_2},
	{"Reboot the computer",-1,6,opt_act_0_3},
	{"Utility menu",1,1,NULL}
	};

static menu_ent optent1[] = {
	{"Run /bin/sh",-1,2,opt_act_1_0},
	{"Set timezone",-1,0,opt_act_1_1},
	{"Configure network",-1,0,opt_act_1_2},
	{"Logging functions",-1,0,opt_act_1_3},
	{"Halt the system",-1,4,opt_act_1_4}
	};

static menu_ent optent2[] = {
	{"Yes",-1,4,opt_act_2_0},
	{"No",-1,4,opt_act_2_1}
	};

static menu_ent optent3[] = {
	{"No",-1,4,opt_act_3_0},
	{"Yes",-1,4,opt_act_3_1}
	};

static menu_ent optent4[] = {
	{"ok",-1,4,NULL}
	};

static menu_ent optent5[] = {
	{"Standard",-1,4,opt_act_5_0},
	{"Standard with X",-1,4,opt_act_5_1},
	{"Custom",-1,4,opt_act_5_2},
	{"Use Existing",-1,4,opt_act_5_3}
	};

static menu_ent optent6[] = {
	{"swap",-1,0,opt_act_6_0},
	{"/usr",-1,0,opt_act_6_1},
	{"/var",-1,0,opt_act_6_2},
	{"/home",-1,0,opt_act_6_3},
	{"/tmp (mfs)",-1,0,opt_act_6_4}
	};

static menu_ent optent7[] = {
	{"Megabytes",-1,4,opt_act_7_0},
	{"Cylinders",-1,4,opt_act_7_1},
	{"Sectors",-1,4,opt_act_7_2}
	};

static menu_ent optent8[] = {
	{"Change partitions",9,1,NULL},
	{"Partitions are ok",-1,4,NULL}
	};

static menu_ent optent9[] = {
	{"Change a",10,1,opt_act_9_0},
	{"Change b",10,1,opt_act_9_1},
	{"Whole disk - can't change",-1,0,NULL},
	{"Change d",10,1,opt_act_9_3},
	{"Change e",10,1,opt_act_9_4},
	{"Change f",10,1,opt_act_9_5},
	{"Change g",10,1,opt_act_9_6},
	{"Change h",10,1,opt_act_9_7},
	{"Set new allocation size",-1,0,opt_act_9_8}
	};

static menu_ent optent10[] = {
	{"FS kind",-1,0,opt_act_10_0},
	{"Offset/size",-1,0,opt_act_10_1},
	{"Bsize/Fsize",-1,0,opt_act_10_2},
	{"Mount point",-1,0,opt_act_10_3},
	{"Preserve",-1,0,opt_act_10_4}
	};

static menu_ent optent11[] = {
	{"4.2BSD",-1,4,opt_act_11_0},
	{"unused",-1,4,opt_act_11_1},
	{"swap",-1,4,opt_act_11_2},
	{"msdos",-1,4,opt_act_11_3}
	};

static menu_ent optent12[] = {
	{"4.2BSD",-1,4,opt_act_12_0},
	{"unused",-1,4,opt_act_12_1},
	{"swap",-1,4,opt_act_12_2},
	{"msdos",-1,4,opt_act_12_3},
	{"4.4LFS",-1,4,opt_act_12_4}
	};

static menu_ent optent13[] = {
	{"ftp",-1,4,opt_act_13_0},
	{"nfs",-1,4,opt_act_13_1},
	{"cdrom",-1,4,opt_act_13_2},
	{"floppy",-1,4,opt_act_13_3},
	{"unmounted fs",-1,4,opt_act_13_4},
	{"local dir",-1,4,opt_act_13_5},
	{"none",-1,4,opt_act_13_6}
	};

static menu_ent optent14[] = {
	{"Full installation",-1,4,NULL},
	{"Custom installation",15,0,NULL}
	};

static menu_ent optent15[] = {
	{"Kernel (SHARK)",-1,0,opt_act_15_0},
	{"Base",-1,0,opt_act_15_1},
	{"System (/etc)",-1,0,opt_act_15_2},
	{"Compiler Tools",-1,0,opt_act_15_3},
	{"Games",-1,0,opt_act_15_4},
	{"Online Manual Pages",-1,0,opt_act_15_5},
	{"Miscellaneous",-1,0,opt_act_15_6},
	{"Text Processing Tools",-1,0,opt_act_15_7},
	{"X11 base and clients",-1,0,opt_act_15_8},
	{"X11 fonts",-1,0,opt_act_15_9},
	{"X11 servers",-1,0,opt_act_15_10},
	{"X contrib clients",-1,0,opt_act_15_11},
	{"X11 programming",-1,0,opt_act_15_12},
	{"X11 Misc.",-1,0,opt_act_15_13}
	};

static menu_ent optent16[] = {
	{"Host",-1,0,opt_act_16_0},
	{"Directory",-1,0,opt_act_16_1},
	{"User",-1,0,opt_act_16_2},
	{"Password",-1,0,opt_act_16_3},
	{"Proxy",-1,0,opt_act_16_4},
	{"Get Distribution",-1,4,NULL}
	};

static menu_ent optent17[] = {
	{"Host",-1,0,opt_act_17_0},
	{"Directory",-1,0,opt_act_17_1},
	{"Continue",-1,4,NULL}
	};

static menu_ent optent18[] = {
	{"Try again",17,5,opt_act_18_0},
	{"Give up",-1,4,opt_act_18_1},
	{"Ignore, continue anyway",-1,4,opt_act_18_2}
	};

static menu_ent optent19[] = {
	{"Try again",-1,4,opt_act_19_0},
	{"Abort install",-1,4,opt_act_19_1}
	};

static menu_ent optent20[] = {
	{"OK",-1,4,opt_act_20_0},
	{"Abort install",-1,4,opt_act_20_1}
	};

static menu_ent optent21[] = {
	{"Device",-1,0,opt_act_21_0},
	{"Directory",-1,0,opt_act_21_1},
	{"Continue",-1,4,NULL}
	};

static menu_ent optent22[] = {
	{"Try again",21,5,opt_act_22_0},
	{"Give up",-1,4,opt_act_22_1},
	{"Ignore, continue anyway",-1,4,opt_act_22_2}
	};

static menu_ent optent23[] = {
	{"Device",-1,0,opt_act_23_0},
	{"Filesystem",-1,0,opt_act_23_1},
	{"Directory",-1,0,opt_act_23_2},
	{"Continue",-1,4,NULL}
	};

static menu_ent optent24[] = {
	{"Try again",23,5,opt_act_24_0},
	{"Give up",-1,4,opt_act_24_1},
	{"Ignore, continue anyway",-1,4,opt_act_24_2}
	};

static menu_ent optent25[] = {
	{"Directory",-1,4,opt_act_25_0},
	{"Continue",-1,4,NULL}
	};

static menu_ent optent26[] = {
	{"Change directory path",-1,4,opt_act_26_0},
	{"Give up",-1,4,opt_act_26_1},
	{"Ignore, continue anyway",-1,4,opt_act_26_2}
	};

static menu_ent optent27[] = {
	{"paradise.v6.kame.net",-1,4,opt_act_27_0},
	{"kiwi.itojun.org",-1,4,opt_act_27_1},
	{"sh1.iijlab.net",-1,4,opt_act_27_2},
	{"ns1.v6.intec.co.jp",-1,4,opt_act_27_3},
	{"nttv6.net",-1,4,opt_act_27_4},
	{"light.imasy.or.jp",-1,4,opt_act_27_5},
	{"other ",-1,4,opt_act_27_6}
	};

static menu_ent optent28[] = {
	{"Yes",-1,4,opt_act_28_0},
	{"No",-1,4,opt_act_28_1}
	};

static menu_ent optent29[] = {
	{"Yes",-1,4,opt_act_29_0},
	{"No",-1,4,opt_act_29_1}
	};

static struct menudesc menu_def[] = {
	{"NetBSD-1.5ZA Install System",12,-1,0,0,4,5,0,0,optent0,NULL,NULL,"Exit Install System",menu_0_postact,NULL},
	{"NetBSD-1.5ZA Utilities",12,-1,0,0,4,5,0,0,optent1,NULL,NULL,"Exit",NULL,NULL},
	{"yes or no?",12,-1,0,0,5,2,0,0,optent2,NULL,NULL,NULL,NULL,NULL},
	{"yes or no?",12,-1,0,0,5,2,0,0,optent3,NULL,NULL,NULL,NULL,NULL},
	{"Hit enter to continue",12,-1,0,0,5,1,0,0,optent4,NULL,NULL,NULL,NULL,NULL},
	{"Choose your installation",12,-1,0,0,5,4,0,0,optent5,NULL,NULL,NULL,NULL,NULL},
	{"Choose filesystems",12,-1,0,0,4,5,0,0,optent6,NULL,NULL,"Exit",menu_6_postact,NULL},
	{"Choose your size specifier",12,-1,0,0,5,3,0,0,optent7,NULL,NULL,NULL,NULL,NULL},
	{"Partitions ok?",15,-1,0,0,5,2,0,0,optent8,NULL,NULL,NULL,menu_8_postact,NULL},
	{"",12,-1,0,0,4,9,0,0,optent9,NULL,NULL,"Exit",menu_9_postact,NULL},
	{"Change what?",14,-1,0,0,4,5,0,0,optent10,NULL,NULL,"Exit",menu_10_postact,NULL},
	{"Select the type",15,-1,0,0,5,4,0,0,optent11,NULL,NULL,NULL,NULL,NULL},
	{"Select the type",15,-1,0,0,5,5,0,0,optent12,NULL,NULL,NULL,NULL,NULL},
	{"Select medium",12,-1,0,0,5,7,0,0,optent13,NULL,NULL,NULL,menu_13_postact,NULL},
	{"Select your distribution",12,-1,0,0,5,2,0,0,optent14,NULL,NULL,NULL,menu_14_postact,NULL},
	{"Selection toggles inclusion",5,26,0,0,4,14,0,0,optent15,NULL,NULL,"Exit",menu_15_postact,NULL},
	{"Change",12,-1,0,0,5,6,0,0,optent16,NULL,NULL,NULL,menu_16_postact,NULL},
	{"Change",12,-1,0,0,5,3,0,0,optent17,NULL,NULL,NULL,menu_17_postact,NULL},
	{"What do you want to do?",12,-1,0,0,5,3,0,0,optent18,NULL,NULL,NULL,NULL,NULL},
	{"What do you want to do?",12,-1,0,0,5,2,0,0,optent19,NULL,NULL,NULL,NULL,NULL},
	{"Hit enter to continue",12,-1,0,0,5,2,0,0,optent20,NULL,NULL,NULL,NULL,NULL},
	{"Change",12,-1,0,0,5,3,0,0,optent21,NULL,NULL,NULL,menu_21_postact,NULL},
	{"What do you want to do?",12,-1,0,0,5,3,0,0,optent22,NULL,NULL,NULL,NULL,NULL},
	{"Change",12,-1,0,0,5,4,0,0,optent23,NULL,NULL,NULL,menu_23_postact,NULL},
	{"What do you want to do?",12,-1,0,0,5,3,0,0,optent24,NULL,NULL,NULL,NULL,NULL},
	{"Change",12,-1,0,0,5,2,0,0,optent25,NULL,NULL,NULL,menu_25_postact,NULL},
	{"What do you want to do?",12,-1,0,0,5,3,0,0,optent26,NULL,NULL,NULL,NULL,NULL},
	{"  Select IPv6 DNS server",12,-1,0,0,5,7,0,0,optent27,NULL,NULL,NULL,NULL,NULL},
	{"Perform IPv6 autoconfiguration?",12,-1,0,0,5,2,0,0,optent28,NULL,NULL,NULL,NULL,NULL},
	{"Perform DHCP autoconfiguration?",12,-1,0,0,5,2,0,0,optent29,NULL,NULL,NULL,NULL,NULL},
{NULL}};

/* __menu_initerror: initscr failed. */
void __menu_initerror (void) {
	(void) fprintf (stderr, "%s: Could not initialize curses\n", getprogname());
	exit(1);
}
/*	$NetBSD: menu_defs.c,v 1.1 2002/01/25 15:35:53 reinoud Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software develooped for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* menu_sys.defs -- Menu system standard routines. */

#include <string.h>
#include <ctype.h>

#define REQ_EXECUTE    1000
#define REQ_NEXT_ITEM  1001
#define REQ_PREV_ITEM  1002
#define REQ_REDISPLAY  1003
#define REQ_SCROLLDOWN 1004
#define REQ_SCROLLUP   1005
#define REQ_HELP       1006

/* Multiple key support */
#define KEYSEQ_FIRST       256
#define KEYSEQ_DOWN_ARROW  256
#define KEYSEQ_UP_ARROW    257
#define KEYSEQ_LEFT_ARROW  258
#define KEYSEQ_RIGHT_ARROW 259
#define KEYSEQ_PAGE_DOWN   260
#define KEYSEQ_PAGE_UP     261

struct keyseq {
	char *termcap_name;
	char *chars;
	int  numchars;
	int  keyseq_val;
	struct keyseq *next;
};

/* keypad and other definitions */
struct keyseq _mc_key_seq[] = {
	/* Cludge for xterm ... */
	{  NULL, "\033[B", 0, KEYSEQ_DOWN_ARROW, NULL },
	{  NULL, "\033[D", 0, KEYSEQ_LEFT_ARROW, NULL },
	{  NULL, "\033[C", 0, KEYSEQ_RIGHT_ARROW, NULL },
	{  NULL, "\033[A", 0, KEYSEQ_UP_ARROW, NULL },
	/* Termcap defined */
	{ "kd", NULL, 0, KEYSEQ_DOWN_ARROW, NULL },
	{ "kl", NULL, 0, KEYSEQ_LEFT_ARROW, NULL },
	{ "kr", NULL, 0, KEYSEQ_RIGHT_ARROW, NULL },
	{ "ku", NULL, 0, KEYSEQ_UP_ARROW, NULL },
	{ "kf", NULL, 0, KEYSEQ_PAGE_DOWN, NULL },  /* scroll forward */
	{ "kN", NULL, 0, KEYSEQ_PAGE_DOWN, NULL },  /* next page */
	{ "kP", NULL, 0, KEYSEQ_PAGE_UP, NULL },    /* scroll backward */
	{ "kR", NULL, 0, KEYSEQ_PAGE_UP, NULL },    /* prev page */
	/* other definitions */
	{ NULL, "\033v", 0, KEYSEQ_PAGE_UP, NULL },   /* ESC-v */
	{ NULL, "\026", 0, KEYSEQ_PAGE_DOWN, NULL },  /* CTL-v */
};

int _mc_num_key_seq = sizeof(_mc_key_seq) / sizeof(struct keyseq);
struct keyseq *pad_list = NULL;
static char str_area [512];
static size_t str_size = sizeof str_area;
static char *str_ptr = str_area;

/* Macros */
#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

/* Initialization state. */
static int __menu_init = 0;
int __m_endwin = 0;
static int max_lines = 0, max_cols = 0;
static char *scrolltext = " <: page up, >: page down";

static menudesc *menus = menu_def;

#ifdef DYNAMIC_MENUS
static int num_menus  = 0;
static int num_avail  = 0;
#define DYN_INIT_NUM 32
#endif

/* prototypes for in here! */
static void ins_keyseq (struct keyseq **seq, struct keyseq *ins);
static void init_keyseq (void);
static void init_menu (struct menudesc *m);
static char opt_ch (int op_no);
static void post_menu (struct menudesc *m);
static void process_help (struct menudesc *m, int num);
static void process_req (struct menudesc *m, int num, int req);
static int menucmd (WINDOW *w);

#ifndef NULL
#define NULL (void *)0
#endif

/* menu system processing routines */
#define mbeep() (void)fputc('\a', stderr)

static void
ins_keyseq (struct keyseq **seq, struct keyseq *ins)
{
	if (*seq == NULL) {
		ins->next = NULL;
		*seq = ins;
	} else if (ins->numchars <= (*seq)->numchars) {
		ins->next = *seq;
		*seq = ins;
	} else
		ins_keyseq (&(*seq)->next, ins);
}

static void
init_keyseq(void)
{
	/*
	 * XXX XXX XXX THIS SHOULD BE NUKED FROM ORBIT!  DO THIS
	 * XXX XXX XXX WITH NORMAL CURSES FACILITIES!
	 */
	struct tinfo *ti;
	char *term = getenv("TERM");
	int i;

	if (term == NULL || term[0] == '\0')
		return;

	if (t_getent(&ti, term) < 0)
		return;

	for (i = 0; i < _mc_num_key_seq; i++) {
		if (_mc_key_seq[i].termcap_name == NULL)
			continue;

		_mc_key_seq[i].chars = t_getstr(ti, _mc_key_seq[i].termcap_name,
		     &str_ptr, &str_size);

		if (_mc_key_seq[i].chars != NULL &&
		    (_mc_key_seq[i].numchars = strlen(_mc_key_seq[i].chars))
		    > 0)
			ins_keyseq(&pad_list, &_mc_key_seq[i]);
	}
	t_freent(ti);
}

static int
mgetch(WINDOW *w)
{
	static char buf[20];
	static int  num = 0;
	struct keyseq *list = pad_list;
	int i, ret;

	/* key pad processing */
	while (list) {
		for (i=0; i< list->numchars; i++) {
			if (i >= num)
				buf[num++] = wgetch(w);
			if (buf[i] != list->chars[i])
				break;
		}
		if (i == list->numchars) {
			num = 0;
			return list->keyseq_val;
		}
		list = list->next;
	}

	ret = buf[0];
	for (i = 0; i < strlen(buf); i++)
		buf[i] = buf[i+1];
	num--;
	return ret;
}

static int
menucmd (WINDOW *w)
{
	int ch;

	while (TRUE) {
		ch = mgetch(w);
		
		switch (ch) {
		case '\n':
			return REQ_EXECUTE;
		case '\016':  /* Contnrol-P */
		case KEYSEQ_DOWN_ARROW:
			return REQ_NEXT_ITEM;
		case '\020':  /* Control-N */
		case KEYSEQ_UP_ARROW:
			return REQ_PREV_ITEM;
		case '\014':  /* Control-L */
		        return REQ_REDISPLAY;
		case '<':
		case '\010':  /* Control-H (backspace) */
		case KEYSEQ_PAGE_UP:
			return REQ_SCROLLUP;
		case '>':
		case ' ':
		case KEYSEQ_PAGE_DOWN:
			return REQ_SCROLLDOWN;
		case '?':
			return REQ_HELP;
		}
		
		if (isalpha(ch)) 
			return (ch);

		mbeep();
		wrefresh(w);
	}
}

static void
init_menu (struct menudesc *m)
{
	int wmax;
	int hadd, wadd, exithadd;
	int i;

	hadd = ((m->mopt & MC_NOBOX) ? 0 : 2);
	wadd = ((m->mopt & MC_NOBOX) ? 2 : 4);

	hadd += strlen(m->title) != 0 ? 2 : 0;
	exithadd = ((m->mopt & MC_NOEXITOPT) ? 0 : 1);

	wmax = strlen(m->title);

	/* Calculate h? h == number of visible options. */
	if (m->h == 0) {
		m->h = m->numopts + exithadd;
		if (m->h + m->y + hadd >= max_lines && (m->mopt & MC_SCROLL))
			m->h = max_lines - m->y - hadd ;
	}

	/* Check window heights and set scrolling */
	if (m->h < m->numopts + exithadd) {
		if (!(m->mopt & MC_SCROLL) || m->h < 3) {
			endwin();
			(void) fprintf (stderr,
				"Window too short for menu \"%s\"\n",
				m->title);
			exit(1);
		}
	} else
		m->mopt &= ~MC_SCROLL;

	/* check for screen fit */
	if (m->y + m->h + hadd > max_lines) {
		endwin();
		(void) fprintf (stderr,
			"Screen too short for menu \"%s\"\n", m->title);
		exit(1);

	}

	/* Calculate w? */
	if (m->w == 0) {
		if (m->mopt & MC_SCROLL)
			wmax = MAX(wmax,strlen(scrolltext));
		for (i=0; i < m->numopts; i++ )
			wmax = MAX(wmax,strlen(m->opts[i].opt_name)+3);
		m->w = wmax;
	}

	/* check and adjust for screen fit */
	if (m->w + wadd > max_cols) {
		endwin();
		(void) fprintf (stderr,
			"Screen too narrow for menu \"%s\"\n", m->title);
		exit(1);

	}
	if (m->x == -1)
		m->x = (max_cols - (m->w + wadd)) / 2;	/* center */
	else if (m->x + m->w + wadd > max_cols)
		m->x = max_cols - (m->w + wadd);

	/* Get the windows. */
	m->mw = newwin(m->h+hadd, m->w+wadd, m->y, m->x);

	if (m->mw == NULL) {
		endwin();
		(void) fprintf (stderr,
			"Could not create window for menu \"%s\"\n", m->title);
		exit(1);
	}

	/* XXX is it even worth doing this right? */
	if (has_colors()) {
		wbkgd(m->mw, COLOR_PAIR(1));
		wattrset(m->mw, COLOR_PAIR(1));
	}
}

static char
opt_ch (int op_no)
{
	char c;
	if (op_no < 25) {
		c = 'a' + op_no;
		if (c >= 'x') c++;
	} else 
		c = 'A' + op_no - 25;
	return (char) c;
}

static void
post_menu (struct menudesc *m)
{
	int i;
	int hasbox, cury, maxy, selrow, lastopt;
	int tadd;
	char optstr[5];
	
	if (m->mopt & MC_NOBOX) {
		cury = 0;
		maxy = m->h;
		hasbox = 0;
	} else {
		cury = 1;
		maxy = m->h+1;
		hasbox = 1;
	}

	/* Clear the window */
	wclear (m->mw);

	tadd = strlen(m->title) ? 2 : 0;

	if (tadd) {
		mvwaddstr(m->mw, cury, cury, " ");
		mvwaddstr(m->mw, cury, cury + 1, m->title);
		cury += 2;
		maxy += 2;
	}

	/* Set defaults, calculate lastopt. */
	selrow = -1;
	if (m->mopt & MC_SCROLL) {
		lastopt = MIN(m->numopts, m->topline+m->h-1);
		maxy -= 1;
	} else
		lastopt = m->numopts;

	for (i=m->topline; i<lastopt; i++, cury++) {
		if (m->cursel == i) {
			mvwaddstr (m->mw, cury, hasbox, ">");
			wstandout(m->mw);
			selrow = cury;
		} else
			mvwaddstr (m->mw, cury, hasbox, " ");
		if (!(m->mopt & MC_NOSHORTCUT)) {
			(void) sprintf (optstr, "%c: ", opt_ch(i));
		    	waddstr (m->mw, optstr);
		}
		waddstr (m->mw, m->opts[i].opt_name);
		if (m->cursel == i)
			wstandend(m->mw);
	}

	/* Add the exit option. */
	if (!(m->mopt & MC_NOEXITOPT) && cury < maxy) {
		if (m->cursel >= m->numopts) {
			mvwaddstr (m->mw, cury, hasbox, ">");
			wstandout(m->mw);
			selrow = cury;
		} else
			mvwaddstr (m->mw, cury, hasbox, " ");
		if (!(m->mopt & MC_NOSHORTCUT))
			waddstr (m->mw, "x: ");
		waddstr (m->mw, m->exitstr);
		if (m->cursel >= m->numopts)
			wstandend(m->mw);
		cury++;
	}

	/* Add the scroll line */
	if (m->mopt & MC_SCROLL) {
		mvwaddstr (m->mw, cury, hasbox, scrolltext);
		if (selrow < 0)
			selrow = cury;
	}

	/* Add the box. */
	if (!(m->mopt & MC_NOBOX))
		box(m->mw, 0, 0);

	wmove(m->mw, selrow, hasbox);
}

static void
process_help (struct menudesc *m, int num)
{
	char *help = m->helpstr;
	int lineoff = 0;
	int curoff = 0;
	int again;
	int winin;

	/* Is there help? */
	if (!help) {
		mbeep();
		return;
	}

	/* Display the help information. */
	do {
		if (lineoff < curoff) {
			help = m->helpstr;
			curoff = 0;
		}
		while (*help && curoff < lineoff) {
			if (*help == '\n')
				curoff++;
			help++;
		}
	
		wclear(stdscr);
		mvwaddstr (stdscr, 0, 0, 
			"Help: exit: x,  page up: u <, page down: d >");
		mvwaddstr (stdscr, 2, 0, help);
		wmove (stdscr, 1, 0);
	  	wrefresh(stdscr);

		do {
			winin = mgetch(stdscr);
			if (winin < KEYSEQ_FIRST)
				winin = tolower(winin);
			again = 0;
			switch (winin) {
				case '<':
				case 'u':
				case KEYSEQ_UP_ARROW:
				case KEYSEQ_LEFT_ARROW:
				case KEYSEQ_PAGE_UP:
					if (lineoff)
						lineoff -= max_lines - 2;
					else
						again = 1;
					break;
				case '>':
				case 'd':
				case KEYSEQ_DOWN_ARROW:
				case KEYSEQ_RIGHT_ARROW:
				case KEYSEQ_PAGE_DOWN:
					if (*help)
						lineoff += max_lines - 2;
					else
						again = 1;
					break;
				case 'q':
					break;
				case 'x':
					winin = 'q';
					break;
				default:
					again = 1;
			}
			if (again)
				mbeep();
		} while (again);
	} while (winin != 'q');

	/* Restore current menu */    
	wclear(stdscr);
	wrefresh(stdscr);
	if (m->post_act)
		(*m->post_act)();
}

static void
process_req (struct menudesc *m, int num, int req)
{
	int ch;
	int hasexit = (m->mopt & MC_NOEXITOPT ? 0 : 1 );
	int refresh = 0;
	int scroll_sel = 0;

	if (req == REQ_EXECUTE)
		return;

	else if (req == REQ_NEXT_ITEM) {
		if (m->cursel < m->numopts + hasexit - 1) {
			m->cursel++;
			scroll_sel = 1;
			refresh = 1;
			if (m->mopt & MC_SCROLL && 
			    m->cursel >= m->topline + m->h -1 )
				m->topline += 1;
		} else
			mbeep();

	} else if (req == REQ_PREV_ITEM) {
		if (m->cursel > 0) {
			m->cursel--;
			scroll_sel = 1;
			refresh = 1;
			if (m->cursel < m->topline )
				m->topline -= 1;
		} else
			mbeep();

	} else if (req == REQ_REDISPLAY) {
		wclear(stdscr);
		wrefresh(stdscr);
		if (m->post_act)
			(*m->post_act)();
		refresh = 1;

	} else if (req == REQ_HELP) {
		process_help (m, num);
		refresh = 1;

	} else if (req == REQ_SCROLLUP) {
		if (!(m->mopt & MC_SCROLL))
			mbeep();
		else if (m->topline == 0)
			mbeep();
		else {
			m->topline = MAX(0,m->topline-m->h+1);
			m->cursel = MAX(0, m->cursel-m->h+1);
			wclear (m->mw);
			refresh = 1;
		}

	} else if (req == REQ_SCROLLDOWN) {
		if (!(m->mopt & MC_SCROLL))
			mbeep();
		else if (m->topline + m->h - 1 >= m->numopts + hasexit)
			mbeep();
		else {
			m->topline = MIN(m->topline+m->h-1,
					 m->numopts+hasexit-m->h+1);
			m->cursel = MIN(m->numopts-1, m->cursel+m->h-1);
			wclear (m->mw);
			refresh = 1;
		}

	} else {
		ch = req;
		if (ch == 'x' && hasexit) {
			m->cursel = m->numopts;
			scroll_sel = 1;
			refresh = 1;
		} else 
			if (!(m->mopt & MC_NOSHORTCUT)) {
				if (ch > 'z')
					ch = 255;
				if (ch >= 'a') {
					if (ch > 'x') ch--;
				   		ch = ch - 'a';
				} else
					ch = 25 + ch - 'A';
				if (ch < 0 || ch >= m->numopts)
					mbeep();
				else {
					m->cursel = ch;
					scroll_sel = 1;
					refresh = 1;
				}
			} else 
				mbeep();
	}

	if (m->mopt & MC_SCROLL && scroll_sel) {
		while (m->cursel >= m->topline + m->h -1 )
			m->topline = MIN(m->topline+m->h-1,
					 m->numopts+hasexit-m->h+1);
		while (m->cursel < m->topline)
			m->topline = MAX(0,m->topline-m->h+1);
	}

	if (refresh) {
		post_menu (m);
		wrefresh (m->mw);
	}
}

int
menu_init (void)
{

	if (__menu_init)
		return 0;

	if (initscr() == NULL)
		return 1;

	cbreak();
	noecho();

	/* XXX Should be configurable but it almost isn't worth it. */
	if (has_colors()) {
		start_color();
		init_pair(1, COLOR_WHITE, COLOR_BLUE);
		bkgd(COLOR_PAIR(1));
		attrset(COLOR_PAIR(1));
	}

	max_lines = getmaxy(stdscr);
	max_cols = getmaxx(stdscr);
	init_keyseq();
#ifdef DYNAMIC_MENUS
	num_menus = DYN_INIT_NUM;
	while (num_menus < DYN_MENU_START)
		num_menus *= 2;
	menus = (menudesc *) malloc(sizeof(menudesc)*num_menus);
	if (menus == NULL)
		return 2;
	(void) memset ((void *)menus, 0, sizeof(menudesc)*num_menus);
	(void) memcpy ((void *)menus, (void *)menu_def,
		sizeof(menudesc)*DYN_MENU_START);
	 num_avail = num_menus - DYN_MENU_START;
#endif

	__menu_init = 1;
	return (0);
}

void
process_menu (int num)
{
	int sel = 0;
	int req, done;
	int last_num;

	struct menudesc *m;

	m = &menus[num];

	done = FALSE;

	/* Initialize? */
	if (menu_init()) {
		__menu_initerror();
		return;
	}

	if (__m_endwin) {
     		wclear(stdscr);
		wrefresh(stdscr);
		__m_endwin = 0;
	}
	if (m->mw == NULL)
		init_menu (m);

	/* Always preselect option 0 and display from 0! */
	m->cursel = 0;
	m->topline = 0;

	while (!done) {
		last_num = num;
		if (__m_endwin) {
			wclear(stdscr);
			wrefresh(stdscr);
			__m_endwin = 0;
		}
		/* Process the display action */
		if (m->post_act)
			(*m->post_act)();
		post_menu (m);
		wrefresh (m->mw);

		while ((req = menucmd (m->mw)) != REQ_EXECUTE)
			process_req (m, num, req);

		sel = m->cursel;
		wclear (m->mw);
		wrefresh (m->mw);

		/* Process the items */
		if (sel < m->numopts) {
			if (m->opts[sel].opt_flags & OPT_ENDWIN) {
				endwin();
				__m_endwin = 1;
			}
			if (m->opts[sel].opt_action)
				done = (*m->opts[sel].opt_action)(m);
			if (m->opts[sel].opt_menu != -1) {
				if (m->opts[sel].opt_flags & OPT_SUB)
					process_menu (m->opts[sel].opt_menu);
				else
					num = m->opts[sel].opt_menu;
			}

                        if (m->opts[sel].opt_flags & OPT_EXIT) 
                                done = TRUE;
 				
		} else
			done = TRUE;

		/* Reselect m just in case */
		if (num != last_num) {
			m = &menus[num];

			/* Initialize? */
			if (m->mw == NULL)
				init_menu (m);
			if (m->post_act)
				(*m->post_act)();
		}
	}

	/* Process the exit action */
	if (m->exit_act)
		(*m->exit_act)();
}


/* Beginning of routines for dynamic menus. */

/* local prototypes */
static int double_menus (void);

static int 
double_menus (void)
{
	menudesc *temp;

	temp  = (menudesc *) malloc(sizeof(menudesc)*num_menus*2);
	if (temp == NULL)
		return 0;
	(void) memset ((void *)temp, 0,
		sizeof(menudesc)*num_menus*2);
	(void) memcpy ((void *)temp, (void *)menus,
		sizeof(menudesc)*num_menus);
	free (menus);
	menus = temp;
	num_avail = num_menus;
	num_menus *= 2;

	return 1;
}

int
new_menu (char * title, menu_ent * opts, int numopts, 
	int x, int y, int h, int w, int mopt,
	void (*post_act)(void), void (*exit_act)(void), char * help)
{
	int ix;

	/* Check for free menu entry. */
	if (num_avail == 0)
		if (!double_menus ())
			return -1;

	/* Find free menu entry. */
	for (ix = DYN_MENU_START; ix < num_menus && menus[ix].mopt & MC_VALID;
		ix++) /* do  nothing */;

	/* if ix == num_menus ... panic */

	/* Set Entries */
	menus[ix].title = title ? title : "";
	menus[ix].opts = opts;
	menus[ix].numopts = numopts;
	menus[ix].x = x;
	menus[ix].y = y;
	menus[ix].h = h;
	menus[ix].w = w;
	menus[ix].mopt = mopt | MC_VALID;
	menus[ix].post_act = post_act;
	menus[ix].exit_act = exit_act;
	menus[ix].helpstr  = help;
	menus[ix].exitstr  = "Exit";

	init_menu (&menus[ix]);

	return ix;
}

void
free_menu (int menu_no)
{
	if (menu_no < num_menus) {
		menus[menu_no].mopt &= ~MC_VALID;
		if (menus[menu_no].mw != NULL)
			delwin (menus[menu_no].mw);
	}
}
