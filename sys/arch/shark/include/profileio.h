/*	$NetBSD: profileio.h,v 1.1 2002/02/10 01:57:37 thorpej Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * Remote profiler structures used to communicate between the 
 * target (SHARK) and the host (GUI'd) machine.
 * Also has stuff used to talk between the profiling driver and 
 * profiling server function.
 *
 */

#ifndef __PROFILE_IO_H__
#define __PROFILE_IO_H__

#include <sys/types.h>

/* I have no idea what the 'P' group id means, 
 * I presume it isn't used for much.??
 */
#define PROFIOSTART	_IOWR('P', 0, struct profStartInfo) /* start profiling */
#define PROFIOSTOP	_IO('P', 1)	/* stop profiling  */

/* hash table stuff. 
 */
#define TABLE_ENTRY_SIZE (sizeof(struct hashEntry))
#define REDUNDANT_BITS   0x02
#define COUNT_BITS       0x02
#define COUNT_BIT_MASK   0x03

/* sample mode flags.
 */
#define SAMPLE_MODE_MASK 0x03
#define SAMPLE_PROC      0x01
#define SAMPLE_KERN      0x02

/* an actual entry
 */
struct profHashEntry 
{
    unsigned int pc;          /* the pc, minus any redundant bits. */
    unsigned int next;        /* the next pointer as an entry index */
    unsigned short counts[4]; /* the counts */
};

/* table header.
 */
struct profHashHeader
{
    unsigned int tableSize;     /* total table size in entries */
    unsigned int entries;       /* first level table size, in entries */
    unsigned int samples;       /* the number of samples in the table. */
    unsigned int missed;        /* the number of samples missed. */
    unsigned int fiqs;          /* the number of fiqs. */
    unsigned int last;          /* last entry in the overflow area */
    pid_t pid;                  /* The pid being sampled */
    int mode;                   /* kernel or user mode */
}
__attribute__ ((packed));

/* actual table
 */
struct profHashTable
{
    struct profHashHeader hdr;
    struct profHashEntry *entries;
};

/* information passed to the start/stop ioctl.
 */
struct profStartInfo
{
    pid_t pid;                  /* if this is -1 sample no processes */
    unsigned int tableSize;     /* the total table size in entries */
    unsigned int entries;       /* number of entries to hash */
    unsigned int mode;          /* if set profile the kernel */
    int status;                 /* status of command returned by driver. */
};


/* Communications Protocol stuff 
 * defines the messages that the host and
 * target will use to communicate.
 */

struct packetHeader
{
    int code;        /* this will either be a command or a
				* data specifier.
				*/
    unsigned int size;         /* size of data to follow, 
				* quantity depends on code. 
				*/
} 
__attribute__ ((packed));

struct startSamplingCommand
{
    pid_t pid;              /* the pid to sample */
    unsigned int tableSize; /* the total table size in entries */
    unsigned int entries;   /* number of entries to hash */
    unsigned int mode;     /* if set profile kernel also. */
} 
__attribute__ ((packed));

struct startSamplingResponse
{
    int status;          /* identifies the status of the command. */
    int count;           /* number of shared lib entries following. */
}
__attribute__ ((packed));

struct stopSamplingCommand
{
    int alert;          /* if set then the daemon sends a SIGINT to 
			 * the process.
			 */
}
__attribute__ ((packed));

struct disassemble
{
    unsigned int offset; /* offset into file to begin disassembling */
    unsigned int length; /* length in arm words ie 32bits. */
} 
__attribute__ ((packed));

struct profStatus
{
    unsigned int status;    /* identifies the status. */
}
__attribute__ ((packed));


/* Command/Data Types 
 * Only one bit may be set for any one command.
 * so these are not masks but distinct values.
 */
#define START_SAMPLING 0x01
#define STOP_SAMPLING  0x02
#define READ_TABLE     0x03
#define DISASSEMBLY    0x04
#define SYMBOL_INFO    0x05
#define PROCESS_INFO   0x06

/* Data Types */
#define TABLE_DATA     0x07
#define SYMBOL_DATA    0x08
#define DISAS_DATA     0x09
#define PROC_DATA      0x0a
#define STATUS_DATA    0x0b
#define START_DATA     0x0c

/* Status Codes */
#define CMD_OK           0x00
#define ALREADY_SAMPLING 0x01
#define NOT_SAMPLING     0x02
#define NO_MEMORY        0x03
#define BAD_TABLE_SIZE   0x04
#define ILLEGAL_PACKET   0x05
#define ILLEGAL_COMMAND  0x06
#define BAD_OPTION       0x07

#endif
