/*
 * Copyright (c) 1997, 2000 Hellmuth Michaelis. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	i4b_l4mgmt.c - layer 4 calldescriptor management utilites
 *	-----------------------------------------------------------
 *
 *	$Id: i4b_l4mgmt.c,v 1.8 2002/03/29 20:29:53 martin Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_l4mgmt.c,v 1.8 2002/03/29 20:29:53 martin Exp $");

#include "isdn.h"

#if NISDN > 0

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <net/if.h>

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
#include <sys/callout.h>
#endif

#if defined(__FreeBSD__)
#if defined (__FreeBSD_version) && __FreeBSD_version <= 400000
#include <machine/random.h>
#else
#include <sys/random.h>
#endif
#endif

#ifdef __FreeBSD__
#include <machine/i4b_debug.h>
#include <machine/i4b_ioctl.h>
#else
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>
#endif

#include <netisdn/i4b_l3l4.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_isdnq931.h>
#include <netisdn/i4b_global.h>

#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l1l2.h>
#include <netisdn/i4b_l4.h>

static unsigned int get_cdid(void);

static void i4b_init_callout(call_desc_t *);
static void i4b_stop_callout(call_desc_t *cd);

#define N_CALL_DESC 40	/* XXX - make this sizeable */
call_desc_t call_desc[N_CALL_DESC];	/* call descriptor array */
int num_call_desc = 0;

/*---------------------------------------------------------------------------*
 *      return a new unique call descriptor id
 *	--------------------------------------
 *	returns a new calldescriptor id which is used to uniquely identyfy
 *	a single call in the communication between kernel and userland.
 *	this cdid is then used to associate a calldescriptor with an id.
 *---------------------------------------------------------------------------*/
static unsigned int
get_cdid(void)
{
	static unsigned int cdid_count = 0;
	int i;
	int x;

	x = splnet();   

	/* get next id */
	
	cdid_count++;
	
again:
	if(cdid_count == CDID_UNUSED)		/* zero is invalid */
		cdid_count++;
	else if(cdid_count > CDID_MAX)		/* wraparound ? */
		cdid_count = 1;

	/* check if id already in use */
	
	for(i=0; i < num_call_desc; i++)
	{
		if(call_desc[i].cdid == cdid_count)
		{
			cdid_count++;
			goto again;
		}
	}

	splx(x);
	
	return(cdid_count);
}

/*---------------------------------------------------------------------------*
 *      reserve a calldescriptor for later usage
 *      ----------------------------------------
 *      searches the calldescriptor array until an unused
 *      descriptor is found, gets a new calldescriptor id
 *      and reserves it by putting the id into the cdid field.
 *      returns pointer to the calldescriptor.
 *---------------------------------------------------------------------------*/
call_desc_t *
reserve_cd(void)
{
	call_desc_t *cd;
	int x;
	int i;

	x = splnet();

	cd = NULL;
	
	for(i=0; i < num_call_desc; i++)
	{
		if(call_desc[i].cdid == CDID_UNUSED)
		{
			cd = &(call_desc[i]);	/* get pointer to descriptor */
			NDBGL4(L4_MSG, "found free cd - index=%d cdid=%u",
				 i, call_desc[i].cdid);
			break;
		}
	}
	if (cd == NULL && num_call_desc < N_CALL_DESC) {
		i = num_call_desc++;
		cd = &(call_desc[i]);	/* get pointer to descriptor */
		NDBGL4(L4_MSG, "found free cd - index=%d cdid=%u",
			 i, call_desc[i].cdid);
	}
	if (cd != NULL) {
		memset(cd, 0, sizeof(call_desc_t)); /* clear it */
		cd->cdid = get_cdid();	/* fill in new cdid */
	}

	splx(x);

	if(cd == NULL)
		panic("reserve_cd: no free call descriptor available!");

	i4b_init_callout(cd);

	return(cd);
}

/*---------------------------------------------------------------------------*
 *      free a calldescriptor
 *      ---------------------
 *      free a unused calldescriptor by giving address of calldescriptor
 *      and writing a 0 into the cdid field marking it as unused.
 *---------------------------------------------------------------------------*/
void
freecd_by_cd(call_desc_t *cd)
{
	int i;
	int x = splnet();
	
	for(i=0; i < num_call_desc; i++)
	{
		if( (call_desc[i].cdid != CDID_UNUSED) &&
		    (&(call_desc[i]) == cd) )
		{
			NDBGL4(L4_MSG, "releasing cd - index=%d cdid=%u cr=%d",
				i, call_desc[i].cdid, cd->cr);
			call_desc[i].cdid = CDID_UNUSED;
			break;
		}
	}

	if(i == N_CALL_DESC)
		panic("freecd_by_cd: ERROR, cd not found, cr = %d\n", cd->cr);

	splx(x);		
}

/*
 * BRI is gone, get rid of all CDs for it
 */
void free_all_cd_of_bri(int bri)
{
	int i;
	int x = splnet();
	
	for(i=0; i < num_call_desc; i++)
	{
		if( (call_desc[i].cdid != CDID_UNUSED) &&
		    call_desc[i].bri == bri) {
			NDBGL4(L4_MSG, "releasing cd - index=%d cdid=%u cr=%d",
				i, call_desc[i].cdid, call_desc[i].cr);
			if (call_desc[i].callouts_inited)
				i4b_stop_callout(&call_desc[i]);
			call_desc[i].cdid = CDID_UNUSED;
			call_desc[i].bri = -1;
			break;
		}
	}

	splx(x);		
}

/*---------------------------------------------------------------------------*
 *      return pointer to calldescriptor by giving the calldescriptor id
 *      ----------------------------------------------------------------
 *      lookup a calldescriptor in the calldescriptor array by looking
 *      at the cdid field. return pointer to calldescriptor if found,
 *      else return NULL if not found.
 *---------------------------------------------------------------------------*/
call_desc_t *
cd_by_cdid(unsigned int cdid)
{
	int i;

	for(i=0; i < num_call_desc; i++)
	{
		if(call_desc[i].cdid == cdid)
		{
			NDBGL4(L4_MSG, "found cdid - index=%d cdid=%u cr=%d",
					i, call_desc[i].cdid, call_desc[i].cr);
			i4b_init_callout(&call_desc[i]);
			return(&(call_desc[i]));
		}
	}
	return(NULL);
}

/*---------------------------------------------------------------------------*
 *      search calldescriptor
 *      ---------------------
 *      This routine searches for the calldescriptor for a passive controller
 *      given by unit number, callreference and callreference flag.
 *	It returns a pointer to the calldescriptor if found, else a NULL.
 *---------------------------------------------------------------------------*/
call_desc_t *
cd_by_bricr(int bri, int cr, int crf)
{
	int i;

	for(i=0; i < num_call_desc; i++)
	{
	  if (call_desc[i].cdid != CDID_UNUSED && call_desc[i].bri == bri &&
	     call_desc[i].cr == cr && call_desc[i].crflag == crf)
	  {
	    NDBGL4(L4_MSG, "found cd, index=%d cdid=%u cr=%d",
			i, call_desc[i].cdid, call_desc[i].cr);
	    i4b_init_callout(&call_desc[i]);
	    return(&(call_desc[i]));
	  }
	}
	return(NULL);
}

/*---------------------------------------------------------------------------*
 *	generate 7 bit "random" number used for outgoing Call Reference
 *---------------------------------------------------------------------------*/
unsigned char
get_rand_cr(int unit)
{
	register int i, j;
	static u_char val, retval;
	static int called = 42;
	
	val += ++called;
	
	for(i=0; i < 50 ; i++, val++)
	{
		int found = 1;
		
#if defined(__FreeBSD__)

#ifdef RANDOMDEV
		read_random((char *)&val, sizeof(val));
#else
		val = (u_char)random();
#endif /* RANDOMDEV */

#else
		val |= unit+i;
		val <<= i;
		val ^= (time.tv_sec >> 8) ^ time.tv_usec;
		val <<= i;
		val ^= time.tv_sec ^ (time.tv_usec >> 8);
#endif

		retval = val & 0x7f;
		
		if(retval == 0 || retval == 0x7f)
			continue;

		for(j=0; j < num_call_desc; j++)
		{
			if( (call_desc[j].cdid != CDID_UNUSED) &&
			    (call_desc[j].cr == retval) )
			{
				found = 0;
				break;
			}
		}

		if(found)
			return(retval);
	}
	return(0);	/* XXX */
}

static void
i4b_stop_callout(call_desc_t *cd)
{
	if (!cd->callouts_inited)
		return;

	callout_stop(&cd->idle_timeout_handle);
	callout_stop(&cd->T303_callout);
	callout_stop(&cd->T305_callout);
	callout_stop(&cd->T308_callout);
	callout_stop(&cd->T309_callout);
	callout_stop(&cd->T310_callout);
	callout_stop(&cd->T313_callout);
	callout_stop(&cd->T400_callout);
}

/*---------------------------------------------------------------------------*
 *	initialize the callout handles for FreeBSD
 *---------------------------------------------------------------------------*/
void
i4b_init_callout(call_desc_t *cd)
{
	if(cd->callouts_inited == 0)
	{
		callout_init(&cd->idle_timeout_handle);
		callout_init(&cd->T303_callout);
		callout_init(&cd->T305_callout);
		callout_init(&cd->T308_callout);
		callout_init(&cd->T309_callout);
		callout_init(&cd->T310_callout);
		callout_init(&cd->T313_callout);
		callout_init(&cd->T400_callout);
		cd->callouts_inited = 1;
	}
}

#ifdef I4B_CD_DEBUG_PRINT

extern char *print_l3state(call_desc_t *cd);

void i4b_print_cdp(call_desc_t *cdp);
void i4b_print_cdx(int index);
void i4b_print_cda(void);
void i4b_print_cdaa(void);
	
/*---------------------------------------------------------------------------*
 *	print a call descriptor by cd-pointer
 *---------------------------------------------------------------------------*/
void 
i4b_print_cdp(call_desc_t *cdp)
{
	if((cdp > &(call_desc[num_call_desc])) || (cdp < &(call_desc[0])))
	{
		printf("i4b_print_cd: cdp out of range!\n");
		return;
	}

	printf("i4b_print_cd: printing call descriptor %d at 0x%lx:\n", cdp - (&(call_desc[0])), (unsigned long)cdp);

	printf("         cdid = %d\n", cdp->cdid);
	printf("   controller = %d (u=%d, dl=%d, b1=%d, b2=%d)\n",
			cdp->controller,
			ctrl_desc[cdp->controller].unit,
			ctrl_desc[cdp->controller].dl_est,
			ctrl_desc[cdp->controller].bch_state[CHAN_B1],
			ctrl_desc[cdp->controller].bch_state[CHAN_B2]);	
	printf("           cr = 0x%02x\n", cdp->cr);
	printf("       crflag = %d\n", cdp->crflag);
	printf("    channelid = %d\n", cdp->channelid);
	printf("        bprot = %d\n", cdp->bprot);
	printf("       driver = %d\n", cdp->driver);
	printf("  driver_unit = %d\n", cdp->driver_unit);
	printf("   call_state = %d\n", cdp->call_state);
	printf("    Q931state = %s\n", print_l3state(cdp));
	printf("        event = %d\n", cdp->event);
	printf("     response = %d\n", cdp->response);
	printf("         T303 = %d\n", cdp->T303);
	printf("T303_first_to = %d\n", cdp->T303_first_to);
	printf("         T305 = %d\n", cdp->T305);
	printf("         T308 = %d\n", cdp->T308);
	printf("T308_first_to = %d\n", cdp->T308_first_to);
	printf("         T309 = %d\n", cdp->T309);
	printf("         T310 = %d\n", cdp->T310);
	printf("         T313 = %d\n", cdp->T313);
	printf("         T400 = %d\n", cdp->T400);
	printf("          dir = %s\n", cdp->dir == DIR_OUTGOING ? "out" : "in");
}

/*---------------------------------------------------------------------------*
 *	print a call descriptor by index
 *---------------------------------------------------------------------------*/
void 
i4b_print_cdx(int index)
{
	if(index >= num_call_desc)
	{
		printf("i4b_print_cdx: index %d >= N_CALL_DESC %d\n", index, N_CALL_DESC);
		return;
	}
	i4b_print_cdp(&(call_desc[index]));
}

/*---------------------------------------------------------------------------*
 *	print all call descriptors
 *---------------------------------------------------------------------------*/
void 
i4b_print_cda(void)
{
	int i;

	for(i=0; i < num_call_desc; i++)
	{
		i4b_print_cdp(&(call_desc[i]));
	}
}

/*---------------------------------------------------------------------------*
 *	print all active call descriptors
 *---------------------------------------------------------------------------*/
void 
i4b_print_cdaa(void)
{
	int i;

	for(i=0; i < num_call_desc; i++)
	{
		if(call_desc[i].cdid != CDID_UNUSED)
		{
			i4b_print_cdp(&(call_desc[i]));
		}
	}
}

#endif /* I4B_CD_DEBUG_PRINT */

#endif /* NISDN > 0 */
