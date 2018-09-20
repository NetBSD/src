/*	$NetBSD: menus.pm,v 1.1 2018/09/20 12:27:42 rin Exp $	*/
/*	NetBSD: menus.mi,v 1.14 2018/09/11 08:05:18 martin Exp 		*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Menu system definitions -- extended partitioning */

menu pmdiskentry, x=50, y=5, exit, default exit;
	option MSG_editbsdpart, exit, action { pm_make_bsd_partitions(pm); };
	option MSG_editmbr,		exit, action { md_get_info();
											md_pre_disklabel();
											memset(&pm->bsdlabel, 0, sizeof pm->bsdlabel);};
	option MSG_switchgpt,	exit, action { if (pm_gpt_convert(pm) == 0)
												pm_partusage(pm, -1, 1); };
	option MSG_renamedisk,	exit, action { pm->unsaved = 1; pm_rename(pm); };
	option MSG_fmtasraid,	exit, action { pm->unsaved = 1;
											pm_partusage(pm, -1, 1);
											layoutkind = LY_NEWRAID;
											md_make_bsd_partitions();};
	option MSG_fmtaslvm,	exit, action { pm->unsaved = 1;
											pm_partusage(pm, -1, 1);
											layoutkind = LY_NEWLVM;
											md_make_bsd_partitions(); };
	option MSG_encrypt,		exit, action { pm->unsaved = 1;
											pm_partusage(pm, -1, 1);
											layoutkind = LY_NEWCGD;
											md_make_bsd_partitions();
											pm_cgd_edit(0, &(part_entry_t)
												{.dev_ptr = pm, .dev_num = PART_E}
											); };
	option MSG_setbootable,	exit, action { pm->unsaved = 1;
											pm->bootable = !pm->bootable; };
	option MSG_erase,		next menu shred_modes;
	option MSG_undo,		exit, action { label_read(); pm->unsaved = 0;
											pm_partusage(pm, -1, 1); };
	option MSG_unconfig,	exit, action { if (pm_unconfigure(pm) == 0)
												pm_partusage(pm, -1, 1); };

menu pmpartentry, x=50, y=5, exit, default exit;
	option MSG_edit,		exit, action { 
									pm->unsaved = 1;
									int tpfs = pm->bsdlabel[*(int*)arg].pi_fstype;
									int tplvm = pm->bsdlabel[*(int*)arg].lvmpv;
									pm_editpart(*(int*)arg);
									if (tpfs != pm->bsdlabel[*(int*)arg].pi_fstype ||
										tplvm != pm->bsdlabel[*(int*)arg].lvmpv)
										/* Oops, partition type changed */
										pm_partusage(pm, *(int*)arg, 1);
								};
	option MSG_fmtasraid,	exit, action {
									if (pm->gpt || pm->isspecial) {
										process_menu(MENU_ok, __UNCONST(MSG_notsupported));
										return -1;
									}
									pm->unsaved = 1;
									pm_partusage(pm, *(int*)arg, 1);
									pm_setfstype(pm, *(int*)arg, FS_RAID);
								};
	option MSG_fmtaslvm,	exit, action {
									if (pm->gpt || pm->isspecial) {
										process_menu(MENU_ok, __UNCONST(MSG_notsupported));
										return -1;
									}
									pm->unsaved = 1;
									pm_partusage(pm, *(int*)arg, 1);
									pm_setfstype(pm, *(int*)arg, FS_BSDFFS);
								    pm->bsdlabel[*(int*)arg].lvmpv = 1;
								};
	option MSG_encrypt,		exit, action {
									if (pm->gpt || pm->isspecial) {
										process_menu(MENU_ok, __UNCONST(MSG_notsupported));
										return -1;
									}
									pm->unsaved = 1;
									pm_partusage(pm, *(int*)arg, 1);
									pm_setfstype(pm, *(int*)arg, FS_CGD);
									pm_cgd_edit(0,
										&(part_entry_t){.dev_ptr = pm,
														.dev_num = *(int*)arg});
								};
	option MSG_erase,		next menu shred_modes;
	option MSG_doumount,	exit, action { pm_umount(pm, *(int*)arg); };
	option MSG_Delete_partition,	exit, action {
									pm->unsaved = 1;
									pm_partusage(pm, *(int*)arg, 1);
									if (pm->isspecial)
										pm_unconfigure(pm);
									else
										pm->bsdlabel[*(int*)arg].pi_fstype = FS_UNUSED;
									};

menu pmgptentry, x=50, y=8, exit, default exit;
	option MSG_editbsdpart, exit, action { pm_make_bsd_partitions(pm); };
	option MSG_switchmbr,	exit, action { if (pm_gpt_convert(pm) == 0)
												pm_partusage(pm, -1, 1); };
	option MSG_setbootable,	exit, action { pm->unsaved = 1;
												pm->bootable = !pm->bootable; };
	option MSG_erase,		next menu shred_modes;
	option MSG_undo,		exit, action { label_read(); pm->unsaved = 0;
											pm_partusage(pm, -1, 1); };
	option MSG_unconfig,	exit, action { if (pm_unconfigure(pm) == 0)
												pm_partusage(pm, -1, 1); };

menu shred_modes, x=50, y=5, exit, default exit;
	option MSG_fillzeros,	exit,
							action { pm_shred(pm, *(int*)arg, SHRED_ZEROS); };
	option MSG_fillrandom,	exit,
							action { pm_shred(pm, *(int*)arg, SHRED_RANDOM); };
	option MSG_fillcrypto,	exit,
							action { pm_shred(pm, *(int*)arg, SHRED_CRYPTO); };

menu raidlevel;
	option MSG_raid0, exit, action { *(int *)arg = 0; };
	option MSG_raid1, exit, action { *(int *)arg = 1; };
	option MSG_raid4, exit, action { *(int *)arg = 4; };
	option MSG_raid5, exit, action { *(int *)arg = 5; };

menu cgd_enctype;
	option "aes-xts",			exit, action { *(const char**)arg = "aes-xts"; };
	option "aes-cbc",			exit, action { *(const char**)arg = "aes-cbc"; };
	option "3des-cbc",			exit, action { *(const char**)arg = "3des-cbc"; };
	option "blowfish-cbc",		exit, action { *(const char**)arg = "blowfish-cbc"; };

menu cgd_ivtype;
	option "encblkno1",			exit, action { *(const char**)arg = "encblkno1"; };
	option "encblkno8",			exit, action { *(const char**)arg = "encblkno8"; };

menu cgd_keygentype;
	option "pkcs5_pbkdf2/sha1",	exit, action { *(const char**)arg = "pkcs5_pbkdf2/sha1"; };
	option "pkcs5_pbkdf2",		exit, action { *(const char**)arg = "pkcs5_pbkdf2"; };
	option "storedkey",			exit, action { *(const char**)arg = "storedkey"; };
	option "randomkey",			exit, action { *(const char**)arg = "randomkey"; };
	option "urandomkey",		exit, action { *(const char**)arg = "urandomkey"; };
	option "shell_cmd",			exit, action { *(const char**)arg = "shell_cmd"; };

menu cgd_verifytype;
	option "none",				exit, action { *(const char**)arg = "none"; };
	option "disklabel",			exit, action { *(const char**)arg = "disklabel"; };
	option "ffs",				exit, action { *(const char**)arg = "ffs"; };
	option "re-enter",			exit, action { *(const char**)arg = "re-enter"; };
