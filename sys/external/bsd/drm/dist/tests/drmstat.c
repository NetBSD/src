/* drmstat.c -- DRM device status and testing program
 * Created: Tue Jan  5 08:19:24 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <getopt.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include "xf86drm.h"

int sigio_fd;

static double usec(struct timeval *end, struct timeval *start)
{
    double e = end->tv_sec   * 1000000 + end->tv_usec;
    double s = start->tv_sec * 1000000 + start->tv_usec;

    return e - s;
}

static void getversion(int fd)
{
    drmVersionPtr version;
    
    version = drmGetVersion(fd);
    if (version) {
	printf( "Name: %s\n", version->name ? version->name : "?" );
	printf( "    Version: %d.%d.%d\n",
		version->version_major,
		version->version_minor,
		version->version_patchlevel );
	printf( "    Date: %s\n", version->date ? version->date : "?" );
	printf( "    Desc: %s\n", version->desc ? version->desc : "?" );
	drmFreeVersion(version);
    } else {
	printf( "No driver available\n" );
    }
}
	
void handler(int fd, void *oldctx, void *newctx)
{
    printf("Got fd %d\n", fd);
}

void process_sigio(char *device)
{
    int              fd;

    if ((fd = open(device, 0)) < 0) {
	drmError(-errno, __FUNCTION__);
	exit(1);
    }

    sigio_fd = fd;
    /*  drmInstallSIGIOHandler(fd, handler); */
    for (;;) sleep(60);
}

int main(int argc, char **argv)
{
    int            c;
    int            r  = 0;
    int            fd = -1;
    drm_handle_t      handle;
    void           *address;
    char           *pt;
    unsigned long  count;
    unsigned long  offset;
    unsigned long  size;
    drm_context_t  context;
    int            loops;
    char           buf[1024];
    int            i;
    drmBufInfoPtr  info;
    drmBufMapPtr   bufs;
    drmLockPtr     lock;
    int            secs;

    while ((c = getopt(argc, argv,
		       "lc:vo:O:f:s:w:W:b:r:R:P:L:C:XS:B:F:")) != EOF)
	switch (c) {
	case 'F':
	    count  = strtoul(optarg, NULL, 0);
	    if (!fork()) {
		dup(fd);
		sleep(count);
	    }
	    close(fd);
	    break;
	case 'v': getversion(fd);                                        break;
	case 'X':
	    if ((r = drmCreateContext(fd, &context))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    printf( "Got %d\n", context);
	    break;
	case 'S':
	    process_sigio(optarg);
	    break;
	case 'C':
	    if ((r = drmSwitchToContext(fd, strtoul(optarg, NULL, 0)))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    break;
	case 'c':
	    if ((r = drmSetBusid(fd,optarg))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    break;
	case 'o':
	    if ((fd = drmOpen(optarg, NULL)) < 0) {
		drmError(fd, argv[0]);
		return 1;
	    }
	    break;
	case 'O':
	    if ((fd = drmOpen(NULL, optarg)) < 0) {
		drmError(fd, argv[0]);
		return 1;
	    }
	    break;
	case 'B':		/* Test buffer allocation */
	    count  = strtoul(optarg, &pt, 0);
	    size   = strtoul(pt+1, &pt, 0);
	    secs   = strtoul(pt+1, NULL, 0);
	    {
		drmDMAReq      dma;
		int            *indices, *sizes;

		indices = alloca(sizeof(*indices) * count);
		sizes   = alloca(sizeof(*sizes)   * count);
		dma.context         = context;
		dma.send_count      = 0;
		dma.request_count   = count;
		dma.request_size    = size;
		dma.request_list    = indices;
		dma.request_sizes   = sizes;
		dma.flags           = DRM_DMA_WAIT;
		if ((r = drmDMA(fd, &dma))) {
		    drmError(r, argv[0]);
		    return 1;
		}
		for (i = 0; i < dma.granted_count; i++) {
		    printf("%5d: index = %d, size = %d\n",
			   i, dma.request_list[i], dma.request_sizes[i]);
		}
		sleep(secs);
		drmFreeBufs(fd, dma.granted_count, indices);
	    }
	    break;
	case 'b':
	    count   = strtoul(optarg, &pt, 0);
	    size    = strtoul(pt+1, NULL, 0);
	    if ((r = drmAddBufs(fd, count, size, 0, 65536)) < 0) {
		drmError(r, argv[0]);
		return 1;
	    }
	    if (!(info = drmGetBufInfo(fd))) {
		drmError(0, argv[0]);
		return 1;
	    }
	    for (i = 0; i < info->count; i++) {
		printf("%5d buffers of size %6d (low = %d, high = %d)\n",
		       info->list[i].count,
		       info->list[i].size,
		       info->list[i].low_mark,
		       info->list[i].high_mark);
	    }
	    if ((r = drmMarkBufs(fd, 0.50, 0.80))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    if (!(info = drmGetBufInfo(fd))) {
		drmError(0, argv[0]);
		return 1;
	    }
	    for (i = 0; i < info->count; i++) {
		printf("%5d buffers of size %6d (low = %d, high = %d)\n",
		       info->list[i].count,
		       info->list[i].size,
		       info->list[i].low_mark,
		       info->list[i].high_mark);
	    }
	    printf("===== /proc/dri/0/mem =====\n");
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/mem");
	    system(buf);
#if 1
	    if (!(bufs = drmMapBufs(fd))) {
		drmError(0, argv[0]);
		return 1;
	    }
	    printf("===============================\n");
	    printf( "%d bufs\n", bufs->count);
	    for (i = 0; i < bufs->count; i++) {
		printf( "  %4d: %8d bytes at %p\n",
			i,
			bufs->list[i].total,
			bufs->list[i].address);
	    }
	    printf("===== /proc/dri/0/vma =====\n");
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/vma");
	    system(buf);
#endif
	    break;
	case 'f':
	    offset  = strtoul(optarg, &pt, 0);
	    size    = strtoul(pt+1, NULL, 0);
	    handle  = 0;
	    if ((r = drmAddMap(fd, offset, size,
			       DRM_FRAME_BUFFER, 0, &handle))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    printf("0x%08lx:0x%04lx added\n", offset, size);
	    printf("===== /proc/dri/0/mem =====\n");
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/mem");
	    system(buf);
	    break;
	case 'r':
	case 'R':
	    offset  = strtoul(optarg, &pt, 0);
	    size    = strtoul(pt+1, NULL, 0);
	    handle  = 0;
	    if ((r = drmAddMap(fd, offset, size,
			       DRM_REGISTERS,
			       c == 'R' ? DRM_READ_ONLY : 0,
			       &handle))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    printf("0x%08lx:0x%04lx added\n", offset, size);
	    printf("===== /proc/dri/0/mem =====\n");
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/mem");
	    system(buf);
	    break;
	case 's':
	    size = strtoul(optarg, &pt, 0);
	    handle = 0;
	    if ((r = drmAddMap(fd, 0, size,
			       DRM_SHM, DRM_CONTAINS_LOCK,
			       &handle))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    printf("0x%04lx byte shm added at 0x%08lx\n", size, handle);
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/vm");
	    system(buf);
	    break;
	case 'P':
	    offset  = strtoul(optarg, &pt, 0);
	    size    = strtoul(pt+1, NULL, 0);
	    address = NULL;
	    if ((r = drmMap(fd, offset, size, &address))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    printf("0x%08lx:0x%04lx mapped at %p for pid %d\n",
		   offset, size, address, getpid());
	    printf("===== /proc/dri/0/vma =====\n");
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/vma");
	    system(buf);
	    mprotect((void *)offset, size, PROT_READ);
	    printf("===== /proc/dri/0/vma =====\n");
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/vma");
	    system(buf);
	    break;
	case 'w':
	case 'W':
	    offset  = strtoul(optarg, &pt, 0);
	    size    = strtoul(pt+1, NULL, 0);
	    address = NULL;
	    if ((r = drmMap(fd, offset, size, &address))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    printf("0x%08lx:0x%04lx mapped at %p for pid %d\n",
		   offset, size, address, getpid());
	    printf("===== /proc/%d/maps =====\n", getpid());
	    snprintf(buf, sizeof(buf), "cat /proc/%d/maps", getpid());
	    system(buf);
	    printf("===== /proc/dri/0/mem =====\n");
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/mem");
	    system(buf);
	    printf("===== /proc/dri/0/vma =====\n");
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/vma");
	    system(buf);
	    printf("===== READING =====\n");
	    for (i = 0; i < 0x10; i++)
		printf("%02x ", (unsigned int)((unsigned char *)address)[i]);
	    printf("\n");
	    if (c == 'w') {
		printf("===== WRITING =====\n");
		for (i = 0; i < size; i+=2) {
		    ((char *)address)[i]   = i & 0xff;
		    ((char *)address)[i+1] = i & 0xff;
		}
	    }
	    printf("===== READING =====\n");
	    for (i = 0; i < 0x10; i++)
		printf("%02x ", (unsigned int)((unsigned char *)address)[i]);
	    printf("\n");
	    printf("===== /proc/dri/0/vma =====\n");
	    snprintf(buf, sizeof(buf), "cat /proc/dri/0/vma");
	    system(buf);
	    break;
	case 'L':
	    context = strtoul(optarg, &pt, 0);
	    offset  = strtoul(pt+1, &pt, 0);
	    size    = strtoul(pt+1, &pt, 0);
	    loops   = strtoul(pt+1, NULL, 0);
	    address = NULL;
	    if ((r = drmMap(fd, offset, size, &address))) {
		drmError(r, argv[0]);
		return 1;
	    }
	    lock       = address;
#if 1
	    {
		int            counter = 0;
		struct timeval loop_start, loop_end;
		struct timeval lock_start, lock_end;
		double         wt;
#define HISTOSIZE 9
		int            histo[HISTOSIZE];
		int            output = 0;
		int            fast   = 0;

		if (loops < 0) {
		    loops = -loops;
		    ++output;
		}

		for (i = 0; i < HISTOSIZE; i++) histo[i] = 0;

		gettimeofday(&loop_start, NULL);
		for (i = 0; i < loops; i++) {
		    gettimeofday(&lock_start, NULL);
		    DRM_LIGHT_LOCK_COUNT(fd,lock,context,fast);
		    gettimeofday(&lock_end, NULL);
		    DRM_UNLOCK(fd,lock,context);
		    ++counter;
		    wt = usec(&lock_end, &lock_start);
		    if      (wt <=      2.5) ++histo[8];
		    if      (wt <       5.0) ++histo[0];
		    else if (wt <      50.0) ++histo[1];
		    else if (wt <     500.0) ++histo[2];
		    else if (wt <    5000.0) ++histo[3];
		    else if (wt <   50000.0) ++histo[4];
		    else if (wt <  500000.0) ++histo[5];
		    else if (wt < 5000000.0) ++histo[6];
		    else                     ++histo[7];
		    if (output) printf( "%.2f uSec, %d fast\n", wt, fast);
		}
		gettimeofday(&loop_end, NULL);
		printf( "Average wait time = %.2f usec, %d fast\n",
			usec(&loop_end, &loop_start) /  counter, fast);
		printf( "%9d <=     2.5 uS\n", histo[8]);
		printf( "%9d <        5 uS\n", histo[0]);
		printf( "%9d <       50 uS\n", histo[1]);
		printf( "%9d <      500 uS\n", histo[2]);
		printf( "%9d <     5000 uS\n", histo[3]);
		printf( "%9d <    50000 uS\n", histo[4]);
		printf( "%9d <   500000 uS\n", histo[5]);
		printf( "%9d <  5000000 uS\n", histo[6]);
		printf( "%9d >= 5000000 uS\n", histo[7]);
	    }
#else
	    printf( "before lock: 0x%08x\n", lock->lock);
	    printf( "lock: 0x%08x\n", lock->lock);
	    sleep(5);
	    printf( "unlock: 0x%08x\n", lock->lock);
#endif
	    break;
	default:
	    fprintf( stderr, "Usage: drmstat [options]\n" );
	    return 1;
	}

    return r; 
}

void
xf86VDrvMsgVerb(int scrnIndex, int type, int verb, const char *format,
                va_list args)
{
	vfprintf(stderr, format, args);
}

int xf86ConfigDRI[10];
