#include "msg_defs.h"
const char *msg_list[] = {
"usage: sysinst [-r release] [-f definition-file]\n"
"","\n","Yes","No","install","reinstall sets for","upgrade","Welcome to sysinst, the NetBSD-1.5ZA system installation tool.\n"
"This menu-driven tool is designed to help you install NetBSD to a hard\n"
"disk, or upgrade an existing NetBSD system, with a minimum of work.\n"
"In the following menus, you may change the current selection by\n"
"either typing the reference letter (a, b, c, ...).  Arrow keys may also work.\n"
"You activate the current selection from the menu by typing the enter key.\n"
"\n"
"","Thank you for using NetBSD!\n"
"","You have chosen to install NetBSD on your hard disk.  This will change\n"
"information on your hard disk.  You should have made a full backup\n"
"before this procedure!  This procedure will do the following things: \n"
"	a) Partition your disk \n"
"	b) Create new BSD file systems \n"
"	c) Load and install distribution sets\n"
"\n"
"(After you enter the partition information but before your disk is\n"
"changed, you will have the opportunity to quit this procedure.)\n"
"\n"
"Shall we continue?\n"
"","Ok, lets upgrade NetBSD on your hard disk.  As always, this will\n"
"change information on your hard disk.  You should have made a full backup\n"
"before this procedure!  Do you really want to upgrade NetBSD?\n"
"(This is your last warning before this procedure starts modifying your\n"
"disks.)\n"
"","Ok, lets unpack the NetBSD distribution sets to a bootable hard disk.\n"
"This procedure just fetches and unpacks sets onto an pre-partitioned\n"
"bootable disk. It does not label disks, upgrade bootblocks, or save\n"
"any existing configuration info.  (Quit and choose `install' or\n"
"`upgrade' if you want those options.)  You should have already done an\n"
"`install' or `upgrade' before starting this procedure!\n"
"\n"
"Do you really want to reinstall NetBSD distribution sets?\n"
"(This is your last warning before this procedure starts modifying your\n"
"disks.)\n"
"","I can not find any hard disks for use by NetBSD.  You will be\n"
"returned to the original menu.\n"
"","I found only one disk, %s.  Therefore I assume you want to %s\n"
"NetBSD on it.\n"
"","I have found the following disks: %s\n"
"\nOn which disk do you want to install NetBSD? ","You did not choose one of the listed disks.  Please try again.\n"
"The following disks are available: %s\n"
"\nOn which disk do you want to install NetBSD? ","Your hard disk is too small for a standard install. You will have to enter\n"
"the partitions size by hand.\n"
"","The root device is not mounted.  Please mount it.\n"
"\n"
"Your selected target disk %s also holds the current root device.  I\n"
"need to know whether I'm currently running out of the target root\n"
"(%sa), or out of an alternate root (say, in %sb, your swap partition).\n"
"I can't tell unless you mount the root from which you booted read/write\n"
"(eg 'mount /dev/%sb /').\n"
"\n"
"I'm aborting back to the main menu so you can fork a subshell.\n"
"","cylinders","heads","sectors","size","start","offset","block size","frag size","mount point","cyl","sec","MB","NetBSD uses a BSD disklabel to carve up the NetBSD portion of the disk\n"
"into multiple BSD partitions.  You must now set up your BSD disklabel.\n"
"You have several choices.  They are summarized below. \n"
"-- Standard: the BSD disklabel partitions are computed by this program. \n"
"-- Standard with X: twice the swap space, space for X binaries. \n"
"-- Custom: you specify the sizes of all the BSD disklabel partitions. \n"
"-- Use existing: use current partitions. You must assign mount points. \n"
"\n"
"The NetBSD part of your disk is %.2f Megabytes. \n"
"Standard requires at least %.2f Megabytes. \n"
"Standard with X requires at least %.2f Megabytes.\n"
"","You have elected to specify partition sizes (either for the BSD\n"
"disklabel, or on some ports, for MBR slices). You must first choose a\n"
"size unit to use.  Choosing megabytes will give partition sizes close\n"
"to your choice, but aligned to cylinder boundaries. Choosing sectors\n"
"will allow you to more accurately specify the sizes.  On modern ZBR disks,\n"
"actual cylinder size varies across the disk and there is little real\n"
"gain from cylinder alignment. On older disks, it is most efficient to\n"
"choose partition sizes that are exact multiples of your actual\n"
"cylinder size.\n"
"","Unless specified with 'M' (megabytes), 'c' (cylinders) or 's' sector\n"
"at the end of input, sizes and offsets are in %s.\n"
"","The start value you specified is beyond the end of the disk.\n"
"","With this value, the partition end is beyond the end of the disk. Your\n"
"partition size has been truncated to %d %s.\n"
"","We now have your BSD-disklabel partitions as (Size and Offset in %s):\n"
"\n"
"","   Size      Offset    End       FStype Bsize Fsize Preserve Mount point\n"
"   --------- --------- --------- ------ ----- ----- -------- -----------\n"
"","%c: %-9d %-9d %-9d %-6s ","%-5d %-5d %-8s %s\n","                     %s\n","\n","You should set the file system (FS) kind first.  Then set the other values.\n"
"\n"
"The current values for partition %c are:\n"
"\n"
"","Partition %c is not of type 4.2BSD and thus does not have a block and\n"
"frag size to set.\n"
"","Please enter a name for your NetBSD disk","Ok, we are now ready to install NetBSD on your hard disk (%s).  Nothing has been\n"
"written yet.  This is your last chance to quit this process before anything\n"
"gets changed.  \n"
"\n"
"Shall we continue?\n"
"","Okay, the first part of the procedure is finished.  Sysinst has\n"
"written a disklabel to the target disk, and newfs'ed and fsck'ed\n"
"the new partitions you specified for the target disk.\n"
"\n"
"The next step is to fetch and unpack the distribution filesets.\n"
"Press <return> to proceed.\n"
"","Okay, the first part of the procedure is finished.  Sysinst has\n"
"written a disklabel to the target disk, and fsck'ed the new\n"
"partitions you specified for the target disk.\n"
"\n"
"The next step is to fetch and unpack the distribution filesets.\n"
"Press <return> to proceed.\n"
"","Could not open %s, error message was: %s.\n"
"","Can't get properties of %s, error message was: %s.\n"
"","I was unable to delete %s, error message was: %s.\n"
"","I was unable to rename %s to %s, error message was: %s.\n"
"","As part of the upgrade procedure, the following have to be deleted:\n"
"","As part of the upgrade procedure, the following directories have to be\n"
"deleted (I will rename those that are not empty):\n"
"","The directory %s has been renamed to %s because it was not empty.\n"
"","Cleanup of the existing install failed. This may cause the extraction\n"
"of the set to fail.\n"
"","Partition %c's type is not 4.2BSD or msdos and therefore does not have\n"
"a mount point.","mount of device %s on %s failed.\n"
"","Populating filesystems with bootstrapping binaries and config files...\n","The bootstrapping binaries and config file installation failed.\n"
"Can't continue ...","The extraction of the selected sets for NetBSD-1.5ZA is complete.\n"
"The system is now able to boot from the selected harddisk. To complete\n"
"the installation, sysinst will give you the opportunity to configure\n"
"some essential things first.\n"
"","The installation of NetBSD-1.5ZA is now complete.  The system\n"
"should boot from hard disk.  Follow the instructions in the INSTALL\n"
"document about final configuration of your system.\n"
"\n"
"At a minimum, you should edit /etc/rc.conf to match your needs. See\n"
"/etc/defaults/rc.conf for the default values.\n"
"","The upgrade to NetBSD-1.5ZA is now complete.  You will\n"
"now need to follow the instructions in the INSTALL document as to\n"
"what you need to do to get your system reconfigured for your\n"
"situation.  Your old /etc has been saved as /etc.old.\n"
"\n"
"At a minimum, you must edit rc.conf for your local environment and change\n"
"rc_configured=NO to rc_configured=YES or reboots will stop at single-user,\n"
"and copy back the password files (taking into account new system accounts\n"
"that may have been created for this release) if you were using local\n"
"password files.\n"
"","Unpacking additional release sets of NetBSD-1.5ZA is now\n"
"complete.  Unpacking sets has clobbered the target /etc.  Any /etc.old\n"
"saved by an earlier upgrade was not touched.  You will now need to\n"
"follow the instructions in the INSTALL document to get your system\n"
"reconfigured for your situation.\n"
"\n"
"At a minimum, you must edit rc.conf for your local environment and change\n"
"rc_configured=NO to rc_configured=YES or reboots will stop at single-user.\n"
"","Your disk is now ready for installing the kernel and the distribution\n"
"sets.  As noted in your INSTALL notes, you have several options.  For\n"
"ftp or nfs, you must be connected to a network with access to the proper\n"
"machines.  If you are not ready to complete the installation at this time,\n"
"you may select \"none\" and you will be returned to the main menu.  When\n"
"you are ready at a later time, you may select \"upgrade\" from the main\n"
"menu to complete the installation.\n"
"","The NetBSD distribution is broken into a collection of distribution\n"
"sets.  There are some basic sets that are needed by all installations\n"
"and there are some other sets that are not needed by all installations.\n"
"You may choose to install all of them (Full installation) or you\n"
"select from the optional distribution sets.\n"
"","The following is the ftp site, directory, user, and password currently\n"
"ready to use.  If \"user\" is \"ftp\", then the password is not needed.\n"
"\n"
"host:		%s \n"
"directory:	%s \n"
"user:		%s \n"
"password:	%s \n"
"proxy:		%s \n"
"","host","directory","user","password","proxy","e-mail address","device","Enter the nfs host and server directory where the distribution is\n"
"located.  Remember, the directory should contain the .tgz files and\n"
"must be nfs mountable.\n"
"\n"
"host:		%s \n"
"directory:	%s \n"
"","The directory %s:%s could not be nfs mounted.","Enter the CDROM device to be used and directory on the CDROM where\n"
"the distribution is located.  Remember, the directory should contain\n"
"the .tgz files.\n"
"\n"
"device:		%s\n"
"directory:	%s\n"
"","Enter the unmounted local device and directory on that device where\n"
"the distribution is located.  Remember, the directory should contain\n"
"the .tgz files.\n"
"\n"
"device:		%s\n"
"filesystem:	%s\n"
"directory:	%s\n"
"","Enter the already-mounted local directory where the distribution is\n"
"located.  Remember, the directory should contain the .tgz files.\n"
"\n"
"directory:	%s\n"
"","filesystem","The CDROM could not be mounted on device %s.","%s could not be mounted on local device %s.","%s is not a directory","%s does not contain the mandatory installation sets etc.tgz, \n"
"base.tgz and kern.tgz.  Are you sure you've got the right directory?","I can not find any network interfaces for use by NetBSD.  You will be\n"
"returned to the previous menu.\n"
"","I have found the following network interfaces: %s\n"
"\nWhich device shall I use?","You did not choose one of the listed network devices.  Please try again.\n"
"The following network devices are available: %s\n"
"\nWhich device shall I use?","To be able to use the network, we need answers to the following:\n"
"\n"
"","Your DNS domain","Your host name","Your IPv4 number","IPv4 Netmask","IPv6 name server","IPv4 name server","IPv4 gateway","Network media type","The following are the values you entered.  Are they OK?\n"
"\n"
"DNS Domain:		%s \n"
"Host Name:		%s \n"
"Primary Interface:	%s \n"
"Host IP:		%s \n"
"Netmask:		%s \n"
"IPv4 Nameserver:	%s \n"
"IPv4 Gateway:		%s \n"
"Media type:		%s \n"
"","IPv6 autoconf:		%s \n"
"IPv6 Nameserver:	%s \n"
"","Please reenter the information about your network.  Your last answers\n"
"will be your default.\n"
"\n"
"","Could not create /etc/resolv.conf.  Install aborted.\n"
"","Could not change to directory %s: %s.  Install aborted.\n"
"","Ftp detected an error.  Press <return> to continue.","Ftp could not retrieve a file.  Do you want to try again?","What directory shall I use for %s? ","During the extraction process, do you want to see the file names as\n"
"each file is extracted?\n"
"","Could not run /bin/ls.  This error should not have happened. Install aborted.\n"
"","Release set %s does not exist.\n"
"\n"
"Continue extracting sets?","All selected distribution sets unpacked successfully.","There were problems unpacking distribution sets.\n"
"Your installation is incomplete.\n"
"\n"
"You selected %d distribution sets.  %d sets couldn't be found\n"
"and %d were skipped after an error occurred.  Of the %d\n"
"that were attempted, %d unpacked without errors and %d with errors.\n"
"\n"
"The installation is aborted. Please recheck your distribution source\n"
"and consider reinstalling sets from the main menu.","Your choices have made it impossible to install NetBSD.  Install aborted.\n"
"","The distribution was not successfully loaded.  You will need to proceed\n"
"by hand.  Installation aborted.\n"
"","The distribution was not successfully loaded.  You will need to proceed\n"
"by hand.  Upgrade aborted.\n"
"","Unpacking additional sets was not successful.  You will need to\n"
"proceed by hand, or choose a different source for release sets and try\n"
"again.\n"
"","sysinst: running \"%s\"\n"
"","\n"
"The program \"%s\" failed unexpectedly with return code %s.\n"
"\n"
"This is probably due to choosing the incorrect top-level install\n"
"option---like trying to do an Upgrade on a bare disk, or doing a fresh\n"
"Install on an already-running system.  Or it might be due to a\n"
"mispackaged miniroot.  Whatever the cause, sysinst did not expect any\n"
"errors here and the installation has almost certainly failed.\n"
"\n"
"Check the error messages above and proceed with *extreme* caution.\n"
"Press <return> to continue.","\n"
"sysinst: Execution of \"%s\" failed unexpectedly with error code %s.\n"
"Cannot recover, aborting.\n"
"","There is a big problem!  Can not create /mnt/etc/fstab.  Bailing out!\n"
"","Help! No /etc/fstab in target disk %s.  Aborting upgrade.\n"
"","Help! Can't parse /etc/fstab in target disk %s.  Aborting upgrade.\n"
"","I cannot save /etc as /etc.old, because the target disk already has an\n"
"/etc.old. Please fix this before continuing.\n"
"\n"
"One way is to start a shell from the Utilities menu, examine the\n"
"target /etc and /etc.old.  If /etc.old is from a completed upgrade,\n"
"you can rm -f etc.old and restart.  Or if /etc.old is from a recent,\n"
"incomplete upgrade, you can rm -f /etc and mv etc.old to /etc.\n"
"\n"
"Aborting upgrade.","I cannot save /usr/X11R6/bin/X as /usr/X11R6/bin/X.old, because the\n"
"target disk already has an /usr/X11R6/bin/X.old. Please fix this before\n"
"continuing.\n"
"\n"
"One way is to start a shell from the Utilities menu, examine the\n"
"target /usr/X11R6/bin/X and /usr/X11R6/bin/X.old.  If\n"
"/usr/X11R6/bin/X.old is from a completed upgrade, you can rm -f\n"
"/usr/X11R6/bin/X.old and restart.  Or if /usr/X11R6/bin/X.old is from\n"
"a recent, incomplete upgrade, you can rm -f /usr/X11R6/bin/X and mv\n"
"/usr/X11R6/bin/X.old to /usr/X11R6/bin/X.\n"
"\n"
"Aborting upgrade.","There was a problem in setting up the network.  Either your gateway\n"
"or your nameserver was not reachable by a ping.  Do you want to\n"
"configure your network again?  (No aborts the install process.)\n"
"","Making device files ...\n"
"","It appears that %s%s is not a BSD file system or the fsck was\n"
"not successful.  The upgrade has been aborted.  (Error number %d.)\n"
"","Your file system %s%s was not successfully mounted.  Upgrade aborted.","Your file system, %s, is using an old inode format.  If you are\n"
"using only NetBSD on these file systems, it is recommended that\n"
"they are upgraded.  Do you want this file system upgraded?\n"
""," target root is missing %s.\n"
"","The completed new root fileystem failed a basic sanity check.\n"
" Are you sure you installed all the required sets?\n"
"","What floppy device do you want to use? ","Please load the floppy containing the file named \"%s\". ","Could not find the file named \"%s\" on the disk.  Please load the\n"
"floppy with that file on it.","The floppy was not mounted successfully.  You may:\n"
"\n"
"Try again and put in the floppy containing the file named \"%s\".\n"
"\n"
"Not load any more files from floppy and abort the process.\n"
"","Is the network information you entered accurate for this machine\n"
"in regular operation and do you want it installed in /etc? ","The following is the list of distribution sets that will be used.\n"
"\n"
"","Distribution set   Use?\n"
"------------------ ----\n"
"","%-18s %s\n","There was an error in extracting the file %s.  That means\n"
"some files were not extracted correctly and your system will not be\n"
"complete.\n"
"\n"
"Continue extracting sets?","partitions %c and %c overlap.","\n"
"\n"
"You can either edit the partition table by hand, or give up and return\n"
"to the main menu.\n"
"\n"
"Edit the partition table again?","Config file %s is not a regular file.\n","Out of memory (malloc failed).\n","Could not open config file %s\n","Could not read config file %s\n","Sysinst could not automatically determine the BIOS geometry of the disk.\n"
"The physical geometry is %d cylinders %d sectors %d heads\n","Using the information already on the disk, my best guess for the BIOS\n"
"geometry is %d cylinders %d sectors %d heads\n","Command\n"
"	%s\n"
"failed. I can't continue.","The directory where the old a.out shared libraries should be moved to could\n"
"not be created. Please try the upgrade procedure again and make sure you\n"
"have mounted all filesystems.","You have not marked a partition active. This may cause your system to not\n"
"start up properly. Should the NetBSD partition of the disk be marked active?","The only suitable partition that was found for NetBSD installation is of\n"
"the old NetBSD/386BSD/FreeBSD partition type. Do you want to change the type\n"
"of this partition to the new NetBSD-only partition type?","Continue?","Please choose the timezone that fits you best from the list below.\n"
"Press RETURN to select an entry. Press 'x' followed by RETURN to quit\n"
"the timezone selection. \n"
"\n"
" Default:	%s \n"
" Selected:	%s \n"
" Local time: 	%s %s \n"
"","The disk that you selected has a swap partition that may currently be\n"
"in use if your system is low on memory. Because you are going to\n"
"repartition this disk, this swap partition will be disabled now. Please\n"
"beware that this might lead to out of swap errors. Should you get such\n"
"an error, please restart the system and try again.","Sysinst failed to deactivate the swap partition on the disk that you\n"
"chose for installation. Please reboot and try again.","The root password of the newly installed system has not yet been initialized,\n"
"and is thus empty.  Do you want to set a root password for the system now?","\n"
"Special values that can be entered for the size value:\n"
"    -1:   use until the end of the NetBSD part of the disk\n"
"   a-%c:   end this partition where partition X starts\n"
"\n"
"","\n"
"Special values that can be entered for the offset value:\n"
"    -1:   start at the beginning of the NetBSD part of the disk\n"
"   a-%c:   start at the end of partition X\n"
"\n"
"","\n"
"Don't forget to fill in the file system mount points for each of the\n"
"file systems that are to be mounted.  Press <return> to continue.\n"
"","Currently selected file-systems\n"
"\n"
"","    Mount-point   Selected\n","    %s         %s\n","\n"
"There is no defined root file system.  You need to define at least\n"
"one mount point with \"/\".\n"
"\n"
"Press <return> to continue.\n"
"","If you booted from a floppy, you may now remove the disk.\n"
"\n"
"","We are now going to install NetBSD on the disk %s.  You may\n"
"choose to install NetBSD on the entire disk or part of the disk.\n"
"Which would you like to do?\n"
"","Cannot read filecore boot block\n"
"","Cannot read RISCiX partition table\n"
"","No NetBSD partition found in RISCiX partition table - Cannot label\n"
"","No NetBSD partition found (filecore only disk ?) - Cannot label\n"
"","Installing boot blocks on %s....\n"
"","I will be asking for partition sizes and on some, mount points.\n"
"\n"
"First the root partition.  You have %d %s left on your disk.\n"
"Root partition size? ","\n"
"Next the swap partition.  You have %d %s left on your disk.\n"
"Swap partition size? ","\n"
"Next the /usr partition.  You have %d %s left on your disk.\n"
"/usr partition size? ","You still have some space remaining unallocated on your disk.  Please\n"
"give sizes and mount points for the following partitions.\n"
"\n"
"","The next partition is /dev/%s%c.  You have %d %s left on your disk.\n"
"Partition size? ","We now have your NetBSD partitions on %s as follows (Size and Offset in %s):\n"
"",NULL};
/*	$NetBSD: msg_defs.c,v 1.1 2002/01/25 15:35:55 reinoud Exp $	*/

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

static WINDOW *msg_win = NULL;
static char *cbuffer;
static size_t cbuffersize;

static int last_i_was_nl, last_i_was_space;
static int last_o_was_punct, last_o_was_space;

static void	_msg_beep(void);
static int	_msg_vprintf(int auto_fill, const char *fmt, va_list ap);
static void	_msg_vprompt(const char *msg, int do_echo, const char *def,
		    char *val, int max_chars, va_list ap);

/* Routines */

static void
_msg_beep(void)
{
	fprintf(stderr, "\a");
}

int msg_window(WINDOW *window)
{
	size_t ncbuffersize;
	char *ncbuffer;

	msg_win = window;

	ncbuffersize = getmaxx(window) * getmaxy(window) + 1;
	if (ncbuffersize > cbuffersize) {
		ncbuffer = malloc(ncbuffersize);
		if (ncbuffer == NULL)
			return 1;
		if (cbuffer != NULL)
			free(cbuffer);
		cbuffer = ncbuffer;
		cbuffersize = ncbuffersize;
	}
	return 0;
}

const char *msg_string (msg msg_no)
{
	return msg_list[(long)msg_no];
}

void msg_clear(void)
{
	wclear (msg_win);
	wrefresh (msg_win);
	last_o_was_punct = 0;
	last_o_was_space = 1;
}

void msg_standout(void)
{
	wstandout(msg_win);
}

void msg_standend(void)
{
	wstandend(msg_win);
}

static int
_msg_vprintf(int auto_fill, const char *fmt, va_list ap)
{
	const char *wstart, *afterw;
	int wordlen, nspaces;
	int ret;

	ret = vsnprintf (cbuffer, cbuffersize, fmt, ap);

	if (!auto_fill) {
		waddstr(msg_win, cbuffer);

		/*
		 * nothing is perfect if they flow text after a table,
		 * but this may be decent.
		 */
		last_i_was_nl = last_i_was_space = 1;
		last_o_was_punct = 0;
		last_o_was_space = 1;
		goto out;
	}

	for (wstart = afterw = cbuffer; *wstart; wstart = afterw) {

		/* eat one space, or a whole word of non-spaces */
		if (isspace(*afterw))
			afterw++;
		else
			while (*afterw && !isspace(*afterw))
				afterw++;

		/* this is an nl: special formatting necessary */
		if (*wstart == '\n') {
			if (last_i_was_nl || last_i_was_space) {

				if (getcurx(msg_win) != 0)
					waddch(msg_win, '\n');
				if (last_i_was_nl) {
					/* last was an nl: paragraph break */
					waddch(msg_win, '\n');
				} else {
					/* last was space: line break */
				}
				last_o_was_punct = 0;
				last_o_was_space = 1;
			} else {
				/* last_o_was_punct unchanged */
				/* last_o_was_space unchanged */
			}
			last_i_was_space = 1;
			last_i_was_nl = 1;
			continue;
		}

		/* this is a tab: special formatting necessary. */
		if (*wstart == '\t') {
			if (last_i_was_nl) {
				/* last was an nl: list indent */
				if (getcurx(msg_win) != 0)
					waddch(msg_win, '\n');
			} else {
				/* last was not an nl: columns */
			}
			waddch(msg_win, '\t');
			last_i_was_nl = 0;
			last_i_was_space = 1;
			last_o_was_punct = 0;
			last_o_was_space = 1;
			continue;
		}

		/* this is a space: ignore it but set flags */
		last_i_was_nl = 0;	/* all newlines handled above */
		last_i_was_space = isspace(*wstart);
		if (last_i_was_space)
			continue;

		/*
		 * we have a real "word," i.e. a sequence of non-space
		 * characters.  wstart is now the start of the word,
		 * afterw is the next character after the end.
		 */
		wordlen = afterw - wstart;
		nspaces = last_o_was_space ? 0 : (last_o_was_punct ? 2 : 1);
		if ((getcurx(msg_win) + nspaces + wordlen) >=
		      getmaxx(msg_win) &&
		    wordlen < (getmaxx(msg_win) / 3)) {
			/* wrap the line */
			waddch(msg_win, '\n');
			nspaces = 0;
		}

		/* output the word, preceded by spaces if necessary */
		while (nspaces-- > 0)
			waddch(msg_win, ' ');
		waddbytes(msg_win, wstart, wordlen);

		/* set up the 'last' state for the next time around */
		switch (afterw[-1]) {
		case '.':
		case '?':
		case '!':
			last_o_was_punct = 1;
			break;
		default:
			last_o_was_punct = 0;
			break;
		}
		last_o_was_space = 0;

		/* ... and do it all again! */
	}

	/* String ended with a newline.  They really want a line break. */
	if (last_i_was_nl) {
		if (getcurx(msg_win) != 0)
			waddch(msg_win, '\n');
		last_o_was_punct = 0;
		last_o_was_space = 1;
	}

out:
	wrefresh (msg_win);
	return ret;
}

void msg_display(msg msg_no, ...)
{
	va_list ap;

	msg_clear();

	va_start(ap, msg_no);
	(void)_msg_vprintf(1, msg_string(msg_no), ap);
	va_end(ap);
}

void msg_display_add(msg msg_no, ...)
{
	va_list ap;

	va_start (ap, msg_no);
	(void)_msg_vprintf(1, msg_string(msg_no), ap);
	va_end (ap);
}


static void
_msg_vprompt(const char *msg, int do_echo, const char *def, char *val,
    int max_chars, va_list ap)
{
	int ch;
	int count = 0;
	int y,x;
	char *ibuf = alloca(max_chars);

	_msg_vprintf(0, msg, ap);
	if (def != NULL && *def) {
		waddstr (msg_win, " [");
		waddstr (msg_win, def);
		waddstr (msg_win, "]");
	}
	waddstr (msg_win, ": ");
	wrefresh (msg_win);

	while ((ch = wgetch(msg_win)) != '\n') {
		if (ch == 0x08 || ch == 0x7f) {  /* bs or del */
			if (count > 0) {
				count--;
				if (do_echo) {
					getyx(msg_win, y, x);
					x--;
					wmove(msg_win, y, x);
					wdelch(msg_win);
				}
			} else
				_msg_beep();
		} else if (ch == 0x15) {	/* ^U; line kill */
			while (count > 0) {
				count--;
				if (do_echo) {
					getyx(msg_win, y, x);
					x--;
					wmove(msg_win, y, x);
					wdelch(msg_win);
				}
			}
		} else if (ch == 0x17) {        /* ^W; word kill */
			/*
			 * word kill kills the spaces and the 'word'
			 * (non-spaces) last typed.  the spaces before
			 * the 'word' aren't killed.
			 */
			while (count > 0 && isspace(ibuf[count - 1])) {
				count--;
				if (do_echo) {
					getyx(msg_win, y, x);
					x--;
					wmove(msg_win, y, x);
					wdelch(msg_win);
				}
			}
			while (count > 0 && !isspace(ibuf[count - 1])) {
				count--;
				if (do_echo) {
					getyx(msg_win, y, x);
					x--;
					wmove(msg_win, y, x);
					wdelch(msg_win);
				}
			}
		} else if (count < (max_chars - 1) && isprint(ch)) {
			if (do_echo)
				waddch (msg_win, ch);
			ibuf[count++] = ch;
		} else
			_msg_beep();
		if (do_echo)
			wrefresh(msg_win);
	}
	if (do_echo) {
		waddch(msg_win, '\n');
		last_o_was_punct = 0;
		last_o_was_space = 1;
	}

	/* copy the appropriate string to the output */
	if (count != 0) {
		ibuf[count] = '\0';
		strcpy(val, ibuf);		/* size known to be OK */
	} else if (def != NULL && val != def) {
		strncpy(val, def, max_chars);
		val[max_chars - 1] = '\0';
	}
}

void
msg_prompt(msg msg_no, const char *def, char *val, int max_chars, ...)
{
	va_list ap;

	msg_clear();

	va_start (ap, max_chars);
	_msg_vprompt(msg_string(msg_no), 1, def, val, max_chars, ap);
	va_end (ap);
}

void
msg_prompt_add(msg msg_no, const char *def, char *val, int max_chars, ...)
{
	va_list ap;

	va_start (ap, max_chars);
	_msg_vprompt(msg_string(msg_no), 1, def, val, max_chars, ap);
	va_end(ap);
}

void
msg_prompt_noecho(msg msg_no, const char *def, char *val, int max_chars, ...)
{
	va_list ap;

	msg_clear();

	va_start (ap, max_chars);
	_msg_vprompt(msg_string(msg_no), 0, def, val, max_chars, ap);
	va_end (ap);
}

void msg_table_add(msg msg_no, ...)
{
	va_list ap;

	va_start (ap, msg_no);
	(void)_msg_vprintf(0, msg_string(msg_no), ap);
	va_end (ap);
}

