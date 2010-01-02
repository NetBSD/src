/*	$NetBSD: menus.md.es,v 1.3 2010/01/02 17:15:07 dsl Exp $	*/

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
 *      This product includes software developed for the NetBSD Project by
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

/* Menu definitions for sysinst. mac68k version, machine dependent. */

menu fullpart, title  "Seleccione su opcion";
	option "Usar solo parte del disco", exit, action  {usefull = 0;};
	option "Usar el disco entero", 	    exit, action  {usefull = 1;};

menu nodiskmap, title "Escoja una opcion", y=16;
	display action { msg_display (MSG_nodiskmap, diskdev); };
	option "Abortar instalacion", exit, action {
		endwin();  exit(1);
	};
	option "Inicializar Mapa de particiones de Disco", exit, action {
		int i;

		msg_clear();
		msg_display (MSG_okwritediskmap);
		process_menu (MENU_okabort, NULL);
		if (!yesno) {
		    endwin();
		    return 0;
		}
		map.size = NEW_MAP_SIZE;
		map.in_use_cnt = new_map[0].pmMapBlkCnt;
		map.blk = (struct apple_part_map_entry *)calloc(map.size,
			sizeof(struct apple_part_map_entry));
		for (i=0;i<map.size;i++)
		    memcpy (&map.blk[i], &new_map[i],
			 sizeof(struct apple_part_map_entry));
	};

menu editparttable, title  "Escoja su particion", exit, y=14;
	display action  { msg_display (MSG_editparttable);
			  sortmerge();
			  if (map.selected >= map.usable_cnt)
				map.selected = 0;
			  disp_selected_part (map.selected);
			};
	option "Seleccione particion siguiente",  action {
		map.selected += 1;
		if (map.selected >= map.usable_cnt)
			map.selected = 0;
		};
	option "Cambiar particion seleccionada", sub menu chooseid;
	option "Ajustar punto de montaje para particion", sub menu mount_point;
	option "Partir particion seleccionada", action {
		int i, j, k, size, free_size;
		char buf[40];
		EBZB *bzb;

		j = map.mblk[map.selected];
		msg_display(MSG_split_part, map.blk[j].pmPartBlkCnt);
		msg_prompt_add (MSG_scratch_size, NULL, buf, sizeof(buf));
		size = atoi(buf);
		if (size > 0 && size < map.blk[j].pmPartBlkCnt) {
		    k = map.in_use_cnt+1;
		    if (k <= map.size) {
			memcpy (&map.blk[k], &map.blk[j],
			    sizeof(struct apple_part_map_entry));
			free_size = map.blk[j].pmPartBlkCnt - size;
			strcpy (map.blk[j].pmPartType, "Apple_Scratch");
			map.blk[j].pmPartBlkCnt = size;
			map.blk[j].pmDataCnt = size;
			bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
			bzb->magic = 0;
			bzb->mount_point[0] = '\0';
			strcpy (map.blk[k].pmPartType, "Apple_Free");
			map.blk[k].pmPyPartStart += size;
			if ((map.blk[k].pmPyPartStart + free_size) > dlsize)
			    map.blk[k].pmPartBlkCnt =
				dlsize - map.blk[k].pmPyPartStart;
			else
			    map.blk[k].pmPartBlkCnt = free_size;
			map.blk[k].pmDataCnt = map.blk[k].pmPartBlkCnt;
			bzb = (EBZB *)&map.blk[k].pmBootArgs[0];
			bzb->magic = 0;
			bzb->mount_point[0] = '\0';
			map.in_use_cnt += 1;	/* Count new part as usable */
			sortmerge();
		    } else {
			msg_display (MSG_diskfull);
			process_menu (MENU_okabort, NULL);
			if (!yesno) {
			    free (map.blk);
			    map.size = NEW_MAP_SIZE;
			    map.in_use_cnt = new_map[0].pmMapBlkCnt;
			    map.blk = (struct apple_part_map_entry *)calloc(map.size,
				sizeof(struct apple_part_map_entry));
			    for (i=0;i<map.size;i++)
				memcpy (&map.blk[i], &new_map[i],
				    sizeof(struct apple_part_map_entry));
			    map.blk[0].pmSigPad = 0; /* Don't rewrite Block0 */
			}
		    }
		} };
	option "Arreglar particion seleccionada", action {
		int i = map.mblk[map.selected];
		EBZB *bzb = (EBZB *)&map.blk[i].pmBootArgs[0];
		msg_display(MSG_partdebug, diskdev, bzb->flags.part,
			map.blk[i].pmPyPartStart,
			map.blk[i].pmPartBlkCnt);
		if ((map.blk[i].pmPyPartStart +
		    map.blk[i].pmPartBlkCnt) > dlsize) {
			msg_display_add(MSG_parttable_fix_fixing,
				diskdev, bzb->flags.part);
			map.blk[i].pmPartBlkCnt =
			    dlsize - map.blk[i].pmPyPartStart;
			map.blk[i].pmDataCnt =
			    map.blk[i].pmPartBlkCnt;
		} else {
		    msg_display_add(MSG_parttable_fix_fine,
			diskdev, bzb->flags.part);
		}
		process_menu(MENU_ok, NULL);
		};


menu ok2, title "¿Abortar?", y=17;
       option "OK", exit, action { };

menu okabort, title "¿Que quiere hacer?", y=17;
	option "Continuar", exit, action { yesno = 1; };
	option "Abortar instalacion", exit, action { yesno = 0; };

menu chooseid, title  "¿Tipo de particion?";
	option "NetBSD Root", exit, action {
		int i, j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		reset_part_flags(&map.blk[j]);
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		bzb->magic = APPLE_BZB_MAGIC;
		strcpy (map.blk[j].pmPartName, "NetBSD Root");
		strcpy (map.blk[j].pmPartType, "Apple_Unix_SVR2");
		bzb->type = APPLE_BZB_TYPEFS;
		bzb->flags.root = 1;
		/*
		 * Automatically determine root mount points.  The first
		 *  root-type filesystem is mounted on "/", all others
		 *  will mount on "/altroot".  If there are multiple
		 *  occurances of "/altroot" they will be picked up on
		 *  the sanity_scan in the next step of the installation.
		 */
		for (i=0,map.root_cnt=0;i<map.usable_cnt;i++) {
		    j = map.mblk[i];
		    if (whichType(&map.blk[j]) == ROOT_PART) {
			bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
			if (bzb->type == APPLE_BZB_TYPEFS && bzb->flags.root) {
			   if (map.root_cnt++ == 0)
				strcpy (bzb->mount_point, "/");
			   else
				strcpy (bzb->mount_point, "/altroot");
			}
		    }
		} };
	option "NetBSD SWAP", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		reset_part_flags(&map.blk[j]);
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		bzb->magic = APPLE_BZB_MAGIC;
		strcpy (map.blk[j].pmPartName, "NetBSD SWAP");
		strcpy (map.blk[j].pmPartType, "Apple_Unix_SVR2");
		bzb->type = APPLE_BZB_TYPESWAP; };
	option "NetBSD Usr", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		reset_part_flags(&map.blk[j]);
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		bzb->magic = APPLE_BZB_MAGIC;
		strcpy (map.blk[j].pmPartName, "NetBSD Usr");
		strcpy (map.blk[j].pmPartType, "Apple_Unix_SVR2");
		bzb->type = APPLE_BZB_TYPEFS;
		bzb->flags.usr = 1;
		if (map.usr_cnt++ == 0)
		    strcpy (bzb->mount_point, "/usr");
		};
	option "NetBSD Root&Usr", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		reset_part_flags(&map.blk[j]);
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		bzb->magic = APPLE_BZB_MAGIC;
		strcpy (map.blk[j].pmPartName, "NetBSD Root & Usr");
		strcpy (map.blk[j].pmPartType, "Apple_Unix_SVR2");
		bzb->type = APPLE_BZB_TYPEFS;
		bzb->flags.root = 1;
		bzb->flags.usr = 1;
		if (map.root_cnt++ == 0)
		    strcpy (bzb->mount_point, "/");
		else {
		    if (map.usr_cnt++ == 0)
			strcpy (bzb->mount_point, "/usr");
		} };
	option "MacOS HFS", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		reset_part_flags(&map.blk[j]);
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		bzb->magic = 0;
		bzb->mount_point[0] = '\0';
		strcpy (map.blk[j].pmPartName, "sin nombre (HFS)");
		strcpy (map.blk[j].pmPartType, "Apple_HFS"); };
	option "Scratch", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		reset_part_flags(&map.blk[j]);
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		bzb->magic = 0;
		bzb->mount_point[0] = '\0';
		strcpy (map.blk[j].pmPartName, "sin nombre (Scratch)");
		strcpy (map.blk[j].pmPartType, "Apple_Scratch"); };
	option "Libre", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		reset_part_flags(&map.blk[j]);
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		bzb->magic = 0;
		bzb->mount_point[0] = '\0';
		strcpy (map.blk[j].pmPartName, "sin nombre (Libre)");
		strcpy (map.blk[j].pmPartType, "Apple_Libre"); };

menu mount_point, title  "¿Punto de montaje?";
	option "/usr", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		strcpy (bzb->mount_point, "/usr"); };
	option "/home", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		strcpy (bzb->mount_point, "/home"); };
	option "/var", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		strcpy (bzb->mount_point, "/var"); };
	option "/tmp", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		strcpy (bzb->mount_point, "/tmp"); };
	option "Ninguna", exit, action {
		int j;
		EBZB *bzb;

		j = map.mblk[map.selected];
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		bzb->mount_point[0] = '\0'; };
	option "otra", exit, action {
		int j;
		char buf[60];
		EBZB *bzb;

		msg_display (MSG_custom_mount_point);
		msg_prompt_add (MSG_mountpoint, NULL, buf, sizeof(buf));
		j = map.mblk[map.selected];
		bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
		if (buf[0] != '\0') {
		    bzb->mount_point[0] = '\0';
		    if (buf[0] != '/')
			strcpy (bzb->mount_point, "/");
		    strcat(bzb->mount_point, buf);
		} };

menu sanity, title "Escoja una opcion";
	display action {msg_display (MSG_sanity_check);
		report_errors(); };
	option "Abortar instalacion", exit, action {yesno = -1; };
	option "Ignorar avisos y continuar", exit, action {yesno = 1;};
	option "Re-editar Mapa de Particiones de Disco", exit, action {yesno = 0; };

/*
 * This menu shouldn't be used in the mac68k port, but it needs to be
 *  defined because it's referenced from the toplevel code.
 */
