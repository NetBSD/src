/*	$NetBSD: rf_disks.c,v 1.6 1999/02/24 00:00:03 oster Exp $	*/
/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Greg Oster
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Mark Holland
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/***************************************************************
 * rf_disks.c -- code to perform operations on the actual disks
 ***************************************************************/

#include "rf_types.h"
#include "rf_raid.h"
#include "rf_alloclist.h"
#include "rf_utils.h"
#include "rf_configure.h"
#include "rf_general.h"
#include "rf_options.h"
#include "rf_sys.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>

/* XXX these should be in a header file somewhere */
int raidlookup __P((char *, struct proc * p, struct vnode **));
int raidwrite_component_label(dev_t, struct vnode *, RF_ComponentLabel_t *);
int raidread_component_label(dev_t, struct vnode *, RF_ComponentLabel_t *);
void rf_UnconfigureVnodes( RF_Raid_t * );
int rf_CheckLabels( RF_Raid_t *, RF_Config_t *);

#define DPRINTF6(a,b,c,d,e,f) if (rf_diskDebug) printf(a,b,c,d,e,f)
#define DPRINTF7(a,b,c,d,e,f,g) if (rf_diskDebug) printf(a,b,c,d,e,f,g)

/**************************************************************************
 *
 * initialize the disks comprising the array
 *
 * We want the spare disks to have regular row,col numbers so that we can 
 * easily substitue a spare for a failed disk.  But, the driver code assumes 
 * throughout that the array contains numRow by numCol _non-spare_ disks, so 
 * it's not clear how to fit in the spares.  This is an unfortunate holdover
 * from raidSim.  The quick and dirty fix is to make row zero bigger than the 
 * rest, and put all the spares in it.  This probably needs to get changed 
 * eventually.
 *
 **************************************************************************/

int 
rf_ConfigureDisks( listp, raidPtr, cfgPtr )
	RF_ShutdownList_t **listp;
	RF_Raid_t *raidPtr;
	RF_Config_t *cfgPtr;
{
	RF_RaidDisk_t **disks;
	RF_SectorCount_t min_numblks = (RF_SectorCount_t) 0x7FFFFFFFFFFFLL;
	RF_RowCol_t r, c;
	int bs, ret;
	unsigned i, count, foundone = 0, numFailuresThisRow;
	int num_rows_done, num_cols_done;

	num_rows_done = 0;
	num_cols_done = 0;

	RF_CallocAndAdd(disks, raidPtr->numRow, sizeof(RF_RaidDisk_t *), 
			(RF_RaidDisk_t **), raidPtr->cleanupList);
	if (disks == NULL) {
		ret = ENOMEM;
		goto fail;
	}
	raidPtr->Disks = disks;

	/* get space for the device-specific stuff... */
	RF_CallocAndAdd(raidPtr->raid_cinfo, raidPtr->numRow,
	    sizeof(struct raidcinfo *), (struct raidcinfo **),
	    raidPtr->cleanupList);
	if (raidPtr->raid_cinfo == NULL) {
		ret = ENOMEM;
		goto fail;
	}
	for (r = 0; r < raidPtr->numRow; r++) {
		numFailuresThisRow = 0;
		/* We allocate RF_MAXSPARE on the first row so that we
		   have room to do hot-swapping of spares */
		RF_CallocAndAdd(disks[r], raidPtr->numCol 
				+ ((r == 0) ? RF_MAXSPARE : 0), 
				sizeof(RF_RaidDisk_t), (RF_RaidDisk_t *), 
				raidPtr->cleanupList);
		if (disks[r] == NULL) {
			ret = ENOMEM;
			goto fail;
		}
		/* get more space for device specific stuff.. */
		RF_CallocAndAdd(raidPtr->raid_cinfo[r],
		    raidPtr->numCol + ((r == 0) ? raidPtr->numSpare : 0),
		    sizeof(struct raidcinfo), (struct raidcinfo *),
		    raidPtr->cleanupList);
		if (raidPtr->raid_cinfo[r] == NULL) {
			ret = ENOMEM;
			goto fail;
		}
		for (c = 0; c < raidPtr->numCol; c++) {
			ret = rf_ConfigureDisk(raidPtr, 
					       &cfgPtr->devnames[r][c][0],
					       &disks[r][c], r, c);
			if (ret)
				goto fail;
#ifdef NOT_YET_BOYS_AND_GIRLS
			if (disks[r][c].status == rf_ds_optimal) {
				raidread_component_label(
					 raidPtr->raid_cinfo[r][c].ci_dev,
					 raidPtr->raid_cinfo[r][c].ci_vp,
					 &raidPtr->raid_cinfo[r][c].ci_label);
			}
#endif
			if (disks[r][c].status != rf_ds_optimal) {
				numFailuresThisRow++;
			} else {
				if (disks[r][c].numBlocks < min_numblks)
					min_numblks = disks[r][c].numBlocks;
				DPRINTF7("Disk at row %d col %d: dev %s numBlocks %ld blockSize %d (%ld MB)\n",
				    r, c, disks[r][c].devname,
				    (long int) disks[r][c].numBlocks,
				    disks[r][c].blockSize,
				    (long int) disks[r][c].numBlocks *
					 disks[r][c].blockSize / 1024 / 1024);
			}
			num_cols_done++;
		}
		/* XXX fix for n-fault tolerant */
		/* XXX this should probably check to see how many failures
		   we can handle for this configuration! */
		if (numFailuresThisRow > 0)
			raidPtr->status[r] = rf_rs_degraded;
		num_rows_done++;
	}

	/* all disks must be the same size & have the same block size, bs must
	 * be a power of 2 */
	bs = 0;
	for (foundone = r = 0; !foundone && r < raidPtr->numRow; r++) {
		for (c = 0; !foundone && c < raidPtr->numCol; c++) {
			if (disks[r][c].status == rf_ds_optimal) {
				bs = disks[r][c].blockSize;
				foundone = 1;
			}
		}
	}
	if (!foundone) {
		RF_ERRORMSG("RAIDFRAME: Did not find any live disks in the array.\n");
		ret = EINVAL;
		goto fail;
	}
	for (count = 0, i = 1; i; i <<= 1)
		if (bs & i)
			count++;
	if (count != 1) {
		RF_ERRORMSG1("Error: block size on disks (%d) must be a power of 2\n", bs);
		ret = EINVAL;
		goto fail;
	}

#if NOT_YET_BOYS_AND_GIRLS
	if (rf_CheckLabels( raidPtr, cfgPtr )) {
		printf("There were fatal errors (ignored for now)\n");
	}
#endif
	for (r = 0; r < raidPtr->numRow; r++) {
		for (c = 0; c < raidPtr->numCol; c++) {
			if (disks[r][c].status == rf_ds_optimal) {
				if (disks[r][c].blockSize != bs) {
					RF_ERRORMSG2("Error: block size of disk at r %d c %d different from disk at r 0 c 0\n", r, c);
					ret = EINVAL;
					goto fail;
				}
				if (disks[r][c].numBlocks != min_numblks) {
					RF_ERRORMSG3("WARNING: truncating disk at r %d c %d to %d blocks\n",
					    r, c, (int) min_numblks);
					disks[r][c].numBlocks = min_numblks;
				}
			}
		}
	}

	raidPtr->sectorsPerDisk = min_numblks;
	raidPtr->logBytesPerSector = ffs(bs) - 1;
	raidPtr->bytesPerSector = bs;
	raidPtr->sectorMask = bs - 1;
	return (0);

fail:
	
	rf_UnconfigureVnodes( raidPtr );

	return (ret);
}


/****************************************************************************
 * set up the data structures describing the spare disks in the array
 * recall from the above comment that the spare disk descriptors are stored
 * in row zero, which is specially expanded to hold them.
 ****************************************************************************/
int 
rf_ConfigureSpareDisks( listp, raidPtr, cfgPtr )
	RF_ShutdownList_t ** listp;
	RF_Raid_t * raidPtr;
	RF_Config_t * cfgPtr;
{
	int     i, ret;
	unsigned int bs;
	RF_RaidDisk_t *disks;
	int     num_spares_done;

	num_spares_done = 0;

	/* The space for the spares should have already been allocated by
	 * ConfigureDisks() */

	disks = &raidPtr->Disks[0][raidPtr->numCol];
	for (i = 0; i < raidPtr->numSpare; i++) {
		ret = rf_ConfigureDisk(raidPtr, &cfgPtr->spare_names[i][0],
				       &disks[i], 0, raidPtr->numCol + i);
		if (ret)
			goto fail;
		if (disks[i].status != rf_ds_optimal) {
			RF_ERRORMSG1("Warning: spare disk %s failed TUR\n", 
				     &cfgPtr->spare_names[i][0]);
		} else {
			disks[i].status = rf_ds_spare;	/* change status to
							 * spare */
			DPRINTF6("Spare Disk %d: dev %s numBlocks %ld blockSize %d (%ld MB)\n", i,
			    disks[i].devname,
			    (long int) disks[i].numBlocks, disks[i].blockSize,
			    (long int) disks[i].numBlocks * 
				 disks[i].blockSize / 1024 / 1024);
		}
		num_spares_done++;
	}

	/* check sizes and block sizes on spare disks */
	bs = 1 << raidPtr->logBytesPerSector;
	for (i = 0; i < raidPtr->numSpare; i++) {
		if (disks[i].blockSize != bs) {
			RF_ERRORMSG3("Block size of %d on spare disk %s is not the same as on other disks (%d)\n", disks[i].blockSize, disks[i].devname, bs);
			ret = EINVAL;
			goto fail;
		}
		if (disks[i].numBlocks < raidPtr->sectorsPerDisk) {
			RF_ERRORMSG3("Spare disk %s (%d blocks) is too small to serve as a spare (need %ld blocks)\n",
				     disks[i].devname, disks[i].blockSize, 
				     (long int) raidPtr->sectorsPerDisk);
			ret = EINVAL;
			goto fail;
		} else
			if (disks[i].numBlocks > raidPtr->sectorsPerDisk) {
				RF_ERRORMSG2("Warning: truncating spare disk %s to %ld blocks\n", disks[i].devname, (long int) raidPtr->sectorsPerDisk);

				disks[i].numBlocks = raidPtr->sectorsPerDisk;
			}
	}

	return (0);

fail:

	/* Release the hold on the main components.  We've failed to allocate
	 * a spare, and since we're failing, we need to free things.. 
		 
	 XXX failing to allocate a spare is *not* that big of a deal... 
	 We *can* survive without it, if need be, esp. if we get hot
	 adding working.  

	 If we don't fail out here, then we need a way to remove this spare... 
	 that should be easier to do here than if we are "live"... 

	 */

	rf_UnconfigureVnodes( raidPtr );

	return (ret);
}



/* configure a single disk in the array */
int 
rf_ConfigureDisk(raidPtr, buf, diskPtr, row, col)
	RF_Raid_t *raidPtr;
	char   *buf;
	RF_RaidDisk_t *diskPtr;
	RF_RowCol_t row;
	RF_RowCol_t col;
{
	char   *p;
	int     retcode;

	struct partinfo dpart;
	struct vnode *vp;
	struct vattr va;
	struct proc *proc;
	int     error;

	retcode = 0;
	p = rf_find_non_white(buf);
	if (p[strlen(p) - 1] == '\n') {
		/* strip off the newline */
		p[strlen(p) - 1] = '\0';
	}
	(void) strcpy(diskPtr->devname, p);

	proc = raidPtr->proc;	/* XXX Yes, this is not nice.. */

	/* Let's start by claiming the component is fine and well... */
	diskPtr->status = rf_ds_optimal;

	raidPtr->raid_cinfo[row][col].ci_vp = NULL;
	raidPtr->raid_cinfo[row][col].ci_dev = NULL;

	error = raidlookup(diskPtr->devname, proc, &vp);
	if (error) {
		printf("raidlookup on device: %s failed!\n", diskPtr->devname);
		if (error == ENXIO) {
			/* the component isn't there... must be dead :-( */
			diskPtr->status = rf_ds_failed;
		} else {
			return (error);
		}
	}
	if (diskPtr->status == rf_ds_optimal) {

		if ((error = VOP_GETATTR(vp, &va, proc->p_ucred, proc)) != 0) {
			return (error);
		}
		error = VOP_IOCTL(vp, DIOCGPART, (caddr_t) & dpart,
				  FREAD, proc->p_ucred, proc);
		if (error) {
			return (error);
		}
		
		diskPtr->blockSize = dpart.disklab->d_secsize;
		
		diskPtr->numBlocks = dpart.part->p_size - rf_protectedSectors;
		
		raidPtr->raid_cinfo[row][col].ci_vp = vp;
		raidPtr->raid_cinfo[row][col].ci_dev = va.va_rdev;
		
		diskPtr->dev = va.va_rdev;
		
		/* we allow the user to specify that only a fraction of the
		 * disks should be used this is just for debug:  it speeds up
		 * the parity scan */
		diskPtr->numBlocks = diskPtr->numBlocks * 
			rf_sizePercentage / 100;
	}
	return (0);
}

/* 

   rf_CheckLabels() - check all the component labels for consistency.
   Return an error if there is anything major amiss.

 */

int 
rf_CheckLabels( raidPtr, cfgPtr )
	RF_Raid_t *raidPtr;
	RF_Config_t *cfgPtr;
{
	int r,c;
	char *dev_name;
	RF_ComponentLabel_t *ci_label;
	int version = 0;
	int serial_number = 0;
	int mod_counter = 0;
	int fatal_error = 0;
	int disk_num = 0;

	for (r = 0; r < raidPtr->numRow; r++) {
		for (c = 0; c < raidPtr->numCol; c++) {
			dev_name = &cfgPtr->devnames[r][c][0];
			ci_label = &raidPtr->raid_cinfo[r][c].ci_label;
			
			printf("Component label for %s being configured at row: %d col: %d\n", dev_name, r, c );
			printf("         Row: %d Column: %d Num Rows: %d Num Columns: %d\n", 			       ci_label->row, ci_label->column, 
			       ci_label->num_rows, ci_label->num_columns);
			printf("         Version: %d Serial Number: %d Clean: %d Status: %d\n", ci_label->version, ci_label->serial_number,
			       ci_label->clean, ci_label->status );
			
			if ( r !=0 && c != 0) {
				if (serial_number != ci_label->serial_number) {
					printf("%s has a different %s\n", 
					       dev_name, "serial number!");
					fatal_error = 1;
				}
				if (version != ci_label->version) {
					printf("%s has a different %s\n",
					       dev_name, "version!");
					fatal_error = 1;
				}
				if (mod_counter != ci_label->mod_counter) {
					printf("%s has a different modfication count!\n",dev_name);
				}
			} else { 
				serial_number = ci_label->serial_number;
				version = ci_label->version;
				mod_counter = ci_label->mod_counter;
			}
							
			if (r != ci_label->row) {
				printf("Row out of alignment for: %s\n", 
				       dev_name); 
				fatal_error = 1;
			}
			if (c != ci_label->column) {
				printf("Column out of alignment for: %s\n", 
				       dev_name);
				fatal_error = 1;
			}
			if (raidPtr->numRow != ci_label->num_rows) {
				printf("Number of rows do not match for: %s\n", 
				       dev_name);
				fatal_error = 1;
			}
			if (raidPtr->numCol != ci_label->num_columns) {
				printf("Number of columns do not match for: %s\n",
				       dev_name);
				fatal_error = 1;
			}
			if (ci_label->clean == 0) {
				/* it's not clean, but it's not fatal */
				printf("%s is not clean!\n", dev_name);
			}
			disk_num++;
		}
	}

	return(fatal_error);	
}


int rf_add_hot_spare(RF_Raid_t *, RF_HotSpare_t *);
int
rf_add_hot_spare(raidPtr, sparePtr)
	RF_Raid_t *raidPtr;
	RF_HotSpare_t *sparePtr;
{
	RF_RaidDisk_t *disks;
	int ret;
	unsigned int bs;
	int spare_number;

	printf("Just in rf_add_hot_spare: %d\n",raidPtr->numSpare);
	printf("Num col: %d\n",raidPtr->numCol);
	if (raidPtr->numSpare >= RF_MAXSPARE) {
		RF_ERRORMSG1("Too many spares: %d\n", raidPtr->numSpare);
		return(EINVAL);
	}
	
	/* the beginning of the spares... */
	disks = &raidPtr->Disks[0][raidPtr->numCol];

	spare_number = raidPtr->numSpare;

	ret = rf_ConfigureDisk(raidPtr, sparePtr->spare_name,
			       &disks[spare_number], 0,
			       raidPtr->numCol + spare_number);

	if (ret)
		goto fail;
	if (disks[spare_number].status != rf_ds_optimal) {
		RF_ERRORMSG1("Warning: spare disk %s failed TUR\n", 
			     sparePtr->spare_name);
		ret=EINVAL;
		goto fail;
	} else {
		disks[spare_number].status = rf_ds_spare;
		DPRINTF6("Spare Disk %d: dev %s numBlocks %ld blockSize %d (%ld MB)\n", spare_number,
			 disks[spare_number].devname,
			 (long int) disks[spare_number].numBlocks, 
			 disks[spare_number].blockSize,
			 (long int) disks[spare_number].numBlocks * 
			 disks[spare_number].blockSize / 1024 / 1024);
	}
	

	/* check sizes and block sizes on the spare disk */
	bs = 1 << raidPtr->logBytesPerSector;
	if (disks[spare_number].blockSize != bs) {
		RF_ERRORMSG3("Block size of %d on spare disk %s is not the same as on other disks (%d)\n", disks[spare_number].blockSize, disks[spare_number].devname, bs);
		ret = EINVAL;
		goto fail;
	}
	if (disks[spare_number].numBlocks < raidPtr->sectorsPerDisk) {
		RF_ERRORMSG3("Spare disk %s (%d blocks) is too small to serve as a spare (need %ld blocks)\n",
			     disks[spare_number].devname, 
			     disks[spare_number].blockSize, 
			     (long int) raidPtr->sectorsPerDisk);
		ret = EINVAL;
		goto fail;
	} else {
		if (disks[spare_number].numBlocks > 
		    raidPtr->sectorsPerDisk) {
			RF_ERRORMSG2("Warning: truncating spare disk %s to %ld blocks\n", disks[spare_number].devname, 
				     (long int) raidPtr->sectorsPerDisk);
			
			disks[spare_number].numBlocks = raidPtr->sectorsPerDisk;
		}
	}

	raidPtr->numSpare++;

	return (0);

fail:
	return(ret);
}

int
rf_remove_hot_spare(raidPtr,sparePtr)
	RF_Raid_t *raidPtr;
	RF_HotSpare_t *sparePtr;
{
	int spare_number;


	if (raidPtr->numSpare==0) {
		printf("No spares to remove!\n");
		return(EINVAL);
	}

	spare_number = sparePtr->spare_number;

	return(EINVAL); /* XXX not implemented yet */
#if 0
	if (spare_number < 0 || spare_number > raidPtr->numSpare) {
		return(EINVAL);
	}

	/* verify that this spare isn't in use... */




	/* it's gone.. */

	raidPtr->numSpare--;

	return(0);
#endif
}


