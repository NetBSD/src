/*	$NetBSD: profile.c,v 1.1.10.1 2002/05/17 15:40:52 gehenna Exp $	*/

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
 * The fiq based profiler.
 */

#include "profiler.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <shark/shark/hat.h>
#include <machine/profileio.h>
#include <dev/ic/i8253reg.h>

#define PROFILER_DEBUG 1

#define countPerTick 500 /* TIMER_FREQ/10000   10 kHz timer */

/* Processor Status Defines */
#define STATUS_MODE_MASK 0x1f
#define USER_MODE        0x10
#define FIQ_MODE         0x11
#define IRQ_MODE         0x12
#define SVC_MODE         0x13
#define ABORT_MODE       0x17
#define UNDEF_MODE       0x1b
#define SYS_MODE         0x1f

/* software controller
 */
struct profiler_sc 
{
    int state;
#define PROF_OPEN 0x01
#define PROF_PROFILING 0x02
} prof_sc;

/*
 * GLOBAL DATA
 */

/* I need my own stack space for the hat */
#define HATSTACKSIZE 1024       /* size of stack used during a FIQ */
static unsigned char hatStack[HATSTACKSIZE]; /* actual stack used 
					      * during a FIQ 
					      */
/* Pointer to the list of hash tables.
 * A backup table is created for every table malloced, this
 * is used so that we don't miss samples while copying the 
 * data out. Thus the actual number of tables in the array is twice
 * what nhashTables says.
 */
struct profHashTable *profTable;
struct profHashTable *phashTables[2];
int nhashTables;

/*
 * FORWARD DECLARATIONS
 */
static void profFiq(int  x);
static void profHatWedge(int nFIQs);
void profStop(void);
void profStart(struct profStartInfo *);
static void profEnter(struct profHashTable * , unsigned int);
void displayTable(struct profHashTable * );

dev_type_open(profopen);
dev_type_close(profclose);
dev_type_read(profread);
dev_type_ioctl(profioctl);

const struct cdevsw prof_cdevsw = {
	profopen, profclose, profread, nowrite, profioctl,
	nostop, notty, nopoll, nommap,
};

void 
profilerattach(n)
    int n;
{
    /* reset the profiler state */
    prof_sc.state = 0;
}

/*
 * Open the profiling devicee.  
 * Returns 
 *       ENXIO for illegal minor device 
 *             ie. if the minor device number is not 0.
 *       EBUSY if file is open by another process.
 *       EROFS if attempt to open in write mode.
 */
int
profopen(dev, flag, mode, p)
    dev_t dev;
    int flag;
    int mode;
    struct proc *p;
{

    /* check that the minor number is correct. */
    if (minor(dev) >= NPROFILER)
    {
	return ENXIO;
    }

    /* check that the device is not already open. */
    if (prof_sc.state && PROF_OPEN)
    {
	return EBUSY;
    }

    /* check that the flag is set to read only. */
    if (!(flag && FWRITE))
    {
	return EROFS;
    }
    /* flag the device as open. */
    prof_sc.state |= PROF_OPEN; 
    nhashTables = 0;
    phashTables[0] = phashTables[1] = NULL;
    return 0;
}

/*
 * Close the descriptor.
 * 
 */
int
profclose(dev, flag, mode, p)
    dev_t dev;
    int flag;
    int mode;
    struct proc *p;
{
    /* clear the state, and stop profiling if 
     * it is happening.
     */
    profStop();
    prof_sc.state &= ~PROF_OPEN;
    return 0;
}

int
profread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
    int error;
    int real, backup;
    
    /* must be profiling to read */
    if (!(prof_sc.state & PROF_PROFILING))
    {
	error = EINVAL;
    }
    else
    {
	if (uio->uio_resid != sizeof(struct profHashHeader) + 
	    profTable->hdr.tableSize * sizeof(struct profHashEntry))
	{
	    printf("profile read size is incorrect!");
	    error = EINVAL;
	}
	else
	{
	    /* first work out which table is currently being used.
	     */
	    if (profTable == phashTables[0])
	    {
		real = 0;
		backup = 1;
	    }
	    else
	    {
		if (profTable == phashTables[1])
		{
		    real = 1;
		    backup = 0;
		}
		else
		{
		    panic("profiler lost buffer\n");
		}
	    }
	    /* now initialise the backup copy before switching over.
	     */   
	    memset(phashTables[backup]->entries, 0,
		  profTable->hdr.tableSize * sizeof(struct profHashEntry));

	    
	    /* now initialise the header */
	    phashTables[backup]->hdr.tableSize = phashTables[real]->hdr.tableSize;
	    phashTables[backup]->hdr.entries = phashTables[backup]->hdr.last 
		= phashTables[real]->hdr.entries;
	    phashTables[backup]->hdr.samples = 0;
	    phashTables[backup]->hdr.missed = 0;
	    phashTables[backup]->hdr.fiqs = 0;
	    phashTables[backup]->hdr.pid = phashTables[real]->hdr.pid;
	    phashTables[backup]->hdr.mode = phashTables[real]->hdr.mode;
	    
	    /* ok now swap over. 
	     * I don't worry about locking the fiq while I change
	     * this, at this point it won't matter which table the 
	     * fiq reads.
	     */
	    profTable = phashTables[backup];
	    
	    /* don't want to send the pointer,
	     * make assumption that table follows the header.
	     */
	    if ( (error = uiomove(phashTables[real], 
				  sizeof(struct profHashHeader), uio))
		!= 0)
	    {
		printf("uiomove failed error is %d\n", error);
	    }
	    else
	    {
		if ( (error = uiomove(phashTables[real]->entries, 
				      phashTables[real]->hdr.tableSize * 
				      sizeof(struct profHashEntry), uio))
		    != 0)
		{
		    printf("uiomove failed error is %d\n", error);
		}
	    }
	}
    }
    return error;
}

/*
 *  PROFIOSTART	  Start Profiling
 *  PROFIOSTOP	  Stop Profiling
 */
static int profcount = 0;
static int ints = 0;
int
profioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
    int error = 0;
    struct profStartInfo *info = (struct profStartInfo *) data;

    switch (cmd)
    {
	case PROFIOSTART :
	    profStart(info);
	    break;
	case PROFIOSTOP :
	    profStop();
	    break;
	default :
	    error = EINVAL;
	    break;
    }
    return error;
}

/* start profiling, returning status information in the 
 * profStartInfo structure.
 * 
 * presumes pid is running, does no checks here.
 */
void 
profStart(struct profStartInfo *info)
{
    unsigned int savedInts;
    char *buffer;

    /* can't already be sampling */
    if ( prof_sc.state & PROF_PROFILING )
    {
	info->status = ALREADY_SAMPLING;
	return ;
    }

    /* sanity check that the table sizes are logical */
    if (info->entries > info->tableSize)
    {
	info->status = BAD_TABLE_SIZE;
	return ;
    }
    
    /* now sanity check that we are sampling either the 
     * kernel or a pid or both.
     */
    if ( !(info->mode & SAMPLE_MODE_MASK) )
    {
	info->status = ILLEGAL_COMMAND;
	return ;
    }

    /* alloc two hash tables. */
    buffer  = malloc(sizeof(struct profHashTable) + 
		     info->tableSize * sizeof(struct profHashEntry), 
		     M_DEVBUF, M_NOWAIT);
    if ( (buffer == NULL) )
    {
	info->status = NO_MEMORY;
	return;
    }
    phashTables[0] = (struct profHashTable *) buffer;
    phashTables[0]->entries = (struct profHashEntry *) 
	( buffer + sizeof(struct profHashTable));

    buffer  = malloc(sizeof(struct profHashTable) + 
		     info->tableSize * sizeof(struct profHashEntry), 
		     M_DEVBUF, M_NOWAIT);
    if ( (buffer == NULL) )
    {
	free(phashTables[0], M_DEVBUF);
	info->status = NO_MEMORY;
	return;
    }
    phashTables[1] = (struct profHashTable *) buffer;
    phashTables[1]->entries = (struct profHashEntry *) 
	( buffer + sizeof(struct profHashTable));

    memset(phashTables[0]->entries, 0,
	  info->tableSize * sizeof(struct profHashEntry));
    memset(phashTables[1]->entries, 0,
	  info->tableSize * sizeof(struct profHashEntry));

    /* now initialise the header */
    profTable = phashTables[0];
    profTable->hdr.tableSize = info->tableSize;
    profTable->hdr.entries = profTable->hdr.last = info->entries;
    profTable->hdr.samples = 0;
    profTable->hdr.missed = 0;
    profTable->hdr.fiqs = 0;
    profTable->hdr.pid = info->pid;
    profTable->hdr.mode = info->mode;

    /* now let the pigeons loose. */
    savedInts = disable_interrupts(I32_bit | F32_bit);
    prof_sc.state |= PROF_PROFILING;
    hatClkOn(countPerTick,
	     profFiq,
	     (int)&prof_sc,
	     hatStack + HATSTACKSIZE - sizeof(unsigned),
	     profHatWedge);
    restore_interrupts(savedInts); 
}

void
profStop(void)
{
    unsigned int savedInts;
    int spl;

    savedInts = disable_interrupts(I32_bit | F32_bit);
    hatClkOff();
    restore_interrupts(savedInts); 

    spl = splbio();
    /* only free the buffer's if we were profiling,
     * who cares if we were not, won't alert any one.
     */
    if (prof_sc.state & PROF_PROFILING)
    {
	/* now free both buffers. */
	free(phashTables[0], M_DEVBUF);
	free(phashTables[1], M_DEVBUF);
    }
    phashTables[0] = phashTables[1] = NULL;
    prof_sc.state &= ~PROF_PROFILING;
    splx(spl);

}
    
/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      profFiq
**
**      This is what the HAT clock calls.   This call drives
**      the timeout queues, which in turn drive the state machines 
**      
**      Be very carefully when calling a timeout as the function
**      that is called may in turn do timeout/untimeout calls 
**      before returning 
**
**  FORMAL PARAMETERS:
**
**      int x       - not used 
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      a timeout may be called if it is due
**--
*/
static void 
profFiq(int  x)
{
    int i;
    int *ip;           /* the fiq stack pointer */
    unsigned int spsr, stacklr;   /* the link register, off the stack. */ 

    
    /* get the link register and see where we came from.
     * We do this by getting the stack pointer using,
     * an inline assembler instruction and then going 9 
     * words up to get the return address from the fiq.
     * 
     * NOTE: the stack will change if more local variables 
     * are added so beware of modifications to this 
     * function.
     * the fiq.S handler puts the following on the stack 
     *          stmfd	sp!, {r0-r3, lr}
     * then this function does
     *          mov     ip, sp
     *          stmfd	sp!, {r4, fp, ip, lr, pc} 
     * or some variant of this.
     *
     * instead of using sp we can use ip, the saved stack pointer
     * and be done with the chance of sp changing around on us.
     *
     * so by the time we get here we have a stack that looks like.
     * (see pg 4-23, ARM programming Techniques doco for description
     * on stm instructions.)
     *         lr-fiq  (we want this one).
     *         r3-fiq
     *         r2-fiq
     *         r1-fiq
     * ip-->   r0-fiq
     *         pc-prof
     *         lr-prof
     *         ip-prof
     *         fp-prof
     * sp-->   r4-prof
     * the sp by the time we get to it will point to r4 at the 
     * bottom of the stack. So we go 9 up to get the lr we want.
     * or even better we have ip pointing to r0 and we can go 4 up
     * to get the saved link register.
     *
     * We are safer this way because fiq.S is coded assembler, we are 
     * at the mercy of the assembler for our stack.
     *
     */
    asm("mov %0, ip" : "=r" (ip) : );
    stacklr = *(ip+4);
    
    /* get the spsr register
     */
    asm("mrs %0, spsr" : "=r" (spsr) : );

    /* now check whether we want this sample.
     * NB. We place kernel and user level samples in the 
     * same table.
     */
    if ( (profTable->hdr.mode & SAMPLE_PROC) &&
	((spsr & STATUS_MODE_MASK) == USER_MODE) )
    {
	if ( curproc->p_pid == profTable->hdr.pid )
	{
	    profEnter(profTable, stacklr-4);
	}
    }

    if ( profTable->hdr.mode & SAMPLE_KERN )
    {
	if ( ((spsr & STATUS_MODE_MASK) == SVC_MODE)/* || 
	    ((spsr & STATUS_MODE_MASK) == IRQ_MODE)*/ )
	{
	    /* Note: the link register will be two instructions,
	     * ahead of the "blamed" instruction. This is actually 
	     * a most likely case and might not actually highlight the 
	     * exact cause of problems, some post processing intelligence
	     * will be required to make use of this data.
	     */
	    profEnter(profTable, stacklr-4);
	}
    }
    /* increment the samples counter */
    profTable->hdr.fiqs++;
}

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      profHatWedge
**
**      Called if the HAT timer becomes clogged/wedged.  Not
**      used by this driver, we let upper layers recover
**      from this condition
**
**  FORMAL PARAMETERS:
**
**      int nFIQs - not used
**
**  IMPLICIT INPUTS:
**
**      nill
**
**  IMPLICIT OUTPUTS:
**
**      nill
**
**  FUNCTION VALUE:
**
**      nill
**
**  SIDE EFFECTS:
**
**      nill
**--
*/
static void 
profHatWedge(int nFIQs)
{
    #ifdef PROFILER_DEBUG
        printf("profHatWedge: nFIQ = %d\n",nFIQs);
    #endif    
}

/* Enter the data in the table.
 *
 * To reduce the time taken to find samples with time
 * an eviction algorithm is implemented.
 * When a new entry in the overflow area is required 
 * the first entry in the hash table is copied there
 * and the new entry placed as the hash table entry. The 
 * displaced entry will then be the first entry accessed in 
 * the table.
 */
static void 
profEnter(struct profHashTable *table, unsigned int lr)
{
    unsigned int entries, hashShift, index, count;
    struct profHashEntry *sample;
    struct profHashEntry *first;
    struct profHashEntry *prev;
    struct profHashEntry tmpEntry;
    int tmpIndex;

    /* work out how many bits
     * are required to hash the given size.
     */
    entries = table->hdr.entries - 1;
    hashShift = 0;
    do
    {
	entries = entries << 1;
	hashShift ++;
    } while (!(entries & 0x80000000));
    
    /* enter the pc in the table. */
    /* remove redundant bits. 
     * and save the count offset bits
     */
    lr = lr >> REDUNDANT_BITS;
    count = lr & COUNT_BIT_MASK;
    lr = lr >> COUNT_BITS;

    /* this is easier than working out how
     * many bits to or, based on the hashShift.
     * maybe it would be better to work out at 
     * the start and save time during the fiq.
     */
    index = (lr << hashShift) >> hashShift;
    
    first = sample = &table->entries[index];
    /* now loop until we either find the entry 
     * or the next free space.
     */
    while ( (sample->pc != lr) && (table->hdr.last < table->hdr.tableSize) )
    {
	if (sample->pc == 0)
	{
	    /* go ahead and stick it in */
	    sample->pc = lr;
	}
	else
	{
	    if (sample->next != 0)
	    {
		/* move along and continue */
		prev = sample;
		sample = &table->entries[sample->next];
	    }
	    else
	    {
		/* create a new entry if available */
		if (table->hdr.last < table->hdr.tableSize)
		{
		    sample = &table->entries[table->hdr.last];
		    /* copy the first sample into the new
		     * field.
		     */
		    memcpy(sample, first, sizeof(struct profHashEntry));
		    /* now update the new entry in the first position.
		     */
		    first->pc = lr;
		    first->next = table->hdr.last;
		    first->counts[0] = 0;
		    first->counts[1] = 0;
		    first->counts[2] = 0;
		    first->counts[3] = 0;
		    table->hdr.last++;
		    /* update the sample pointer so that we
		     * can insert the count.
		     */
		    sample = first;
		}
	    }
	}
    }
	
    /* check if we need to do an eviction. */
    if (sample != first)
    {
	/* copy the sample out of the table. */
	memcpy(&tmpEntry, sample, sizeof(struct profHashEntry));
	/* remove the sample from the chain. */
	tmpIndex = prev->next;
	prev->next = sample->next;
	/* now insert it at the beginning. */
	memcpy(sample, first, sizeof(struct profHashEntry));
	memcpy(first, &tmpEntry, sizeof(struct profHashEntry));
	/* now make the new first entry point to the old
	 * first entry.
	 */
	first->next = tmpIndex;
    }
	
    /* must now check the lr
     * to see if the table is full.
     */
    if (sample->pc == lr)
    {
	/* update the count */
	sample->counts[count]++;
	table->hdr.samples++;
    }
    else
    {
	table->hdr.missed++;
    }
}

void 
displayTable(struct profHashTable *table)
{
    int i;
    struct profHashEntry *sample;
    char buff[100] = ".............................................\n";
    
    for (i=0; i < table->hdr.tableSize; i++)
    {
	sample = &table->entries[i];
	if ((i * table->hdr.tableSize) >= table->hdr.entries)
	{
	    printf("%s", buff);
	    buff[0] = '\0';
	}
	printf("i = %d, pc = 0x%x, next = %d, counts %d %d %d %d\n",
	       i, sample->pc, sample->next, sample->counts[0], 
	       sample->counts[1], sample->counts[2], sample->counts[3]);
    }
    return;
}
