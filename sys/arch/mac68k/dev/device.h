/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: device.h,v 1.2 1993/11/29 00:32:29 briggs Exp $
 */

#include "nubus.h"

struct driver {
	int	(*d_init)();
	char	*d_name;
	int	(*d_start)();
	int	(*d_go)();
	int	(*d_intr)();
	int	(*d_done)();
};


struct mac_ctlr {
	struct driver	*mac_driver;
	int		mac_unit;
	int		mac_alive;
	char		*mac_addr;
	int		mac_flags;
	int		mac_ipl;
};

struct mac_device {
	struct driver	*mac_driver;
	struct driver	*mac_cdriver;
	int		mac_unit;
	int		mac_ctlr;
	int		mac_slave;
	char		*mac_addr;
	int		mac_dk;
	int		mac_flags;
	int		mac_alive;
	int		mac_ipl;
};


struct	devqueue {
	struct	devqueue *dq_forw;
	struct	devqueue *dq_back;
	int	dq_ctlr;
	int	dq_unit;
	int	dq_slave;
	struct	driver *dq_driver;
};


#define	MAXSLOTS	16	/* Size of HW table (NuBus) */
#define MAXADB		16	/* Max number of ADB devices. */



struct adb_hw{
   int found;			/* is there a device at this address */
   int addr;			/* ADB address */
   int handler;			/* handler found by talk reg 3 */
   int claimed;			/* TRUE if a driver claims this */
};


#ifdef KERNEL
extern struct nubus_hw nubus_table[];
extern struct mac_device mac_dinit[];
extern caddr_t sctova(), sctopa(), iomap();
#endif

/* LAK:  The rest of this file is my attempt at making a device driver
framework. */

/* For struct macdriver.type: */
#define TYPE_NUBUS	1
#define TYPE_ADB	2
#define TYPE_SCC	3
#define TYPE_SCSI	4
#define TYPE_VIDEO	5

struct macdriver {
  int type;				/* Type of device (see above)	*/
  int hwfound;				/* If the hardware was found	*/
  int unit;				/* Unit number (e.g., SCSI ID)	*/
  void (*init)(struct macdriver *);	/* Initialize all hardware	*/
  char *name;				/* Name of the driver		*/
};

struct scsi_subdev {
	struct macdriver *macdriver;	/* For future NuBUS scsi support */
	int		 unit;
	int		 flags;
	int		 drive;
	int		 parentunit;
};

extern struct macdriver macdriver[];	/* In ioconf.c */
extern int numdrivers;			/* In ioconf.c */
extern struct scsi_subdev scsi_subs[];	/* In ioconf.c */
extern int numscsi_subs;		/* In ioconf.c */
