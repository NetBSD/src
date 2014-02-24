/* Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Copyright (c) 2008 Microsoft.  All rights reserved.
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
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/reboot.h>

#include <machine/emipsreg.h>

#include "common.h"
#include "bootinfo.h"
#include "start.h"
#include "prom_iface.h"

#if _DEBUG
#define TRACE(x) printf x
#else
#define TRACE(x)
#endif

void epmc_halt(void);
void save_locore(void);
void restore_locore(void);

static void *nope(void) {return NULL;}
int getchar(void){return GetChar();}

static void
real_halt(void *arg)
{
    int howto = (int)arg;
    u_int ps = GetPsr();

    /* Turn off interrupts and the TLB */
#define EMIPS_SR_RP  0x08000000  /* reduced power */
#define EMIPS_SR_TS  0x00200000  /* TLB shutdown  */
#define EMIPS_SR_RST 0x00000080  /* Soft-reset    */
#define EMIPS_SR_INT 0x0000ff00  /* Interrupt enable mask */
#define EMIPS_SR_IEc 0x00000001  /* Interrupt enable current */

    ps &= ~(EMIPS_SR_INT | EMIPS_SR_IEc);
    ps |= EMIPS_SR_TS;
    SetPsr(ps);

    /* Reset entry must be restored for reboot 
     */
    restore_locore();

    /* Tell the power manager to halt? */
    for (;howto & RB_HALT;) {
        epmc_halt();

        /* We should not be here!! */
        ps |= EMIPS_SR_RP | EMIPS_SR_INT; /* but not current */
        SetPsr(ps);
    }

    /* For a reboot, all we can really do is a reset actually */
    for (;;) {
        ps |= EMIPS_SR_RST;
        SetPsr(ps);
    }
}

static void
halt(int *unused, int howto)
{
    /* We must switch to a safe stack! TLB will go down 
     */
    switch_stack_and_call((void *)howto,real_halt);
    /* no return, stack lost */
}

struct callback cb = {
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    getchar,
    nope,
    nope,
    printf,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    getsysid,
    nope,
    nope,
    nope,
    nope,
    nope,
    nope,
    halt
};

typedef char *string_t;

void epmc_halt(void)
{
    struct _Pmc *pm = (struct _Pmc *)0xfff00000;

    pm->SystemPowerDisable = PMCSC_CPU;
}


int init_usart(void)
{
    struct _Usart *us = (struct _Usart *)0xfff90000;

    us->Baud = 0x29;
    us->Control = (USC_RXEN|USC_TXEN|USC_BPC_8|USC_NONE|USC_1STOP|USC_CLKDIV_4);
    return 1;
}

/* Need to scan the PMT for all memory controllers
 * Actually.. just enough to make the kernel fit but we dont know how big it is
 */

/* Common format for SRAM, DDRAM, and FLASH controllers. 
 * Use SRAM decl. and careful about DDRAM that is twice as big.
 */
typedef struct _Sram *ram_controller_t;
# define                RAMBT_TAG   SRAMBT_TAG
# define                RAMBT_BASE  SRAMBT_BASE
# define                RAMST_SIZE  SRAMST_SIZE

int init_memory(void)
{
    struct _Pmt *Pmt;
    ram_controller_t Ram, Ours, First;
    uint32_t base, addr, moi = (uint32_t)(&init_memory) & 0x3ffff000;
    size_t size;
    uint16_t tag;
    int nsr, ndr, nfl;

    /* Make three passes.
     * First find which controller we are running under, cuz we cant touch it.
     * Then remap every RAM segment around it.
     * Then make sure FLASH segments do not overlap RAM.
     */

    nsr = ndr = nfl = 0;
    First = Ours = NULL;
    base = ~0;
    for (Pmt = ThePmt;;Pmt--) {
        tag = Pmt->Tag;
        //printf("PMT @%x tag=%x\n",Pmt,tag);
        switch (tag) {
        case PMTTAG_END_OF_TABLE:
            goto DoneFirst;
        case PMTTAG_SRAM:
        case PMTTAG_DDRAM:
        case PMTTAG_FLASH:
            Ram = (ram_controller_t)(Pmt->TopOfPhysicalAddress << 16);
            /* Scan the whole segment */
            for (;;) {
                //printf("RAM @%x tag=%x ctl=%x\n", Ram, Ram->BaseAddressAndTag,Ram->Control);
                if (tag != (Ram->BaseAddressAndTag & RAMBT_TAG))
                    break;
                addr = Ram->BaseAddressAndTag & RAMBT_BASE;
                if ((tag != PMTTAG_FLASH) && (addr < base)) {
                    base = addr;
                    First = Ram;
                }
                size = Ram->Control & RAMST_SIZE;
                if ((moi >= addr) && (moi < (addr + size))) {
                    Ours = Ram;
                }
                /* Next one.. and count them */
                Ram++;
                switch (tag) {
                case PMTTAG_SRAM:
                    nsr++;
                    break;
                case PMTTAG_FLASH:
                    nfl++;
                    break;
                case PMTTAG_DDRAM:
                    Ram++; /* yeach */
                    ndr++;
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    /* Make sure we know */
 DoneFirst:
    if ((First == NULL) || (Ours == NULL)) {
        printf("Bad memory layout (%p, %p), wont work.\n", First, Ours);
        return 0;
    }

    /* Second pass now */
    base += First->BaseAddressAndTag & RAMBT_BASE;
    for (Pmt = ThePmt;;Pmt--) {
        tag = Pmt->Tag;
        //printf("PMT @%x tag=%x\n",Pmt,tag);
        switch (tag) {
        case PMTTAG_END_OF_TABLE:
            goto DoneSecond;
        case PMTTAG_SRAM:
        case PMTTAG_DDRAM:
        case PMTTAG_FLASH:
            Ram = (ram_controller_t)(Pmt->TopOfPhysicalAddress << 16);
            /* Scan the whole segment */
            for (;;) {
                //printf("RAM @%x tag=%x ctl=%x\n", Ram, Ram->BaseAddressAndTag,Ram->Control);
                if (tag != (Ram->BaseAddressAndTag & RAMBT_TAG))
                    break;
                /* Leave us alone */
                if (Ram == Ours)
                    goto Next;
                /* Leave the first alone too */
                if (Ram == First)
                    goto Next;
                /* We do FLASH next round */
                if (tag == PMTTAG_FLASH)
                    goto Next;

                addr = Ram->BaseAddressAndTag & RAMBT_BASE;
                size = Ram->Control & RAMST_SIZE;

                /* Dont make it overlap with us */
                if ((moi >= base) && (moi < (base + size)))
                    base += Ours->Control & RAMST_SIZE;

                if (addr != base) {
                    printf("remapping %x+%x to %x\n", addr, size, base);
                    Ram->BaseAddressAndTag = base;
                }
                base += size;

            Next:
                Ram++;
                if (tag == PMTTAG_DDRAM) Ram++; /* yeach */
            }
            break;
        default:
            break;
        }
    }
 DoneSecond:

    /* Third pass now: FLASH */
    for (Pmt = ThePmt;;Pmt--) {
        tag = Pmt->Tag;
        //printf("PMT @%x tag=%x\n",Pmt,tag);
        switch (tag) {
        case PMTTAG_END_OF_TABLE:
            goto DoneThird;
        case PMTTAG_FLASH:
            Ram = (ram_controller_t)(Pmt->TopOfPhysicalAddress << 16);
            /* Scan the whole segment */
            for (;;Ram++) {
                //printf("RAM @%x tag=%x ctl=%x\n", Ram, Ram->BaseAddressAndTag,Ram->Control);
                if (tag != (Ram->BaseAddressAndTag & RAMBT_TAG))
                    break;
                /* Leave us alone */
                if (Ram == Ours)
                    continue;

                addr = Ram->BaseAddressAndTag & RAMBT_BASE;
                size = Ram->Control & RAMST_SIZE;

                /* No need to move if it does not overlap RAM */
                if (addr >= base)
                    continue;

                /* Ahi */
                printf("remapping FLASH %x+%x to %x\n", addr, size, base);
                Ram->BaseAddressAndTag = base;
                base += size;
            }
            break;
        default:
            break;
        }
    }
DoneThird:
    return (nfl<<16) | (nsr << 8) | (ndr << 0);
}

u_int startjump[2];
u_int exceptioncode[(0x200-0x080)/4]; /* Change if ExceptionHandlerEnd changes */

void save_locore(void)
{
    memcpy(startjump,start,sizeof startjump);
    memcpy(exceptioncode,ExceptionHandler,sizeof exceptioncode);
}

void restore_locore(void)
{
    memcpy(start,startjump,sizeof startjump);
    memcpy(ExceptionHandler,exceptioncode,sizeof exceptioncode);
    /* BUGBUG flush icache */
}

void call_kernel(uint32_t addr, char *kname, char *kargs, u_int bim, char *bip)
{
    int argc = 0;
    string_t argv[3];
    int code = PROM_MAGIC;
    struct callback * cv = &cb;

    /* Safeguard ourselves */
    save_locore();

    if (kargs == NULL) kargs = "";
    argv[0] = kname;
    argv[1] = kargs;
    argv[2] = NULL;
    argc = 2;

    TRACE(("Geronimo(%x,%s %s)!\n",addr,kname,kargs));
    ((void(*)(int,char**,int,struct callback *,u_int,char*))addr)
           (argc,argv,code,cv,bim,bip);
}

