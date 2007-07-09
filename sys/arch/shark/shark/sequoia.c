/*	$NetBSD: sequoia.c,v 1.9 2007/07/09 20:52:28 ad Exp $	*/

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
**
**  INCLUDE FILES
**
*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sequoia.c,v 1.9 2007/07/09 20:52:28 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <machine/bus.h>
#include <sys/time.h>
#include <sys/kernel.h>


#include <dev/isa/isareg.h>
#include <machine/isa_machdep.h>
#include <shark/shark/sequoia.h>
#include <shark/shark/shark_fiq.h>
#include <arm/cpufunc.h>





/*
** 
** MACROS 
**
*/

/* define regisers on sequoia used by pins  */
#define SEQUOIA_1GPIO       PMC_GPCR_REG         /* reg 0x007 gpio 0-3 */      
#define SEQUOIA_2GPIO       SEQ2_OGPIOCR_REG     /* reg 0x304 gpio 4.8 */

/* define pins on sequoia that talk to smart card reader */
#define SCR_DETECT_DIR      GPIOCR2_M_GPIOBDIR0
#define SCR_DETECT          GPIOCR2_M_GPIOBDATA0


#define SCR_POWER_DIR       GPCR_M_GPIODIR0
#define SCR_POWER           GPCR_M_GPIODATA0

#define SCR_RESET_DIR       GPCR_M_GPIODIR1
#define SCR_RESET           GPCR_M_GPIODATA1

#define SCR_CLOCK_DIR       OGPIOCR_M_GPIODIR6
#define SCR_CLOCK           OGPIOCR_M_GPIODATA6

#define SCR_DATA_IN_DIR     GPCR_M_GPIODIR2
#define SCR_DATA_IN         GPCR_M_GPIODATA2

#define SCR_DATA_OUT_DIR    OGPIOCR_M_GPIODIR5
#define SCR_DATA_OUT        OGPIOCR_M_GPIODATA5

#define SCR_BUGA_DIR        OGPIOCR_M_GPIODIR4
#define SCR_BUGA            OGPIOCR_M_GPIODATA4

#define SCR_BUGB_DIR        OGPIOCR_M_GPIODIR7
#define SCR_BUGB            OGPIOCR_M_GPIODATA7

                                 

/* define pins on sequoia that talk to leds */
#define LED_BILED_YELLOW_BIT    FOMPCR_M_PCON5
#define LED_BILED_GREEN_BIT     FOMPCR_M_PCON6

#define LED_DEBUG_YELLOW_BIT    FOMPCR_M_PCON7
#define LED_DEBUG_GREEN_BIT     FOMPCR_M_PCON8


/* define biled colors */
#define  LED_BILED_NONE     0
#define  LED_BILED_GREEN    1
#define  LED_BILED_YELLOW   2
#define  LED_BILED_RED      3

    
#define LED_TIMEOUT    hz / 20                       /* 20 times a second */
#define LED_NET_ACTIVE (1000000/hz) * LED_TIMEOUT   /* delay in us for net activity */
      
    
    

/*
**
** DATA
**
*/
static bus_space_handle_t sequoia_ioh;

static struct timeval ledLastActive;      /* last time we get net activity */
static int      ledColor;           /* present color of led */
static int      ledBlockCount;;     /* reference count of block calles */                            
int sequoia_index_cache = -1;       /* set to silly value so that we dont cache on init */

static callout_t led_timo_ch;

/*
**
** FUNCITONAL PROTOTYPES
**
*/
static void ledSetBiled(int color);
static void ledTimeout(void *arg);

/*
** 
** FUNCITONS
**
*/
void sequoiaInit(void)
{
    u_int16_t  seqReg;

    callout_init(&led_timo_ch, 0);

    /* map the sequoi registers */
    if (bus_space_map(&isa_io_bs_tag, SEQUOIA_BASE, SEQUOIA_NPORTS, 0,  &sequoia_ioh))
    {
        panic("SequoiaInit: io mapping failed");
    }

    /*
    **
    ** setup the pins associated with the X server
    **
    */
    /* seems power on sets them correctly */



    /*
    ** setup the pins associated with the led
    */
    sequoiaRead(SEQR_SEQPSR1_REG,&seqReg); 
    SET(seqReg,SEQPSR1_M_TAGDEN);           /* enable pc[4:9] */
    sequoiaWrite(SEQR_SEQPSR1_REG,seqReg); 

    
    sequoiaRead(SEQR_SEQPSR3_REG,&seqReg); 
    CLR(seqReg,SEQPSR3_M_PC5PINEN);      /* enable pc5, biled */
    CLR(seqReg,SEQPSR3_M_PC6PINEN);      /* enable pc6, biled */
    CLR(seqReg,SEQPSR3_M_PC7PINEN);      /* enable pc7, debug led yellow */
    CLR(seqReg,SEQPSR3_M_PC8PINEN);      /* enable pc8, debug led green  */
    sequoiaWrite(SEQR_SEQPSR3_REG,seqReg); 
    
    sequoiaRead (PMC_FOMPCR_REG, &seqReg);
    CLR(seqReg,LED_BILED_YELLOW_BIT); 
    CLR(seqReg,LED_BILED_GREEN_BIT);
    SET(seqReg,LED_DEBUG_YELLOW_BIT);
    CLR(seqReg,LED_DEBUG_GREEN_BIT);
    sequoiaWrite(PMC_FOMPCR_REG, seqReg);

    
    /* setup the biled info */
    ledColor = LED_BILED_GREEN;
    ledLastActive.tv_usec = 0;
    ledLastActive.tv_sec = 0;
    ledBlockCount = 0;
    callout_reset(&led_timo_ch, LED_TIMEOUT, ledTimeout, NULL);
    /* 
    ** 
    ** setup the pins associated with the smart card reader *
    **
    */
    /* sequoia 1 direction & data */

    sequoiaRead(SEQUOIA_1GPIO,&seqReg);

    SET(seqReg,SCR_POWER_DIR);      /* output */
    SET(seqReg,SCR_RESET_DIR); /* output */
    CLR(seqReg,SCR_DATA_IN_DIR);        /* input */

    CLR(seqReg,SCR_POWER);          /* 0V to card */
    SET(seqReg,SCR_RESET);          /* 0V to card */

    sequoiaWrite(SEQUOIA_1GPIO,seqReg);


    

    /* sequoia 2 pin enables */
    sequoiaRead(SEQ2_SEQ2PSR_REG,&seqReg);

    SET(seqReg,SEQ2PSR_M_DPBUSEN);
    CLR(seqReg,SEQ2PSR_M_GPIOPINEN);

    sequoiaWrite(SEQ2_SEQ2PSR_REG,seqReg);



    /* sequoia 2 direction & data */
    sequoiaRead(SEQUOIA_2GPIO,&seqReg);

    SET(seqReg,SCR_BUGA_DIR);       /* output */
    SET(seqReg,SCR_DATA_OUT_DIR);   /* output */
    SET(seqReg,SCR_CLOCK_DIR);      /* output */
    SET(seqReg,SCR_BUGB_DIR);       /* output */

    CLR(seqReg,SCR_BUGA);           /* 0 */
    CLR(seqReg,SCR_BUGB);           /* 0 */
    CLR(seqReg,SCR_CLOCK);           /* 0 */
    sequoiaWrite(SEQUOIA_2GPIO,seqReg);

     /* setup the wak0 pin to be detect */
    sequoiaRead(SEQR_SEQPSR2_REG,&seqReg);

    SET(seqReg,SEQPSR2_M_DIRTYPINEN); 
    SET(seqReg,SEQPSR2_M_GPIOB0PINEN); 

    sequoiaWrite(SEQR_SEQPSR2_REG,seqReg);


    sequoiaRead(PMC_GPIOCR2_REG,&seqReg);

    CLR(seqReg,SCR_DETECT_DIR);  /* input */

    sequoiaWrite(PMC_GPIOCR2_REG,seqReg);

    /* don't delay when setting PC bits.  this is particularly important
       for using PC[4] to clear the FIQ. */
    sequoiaRead(PMC_SCCR_REG, &seqReg);
    sequoiaWrite(PMC_SCCR_REG, seqReg | SCCR_M_PCSTGDIS);

}





/* X console functions */
void consXTvOn(void)
{
    u_int16_t savedPSR3;
    u_int16_t savedFMPCR;  
    /* 
    ** Switch on TV output on the Sequoia, data indicates mode,
    ** but we are currently hardwired to NTSC, so ignore it.
    */

    sequoiaRead (SEQR_SEQPSR3_REG, &savedPSR3);
    sequoiaWrite(SEQR_SEQPSR3_REG, (savedPSR3 | SEQPSR3_M_PC3PINEN));

    sequoiaRead (PMC_FOMPCR_REG, &savedFMPCR);
    sequoiaWrite(PMC_FOMPCR_REG, (savedFMPCR | FOMPCR_M_PCON3));

}

void consXTvOff(void)
{
    u_int16_t savedPSR3;
    u_int16_t savedFMPCR;  
    /* 
    ** Switch off TV output on the Seqoia 
    */
    sequoiaRead (SEQR_SEQPSR3_REG, &savedPSR3);
    sequoiaWrite(SEQR_SEQPSR3_REG, (savedPSR3 & ~SEQPSR3_M_PC3PINEN));

    sequoiaRead (PMC_FOMPCR_REG, &savedFMPCR);
    sequoiaWrite(PMC_FOMPCR_REG, (savedFMPCR & ~FOMPCR_M_PCON3));

}




  /* smart card routines */

int scrGetDetect (void)
{
    int r;
    u_int16_t  seqReg;

    sequoiaRead(PMC_GPIOCR2_REG,&seqReg);

    /* inverse logic, so invert */
    if (ISSET(seqReg,SCR_DETECT))
    {
        r = 0;
    } else
    {
        r = 1;
    }

    return r;
}


void scrSetPower (int value)
{
    u_int16_t  seqReg;
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif

    sequoiaRead(SEQUOIA_1GPIO,&seqReg);

    if (value)
    {
        SET(seqReg,SCR_POWER);
    } else
    {
        CLR(seqReg,SCR_POWER);
    }
    sequoiaWrite(SEQUOIA_1GPIO,seqReg);

#ifdef SHARK
    restore_interrupts(savedints);
#endif
}

void scrSetClock (int value)
{
    u_int16_t  seqReg;
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif

    sequoiaRead(SEQUOIA_2GPIO,&seqReg);

    if (value)
    {
        SET(seqReg,SCR_CLOCK);
    } else
    {
        CLR(seqReg,SCR_CLOCK);
    }
    sequoiaWrite(SEQUOIA_2GPIO,seqReg);
#ifdef SHARK
    restore_interrupts(savedints);
#endif
}

void scrSetReset (int value)
{
    u_int16_t  seqReg;
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif

    sequoiaRead(SEQUOIA_1GPIO,&seqReg);

    if (value)                         
    {
        SET(seqReg,SCR_RESET);
    } else
    {
        CLR(seqReg,SCR_RESET);
    }
    sequoiaWrite(SEQUOIA_1GPIO,seqReg);

#ifdef SHARK
    restore_interrupts(savedints);
#endif
}


void scrSetDataHighZ (void)
{
    u_int16_t  seqReg;
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif

    sequoiaRead(SEQUOIA_2GPIO,&seqReg);

    /* set data to 5V, io pin is inverse logic */
    CLR(seqReg,SCR_DATA_OUT);

    sequoiaWrite(SEQUOIA_2GPIO,seqReg);
#ifdef SHARK
    restore_interrupts(savedints);
#endif
}

void scrSetData (int value) 
{
    u_int16_t  seqReg;
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif

    sequoiaRead(SEQUOIA_2GPIO,&seqReg);
    /* inverse logic */
    if (value )
    {
        CLR(seqReg,SCR_DATA_OUT);
    } else
    {
        SET(seqReg,SCR_DATA_OUT);
    }
    sequoiaWrite(SEQUOIA_2GPIO,seqReg);
#ifdef SHARK
    restore_interrupts(savedints);
#endif
}

int  scrGetData (void)
{
    int r;
    u_int16_t  seqReg;

    sequoiaRead(SEQUOIA_1GPIO,&seqReg);

    if (ISSET(seqReg,SCR_DATA_IN))
    {
        r = 1;
    } else
    {
        r = 0;
    }
    return r;
}








void ledNetActive (void)
{
    getmicrotime(&ledLastActive);
}

void ledNetBlock    (void)
{
    ledBlockCount++;
}

void ledNetUnblock  (void)
{
    ledBlockCount--;
}

void ledPanic       (void)
{
    /* we are in panic, timeout wont happen, so set led */
    ledSetBiled(LED_BILED_RED);
}

static void   ledTimeout(void *arg)
{
    int timeSpan;   /* in usec */
    struct timeval now;

    getmicrotime(&now);
    if(now.tv_sec == ledLastActive.tv_sec)
    {
        timeSpan = now.tv_usec -  ledLastActive.tv_usec;
    }
    
    else if (now.tv_sec - 10  < ledLastActive.tv_sec) /* stop rollover problems */
    {
        timeSpan = (1000000 + now.tv_usec) -  ledLastActive.tv_usec;
    }

    else
    {
        timeSpan =  LED_NET_ACTIVE * 2; /* ie big number */
    }
    
        
    
    /* check if we are blocked */
    if(ledBlockCount)
    {
        if(ledColor == LED_BILED_YELLOW)
        {
            ledSetBiled(LED_BILED_NONE);
        }
        else
        {
            ledSetBiled(LED_BILED_YELLOW);
        }
    }


    /* check if we have network activity */
    else if (timeSpan < LED_NET_ACTIVE)
    {
        if(ledColor == LED_BILED_GREEN)
        {
            ledSetBiled(LED_BILED_NONE);
        }
        else
        {
            ledSetBiled(LED_BILED_GREEN);
        }
    }

    /* normal operating mode */        
    else
    {
        if(ledColor != LED_BILED_GREEN)
        {
            ledSetBiled(LED_BILED_GREEN);
        }
    }

    /* resubmint the timeout */
    callout_reset(&led_timo_ch, LED_TIMEOUT, ledTimeout, NULL);

}


static void ledSetBiled(int color)
{
    u_int16_t  seqReg;
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif
    ledColor = color;    
    

    sequoiaRead (PMC_FOMPCR_REG, &seqReg);
    switch(color)
    {
        case LED_BILED_NONE:
            SET(seqReg,LED_BILED_YELLOW_BIT);
            SET(seqReg,LED_BILED_GREEN_BIT);
            break;
    
        case LED_BILED_YELLOW:
            CLR(seqReg,LED_BILED_YELLOW_BIT);
            SET(seqReg,LED_BILED_GREEN_BIT);
            break;
    
        case LED_BILED_GREEN:
            SET(seqReg,LED_BILED_YELLOW_BIT);
            CLR(seqReg,LED_BILED_GREEN_BIT);
            break;
    
        case LED_BILED_RED:
            CLR(seqReg,LED_BILED_YELLOW_BIT);
            CLR(seqReg,LED_BILED_GREEN_BIT);
            break;
    
        default:
            panic("invalid color %x",color);
            break;
    }
    sequoiaWrite(PMC_FOMPCR_REG, seqReg);
#ifdef SHARK
    restore_interrupts(savedints);
#endif
}


int hwGetRev(void)
{
    u_int16_t  seqReg;

    sequoiaRead(SR_POR_REG,&seqReg);
    
    seqReg = seqReg >> POR_V_MISCCF0;
    seqReg = seqReg & 0x7;

    return seqReg;
}





/* routines to read/write to sequoia registers */
void sequoiaWrite(int reg,u_int16_t  seqReg)     
{   
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif

    /*
       On SHARK, the fiq comes from the pmi/smi.  After receiving
       a FIQ, the SMI must be cleared.  The SMI gets cleared by
       changing to sleep mode, thereby lowering PC[4]. */
    // need to do the right thing with the cache if this is going to work */
    if (reg == PMC_FOMPCR_REG) {
      bus_space_write_2(&isa_io_bs_tag,sequoia_ioh,SEQUOIA_INDEX_OFFSET,reg);
      bus_space_write_2(&isa_io_bs_tag,sequoia_ioh,SEQUOIA_DATA_OFFSET,
			seqReg | (FOMPCR_M_PCON4));
      bus_space_write_2(&isa_io_bs_tag,sequoia_ioh,SEQUOIA_INDEX_OFFSET,
			PMC_SLPMPCR_REG);
      bus_space_write_2(&isa_io_bs_tag,sequoia_ioh,SEQUOIA_DATA_OFFSET,
			seqReg & ~(SLPMPCR_M_PCSLP4));
      sequoia_index_cache = PMC_SLPMPCR_REG;
    } else {
      /* normal case: just do the write */
      if(sequoia_index_cache != reg)
      {
        sequoia_index_cache = reg;
        bus_space_write_2(&isa_io_bs_tag,sequoia_ioh,SEQUOIA_INDEX_OFFSET,reg);
      }
      bus_space_write_2(&isa_io_bs_tag,sequoia_ioh,SEQUOIA_DATA_OFFSET,seqReg);
    }
#ifdef SHARK
    restore_interrupts(savedints);
#endif
}

void sequoiaRead (int reg,u_int16_t * seqReg_ptr)     
{
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif
    if(sequoia_index_cache != reg)
    {
        sequoia_index_cache = reg;
        bus_space_write_2(&isa_io_bs_tag,sequoia_ioh,SEQUOIA_INDEX_OFFSET,reg);    
    }
    *seqReg_ptr = bus_space_read_2(&isa_io_bs_tag,sequoia_ioh,SEQUOIA_DATA_OFFSET);  
#ifdef SHARK
    restore_interrupts(savedints);
#endif
}


void ledSetDebug(int command)
{
    u_int16_t  seqReg;
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif
    sequoiaRead (PMC_FOMPCR_REG, &seqReg);


    switch (command) 
    {
        case LED_DEBUG_STATE_0:    
            CLR(seqReg,LED_DEBUG_YELLOW_BIT);
            CLR(seqReg,LED_DEBUG_GREEN_BIT);
            break;

        case LED_DEBUG_STATE_1:
            SET(seqReg,LED_DEBUG_YELLOW_BIT);
            CLR(seqReg,LED_DEBUG_GREEN_BIT);
            break;

        case LED_DEBUG_STATE_2:
            CLR(seqReg,LED_DEBUG_YELLOW_BIT);
            SET(seqReg,LED_DEBUG_GREEN_BIT);
            break;

        case LED_DEBUG_STATE_3:
            SET(seqReg,LED_DEBUG_YELLOW_BIT);
            SET(seqReg,LED_DEBUG_GREEN_BIT);
            break;

        case LED_DEBUG_YELLOW_ON:
            SET(seqReg,LED_DEBUG_YELLOW_BIT);
            break;

        case LED_DEBUG_YELLOW_OFF:
            CLR(seqReg,LED_DEBUG_YELLOW_BIT);
            break;

        case LED_DEBUG_GREEN_ON:
            SET(seqReg,LED_DEBUG_GREEN_BIT);
            break;

        case LED_DEBUG_GREEN_OFF:
            CLR(seqReg,LED_DEBUG_GREEN_BIT);
            break;

        default:
            panic("ledSetDebug: invalid command %d",command);
            break;
    }
    sequoiaWrite(PMC_FOMPCR_REG, seqReg);
#ifdef SHARK
    restore_interrupts(savedints);
#endif
}


#ifdef USEFULL_DEBUG  
void sequoiaOneAccess(void)
{
u_int16_t reg;
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif
    reg = bus_space_read_2(&isa_io_bs_tag,sequoia_ioh,SEQUOIA_DATA_OFFSET);  
#ifdef SHARK
    restore_interrupts(savedints);
#endif
}
#endif

int testPin=0;
void scrToggleTestPin (void)
{
    u_int16_t  seqReg;
#ifdef SHARK
    u_int savedints;

    savedints = disable_interrupts(I32_bit | F32_bit);    
#endif

    sequoiaRead(SEQUOIA_2GPIO,&seqReg);

    if (testPin)
    {
        testPin = 0;
        CLR(seqReg,SCR_BUGA);
    } 
    else
    {
        SET(seqReg,SCR_BUGA);
        testPin = 1;
    }
    sequoiaWrite(SEQUOIA_2GPIO,seqReg);
#ifdef SHARK
    restore_interrupts(savedints);
#endif
}

