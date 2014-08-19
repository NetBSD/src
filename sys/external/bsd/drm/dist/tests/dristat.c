/* dristat.c -- 
 * Created: Mon Jan 15 05:05:07 2001 by faith@acm.org
 *
 * Copyright 2000 VA Linux Systems, Inc., Fremont, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * Authors: Rickard E. (Rik) Faith <faith@valinux.com>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "xf86drm.h"
#include "xf86drmRandom.c"
#include "xf86drmHash.c"
#include "xf86drm.c"

#define DRM_VERSION 0x00000001
#define DRM_MEMORY  0x00000002
#define DRM_CLIENTS 0x00000004
#define DRM_STATS   0x00000008
#define DRM_BUSID   0x00000010

static void getversion(int fd)
{
    drmVersionPtr version;
    
    version = drmGetVersion(fd);
    if (version) {
	printf("  Version information:\n");
	printf("    Name: %s\n", version->name ? version->name : "?");
	printf("    Version: %d.%d.%d\n",
	       version->version_major,
	       version->version_minor,
	       version->version_patchlevel);
	printf("    Date: %s\n", version->date ? version->date : "?");
	printf("    Desc: %s\n", version->desc ? version->desc : "?");
	drmFreeVersion(version);
    } else {
	printf("  No version information available\n");
    }
}

static void getbusid(int fd)
{
    const char *busid = drmGetBusid(fd);
    
    printf("  Busid: %s\n", *busid ? busid : "(not set)");
    drmFreeBusid(busid);
}


static void getvm(int fd)
{
    int             i;
    const char      *typename;
    char            flagname[33];
    drm_handle_t    offset;
    drmSize         size;
    drmMapType      type;
    drmMapFlags     flags;
    drm_handle_t    handle;
    int             mtrr;

    printf("  VM map information:\n");
    printf("  flags: (R)estricted (r)ead/(w)rite (l)ocked (k)ernel (W)rite-combine (L)ock:\n");
    printf("    slot     offset       size type flags    address mtrr\n");

    for (i = 0;
	 !drmGetMap(fd, i, &offset, &size, &type, &flags, &handle, &mtrr);
	 i++) {
	
	switch (type) {
	case DRM_FRAME_BUFFER:   typename = "FB";  break;
	case DRM_REGISTERS:      typename = "REG"; break;
	case DRM_SHM:            typename = "SHM"; break;
	case DRM_AGP:            typename = "AGP"; break;
	case DRM_SCATTER_GATHER: typename = "SG";  break;
	default:                 typename = "???"; break;
	}

	flagname[0] = (flags & DRM_RESTRICTED)      ? 'R' : ' ';
	flagname[1] = (flags & DRM_READ_ONLY)       ? 'r' : 'w';
	flagname[2] = (flags & DRM_LOCKED)          ? 'l' : ' ';
	flagname[3] = (flags & DRM_KERNEL)          ? 'k' : ' ';
	flagname[4] = (flags & DRM_WRITE_COMBINING) ? 'W' : ' ';
	flagname[5] = (flags & DRM_CONTAINS_LOCK)   ? 'L' : ' ';
	flagname[6] = '\0';
	
	printf("    %4d 0x%08lx 0x%08lx %3.3s %6.6s 0x%08lx ",
	       i, offset, (unsigned long)size, typename, flagname, handle);
	if (mtrr < 0) printf("none\n");
	else          printf("%4d\n", mtrr);
    }
}

static void getclients(int fd)
{
    int           i;
    int           auth;
    int           pid;
    int           uid;
    unsigned long magic;
    unsigned long iocs;
    char          buf[64];
    char          cmd[40];
    int           procfd;
    
    printf("  DRI client information:\n");
    printf("    a   pid   uid      magic     ioctls   prog\n");

    for (i = 0; !drmGetClient(fd, i, &auth, &pid, &uid, &magic, &iocs); i++) {
	snprintf(buf, sizeof(buf), "/proc/%d/cmdline", pid);
	memset(cmd, 0, sizeof(cmd));
	if ((procfd = open(buf, O_RDONLY, 0)) >= 0) {
	    read(procfd, cmd, sizeof(cmd)-1);
	    close(procfd);
	}
	if (*cmd) {
	    char *pt;

	    for (pt = cmd; *pt; pt++) if (!isprint(*pt)) *pt = ' ';
	    printf("    %c %5d %5d %10lu %10lu   %s\n",
		   auth ? 'y' : 'n', pid, uid, magic, iocs, cmd);
	} else {
	    printf("    %c %5d %5d %10lu %10lu\n",
		   auth ? 'y' : 'n', pid, uid, magic, iocs);
	}
    }
}

static void printhuman(unsigned long value, const char *name, int mult)
{
    const char *p;
    double     f;
				/* Print width 5 number in width 6 space */
    if (value < 100000) {
	printf(" %5lu", value);
	return;
    }

    p = name;
    f = (double)value / (double)mult;
    if (f < 10.0) {
	printf(" %4.2f%c", f, *p);
	return;
    }

    p++;
    f = (double)value / (double)mult;
    if (f < 10.0) {
	printf(" %4.2f%c", f, *p);
	return;
    }
    
    p++;
    f = (double)value / (double)mult;
    if (f < 10.0) {
	printf(" %4.2f%c", f, *p);
	return;
    }
}

static void getstats(int fd, int i)
{
    drmStatsT prev, curr;
    int       j;
    double    rate;
    
    printf("  System statistics:\n");

    if (drmGetStats(fd, &prev)) return;
    if (!i) {
	for (j = 0; j < prev.count; j++) {
	    printf("    ");
	    printf(prev.data[j].long_format, prev.data[j].long_name);
	    if (prev.data[j].isvalue) printf(" 0x%08lx\n", prev.data[j].value);
	    else                      printf(" %10lu\n", prev.data[j].value);
	}
	return;
    }

    printf("    ");
    for (j = 0; j < prev.count; j++)
	if (!prev.data[j].verbose) {
	    printf(" ");
	    printf(prev.data[j].rate_format, prev.data[j].rate_name);
	}
    printf("\n");
    
    for (;;) {
	sleep(i);
	if (drmGetStats(fd, &curr)) return;
	printf("    ");
	for (j = 0; j < curr.count; j++) {
	    if (curr.data[j].verbose) continue;
	    if (curr.data[j].isvalue) {
		printf(" %08lx", curr.data[j].value);
	    } else {
		rate = (curr.data[j].value - prev.data[j].value) / (double)i;
		printhuman(rate, curr.data[j].mult_names, curr.data[j].mult);
	    }
	}
	printf("\n");
	memcpy(&prev, &curr, sizeof(prev));
    }
    
}

int main(int argc, char **argv)
{
    int  c;
    int  mask     = 0;
    int  minor    = 0;
    int  interval = 0;
    int  fd;
    char buf[64];
    int  i;

    while ((c = getopt(argc, argv, "avmcsbM:i:")) != EOF)
	switch (c) {
	case 'a': mask = ~0;                          break;
	case 'v': mask |= DRM_VERSION;                break;
	case 'm': mask |= DRM_MEMORY;                 break;
	case 'c': mask |= DRM_CLIENTS;                break;
	case 's': mask |= DRM_STATS;                  break;
	case 'b': mask |= DRM_BUSID;                  break;
	case 'i': interval = strtol(optarg, NULL, 0); break;
	case 'M': minor = strtol(optarg, NULL, 0);    break;
	default:
	    fprintf( stderr, "Usage: dristat [options]\n\n" );
	    fprintf( stderr, "Displays DRM information. Use with no arguments to display available cards.\n\n" );
	    fprintf( stderr, "  -a            Show all available information\n" );
	    fprintf( stderr, "  -b            Show DRM bus ID's\n" );
	    fprintf( stderr, "  -c            Display information about DRM clients\n" );
	    fprintf( stderr, "  -i [interval] Continuously display statistics every [interval] seconds\n" );
	    fprintf( stderr, "  -v            Display DRM module and card version information\n" );
	    fprintf( stderr, "  -m            Display memory use information\n" );
	    fprintf( stderr, "  -s            Display DRM statistics\n" );
	    fprintf( stderr, "  -M [minor]    Select card by minor number\n" );
	    return 1;
	}

    for (i = 0; i < 16; i++) if (!minor || i == minor) {
	snprintf(buf, sizeof(buf), DRM_DEV_NAME, DRM_DIR_NAME, i);
	fd = drmOpenMinor(i, 1, DRM_NODE_RENDER);
	if (fd >= 0) {
	    printf("%s\n", buf);
	    if (mask & DRM_BUSID)   getbusid(fd);
	    if (mask & DRM_VERSION) getversion(fd);
	    if (mask & DRM_MEMORY)  getvm(fd);
	    if (mask & DRM_CLIENTS) getclients(fd);
	    if (mask & DRM_STATS)   getstats(fd, interval);
	    close(fd);
	}
    }

    return 0; 
}
