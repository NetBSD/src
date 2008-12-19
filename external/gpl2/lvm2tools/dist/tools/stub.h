/*	$NetBSD: stub.h,v 1.2 2008/12/19 15:24:18 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define unimplemented \
	log_error("Command not implemented yet."); return ECMD_FAILED

/*int e2fsadm(struct cmd_context *cmd, int argc, char **argv) unimplemented*/
int lvmsadc(struct cmd_context *cmd __attribute((unused)),
	    int argc __attribute((unused)),
	    char **argv __attribute((unused)))
{
	unimplemented;
}

int lvmsar(struct cmd_context *cmd __attribute((unused)),
	   int argc __attribute((unused)),
	   char **argv __attribute((unused)))
{
	unimplemented;
}

int pvdata(struct cmd_context *cmd __attribute((unused)),
	   int argc __attribute((unused)),
	   char **argv __attribute((unused)))
{
	log_error("There's no 'pvdata' command in LVM2.");
	log_error("Use lvs, pvs, vgs instead; or use vgcfgbackup and read the text file backup.");
	log_error("Metadata in LVM1 format can still be displayed using LVM1's pvdata command.");
	return ECMD_FAILED;
}

/*	$NetBSD: stub.h,v 1.2 2008/12/19 15:24:18 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define unimplemented \
	log_error("Command not implemented yet."); return ECMD_FAILED

/*int e2fsadm(struct cmd_context *cmd, int argc, char **argv) unimplemented*/
int lvmsadc(struct cmd_context *cmd __attribute((unused)),
	    int argc __attribute((unused)),
	    char **argv __attribute((unused)))
{
	unimplemented;
}

int lvmsar(struct cmd_context *cmd __attribute((unused)),
	   int argc __attribute((unused)),
	   char **argv __attribute((unused)))
{
	unimplemented;
}

int pvdata(struct cmd_context *cmd __attribute((unused)),
	   int argc __attribute((unused)),
	   char **argv __attribute((unused)))
{
	log_error("There's no 'pvdata' command in LVM2.");
	log_error("Use lvs, pvs, vgs instead; or use vgcfgbackup and read the text file backup.");
	log_error("Metadata in LVM1 format can still be displayed using LVM1's pvdata command.");
	return ECMD_FAILED;
}

