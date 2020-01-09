/*	$NetBSD: menus.pm,v 1.3 2020/01/09 13:22:30 martin Exp $	*/
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

/* arg is a struct part_entry* */
menu pmdiskentry, x=50, y=5, exit, default exit;
	display action { pmdiskentry_enable(menu, arg); };
	option MSG_edit_parts, exit, action { pm_edit_partitions(arg); };
	option MSG_switch_parts, exit, action {
		struct pm_devs *my_pm = pm_from_pe(arg);
		const char *err;

		if (!convert_scheme(my_pm, false, &err))
			err_msg_win(err);
		else
			pm_partusage(my_pm, -1, 1);
	};
	option MSG_fmtasraid, exit, action
	{
		pm_whole_disk(arg, SY_NEWRAID);
	};
	option MSG_fmtaslvm, exit, action
	{
		part_id new_part;

		new_part = pm_whole_disk(arg, SY_NEWLVM);
		if (new_part != NO_PART)
			pm_set_lvmpv(pm_from_pe(arg), new_part, true);
	};
	option MSG_encrypt, exit, action
	{
		part_id new_part;
		new_part = pm_whole_disk(arg, SY_NEWCGD);
		if (new_part != NO_PART)
			 pm_cgd_edit_new(pm_from_pe(arg), new_part);
	};
	option MSG_erase,		next menu shred_modes;
	option MSG_undo,		exit, action {
				/* label_read(); */ pm->unsaved = 0;
				pm_partusage(pm, -1, 1);
	};
	option MSG_unconfig,	exit, action {
				if (pm_unconfigure(pm) == 0)
					pm_partusage(pm, -1, 1);
	};

/* arg is a struct part_entry* */
menu pmpartentry, x=50, y=5, exit, default exit;
	option MSG_edit, exit, action
	{
		struct part_entry *cur_pe = arg;
		pm->unsaved = 1;
		pm_editpart(cur_pe->id);
		pm_partusage(pm, cur_pe->id, 1);
	};
	option MSG_fmtasraid, exit, action {
		if (pm->no_part) {
			hit_enter_to_continue(NULL, MSG_notsupported);
			return -1;
		}
		struct part_entry *cur_pe = arg;
		pm->unsaved = 1;
		pm_partusage(pm, cur_pe->id, 1);
		pm_setfstype(pm, cur_pe->id, FS_RAID, 0);
	};
	option MSG_fmtaslvm, exit, action {
		if (pm->no_part) {
			hit_enter_to_continue(NULL, MSG_notsupported);
			return -1;
		}
		struct part_entry *cur_pe = arg;
		pm->unsaved = 1;
		pm_partusage(pm, cur_pe->id, 1);
		pm_setfstype(pm, cur_pe->id, FS_BSDFFS, 2);
		pm_set_lvmpv(pm, cur_pe->id, true);
	};
	option MSG_encrypt, exit, action {
		if (pm->no_part) {
			hit_enter_to_continue(NULL, MSG_notsupported);
			return -1;
		}
		struct part_entry *cur_pe = arg;
		pm->unsaved = 1;
		pm_partusage(pm, cur_pe->id, 1);
		pm_setfstype(pm, cur_pe->id, FS_CGD, 0);
		pm_cgd_edit_old(cur_pe);
	};
	option MSG_erase, next menu shred_modes;
	option MSG_doumount, exit, action {
		struct part_entry *cur_pe = arg;
		pm_umount(pm, cur_pe->id);
	};
	option MSG_Delete_partition, exit, action {
		struct part_entry *cur_pe = arg;
		pm->unsaved = 1;
		pm_partusage(pm, cur_pe->id, 1);
		if (pm->no_part)
			pm_unconfigure(pm);
		else
			pm_setfstype(pm, cur_pe->id, FS_UNUSED, 0);
	};

/* arg is a struct part_entry* */
menu shred_modes, x=50, y=5, exit, default exit;
	option MSG_fillzeros, exit,
		action { pm_shred(arg, SHRED_ZEROS); };
	option MSG_fillrandom,	exit,
		action { pm_shred(arg, SHRED_RANDOM); };

/* arg is a int pointer for the requested raid level */
menu raidlevel;
	option MSG_raid0, exit, action { *(int *)arg = 0; };
	option MSG_raid1, exit, action { *(int *)arg = 1; };
	option MSG_raid4, exit, action { *(int *)arg = 4; };
	option MSG_raid5, exit, action { *(int *)arg = 5; };

/* arg is a const char ** set to the selected encryption type */
menu cgd_enctype;
	option "aes-xts", exit, action { *(const char**)arg = "aes-xts"; };
	option "aes-cbc", exit, action { *(const char**)arg = "aes-cbc"; };
	option "3des-cbc", exit, action { *(const char**)arg = "3des-cbc"; };
	option "blowfish-cbc", exit, action
				{ *(const char**)arg = "blowfish-cbc"; };

/* arg is a const char ** set to the requested iv type */
menu cgd_ivtype;
	option "encblkno1", exit, action { *(const char**)arg = "encblkno1"; };
	option "encblkno8", exit, action { *(const char**)arg = "encblkno8"; };

/* arg is const char ** set to the key gen type */
menu cgd_keygentype;
	option "pkcs5_pbkdf2/sha1", exit, action
				 { *(const char**)arg = "pkcs5_pbkdf2/sha1"; };
	option "pkcs5_pbkdf2", exit, action
				{ *(const char**)arg = "pkcs5_pbkdf2"; };
	option "storedkey", exit, action
				{ *(const char**)arg = "storedkey"; };
	option "randomkey", exit, action
				{ *(const char**)arg = "randomkey"; };
	option "urandomkey", exit, action
				{ *(const char**)arg = "urandomkey"; };
	option "shell_cmd", exit, action
				{ *(const char**)arg = "shell_cmd"; };

/* art is a const char ** set to the requested verification type */
menu cgd_verifytype;
	option "none",		exit, action { *(const char**)arg = "none"; };
	option "disklabel",	exit, action { *(const char**)arg = "disklabel"; };
	option "MBR",		exit, action { *(const char**)arg = "mbr"; };
	option "GPT",		exit, action { *(const char**)arg = "gpt"; };
	option "ffs",		exit, action { *(const char**)arg = "ffs"; };
	option "re-enter",	exit, action { *(const char**)arg = "re-enter"; };
