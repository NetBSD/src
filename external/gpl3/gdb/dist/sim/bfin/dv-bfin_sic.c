/* Blackfin System Interrupt Controller (SIC) model.

   Copyright (C) 2010-2011 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"

#include "sim-main.h"
#include "devices.h"
#include "dv-bfin_sic.h"
#include "dv-bfin_cec.h"

struct bfin_sic
{
  /* We assume first element is the base.  */
  bu32 base;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu16 BFIN_MMR_16(swrst);
  bu16 BFIN_MMR_16(syscr);
  bu16 BFIN_MMR_16(rvect);  /* XXX: BF59x has a 32bit AUX_REVID here.  */
  union {
    struct {
      bu32 imask0;
      bu32 iar0, iar1, iar2, iar3;
      bu32 isr0, iwr0;
      bu32 _pad0[9];
      bu32 imask1;
      bu32 iar4, iar5, iar6, iar7;
      bu32 isr1, iwr1;
    } bf52x;
    struct {
      bu32 imask;
      bu32 iar0, iar1, iar2, iar3;
      bu32 isr, iwr;
    } bf537;
    struct {
      bu32 imask0, imask1, imask2;
      bu32 isr0, isr1, isr2;
      bu32 iwr0, iwr1, iwr2;
      bu32 iar0, iar1, iar2, iar3;
      bu32 iar4, iar5, iar6, iar7;
      bu32 iar8, iar9, iar10, iar11;
    } bf54x;
    struct {
      bu32 imask0, imask1;
      bu32 iar0, iar1, iar2, iar3;
      bu32 iar4, iar5, iar6, iar7;
      bu32 isr0, isr1;
      bu32 iwr0, iwr1;
    } bf561;
  };
};
#define mmr_base()      offsetof(struct bfin_sic, swrst)
#define mmr_offset(mmr) (offsetof(struct bfin_sic, mmr) - mmr_base())
#define mmr_idx(mmr)    (mmr_offset (mmr) / 4)

static const char * const bf52x_mmr_names[] =
{
  "SWRST", "SYSCR", "SIC_RVECT", "SIC_IMASK0", "SIC_IAR0", "SIC_IAR1",
  "SIC_IAR2", "SIC_IAR3", "SIC_ISR0", "SIC_IWR0",
  [mmr_idx (bf52x.imask1)] = "SIC_IMASK1", "SIC_IAR4", "SIC_IAR5",
  "SIC_IAR6", "SIC_IAR7", "SIC_ISR1", "SIC_IWR1",
};
static const char * const bf537_mmr_names[] =
{
  "SWRST", "SYSCR", "SIC_RVECT", "SIC_IMASK", "SIC_IAR0", "SIC_IAR1",
  "SIC_IAR2", "SIC_IAR3", "SIC_ISR", "SIC_IWR",
};
static const char * const bf54x_mmr_names[] =
{
  "SWRST", "SYSCR", "SIC_RVECT", "SIC_IMASK0", "SIC_IMASK1", "SIC_IMASK2",
  "SIC_ISR0", "SIC_ISR1", "SIC_ISR2", "SIC_IWR0", "SIC_IWR1", "SIC_IWR2",
  "SIC_IAR0", "SIC_IAR1", "SIC_IAR2", "SIC_IAR3",
  "SIC_IAR4", "SIC_IAR5", "SIC_IAR6", "SIC_IAR7",
  "SIC_IAR8", "SIC_IAR9", "SIC_IAR10", "SIC_IAR11",
};
static const char * const bf561_mmr_names[] =
{
  "SWRST", "SYSCR", "SIC_RVECT", "SIC_IMASK0", "SIC_IMASK1",
  "SIC_IAR0", "SIC_IAR1", "SIC_IAR2", "SIC_IAR3",
  "SIC_IAR4", "SIC_IAR5", "SIC_IAR6", "SIC_IAR7",
  "SIC_ISR0", "SIC_ISR1", "SIC_IWR0", "SIC_IWR1",
};
static const char * const *mmr_names;
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

static void
bfin_sic_forward_interrupts (struct hw *me, bu32 *isr, bu32 *imask, bu32 *iar)
{
  int my_port;
  bu32 ipend;

  /* Process pending and unmasked interrupts.  */
  ipend = *isr & *imask;

  /* Usually none are pending unmasked, so avoid bit twiddling.  */
  if (!ipend)
    return;

  for (my_port = 0; my_port < 32; ++my_port)
    {
      bu32 iar_idx, iar_off, iar_val;
      bu32 bit = (1 << my_port);

      /* This bit isn't pending, so check next one.  */
      if (!(ipend & bit))
	continue;

      /* The IAR registers map the System input to the Core output.
         Every 4 bits in the IAR are used to map to IVG{7..15}.  */
      iar_idx = my_port / 8;
      iar_off = (my_port % 8) * 4;
      iar_val = (iar[iar_idx] & (0xf << iar_off)) >> iar_off;
      HW_TRACE ((me, "forwarding int %i to CEC", IVG7 + iar_val));
      hw_port_event (me, IVG7 + iar_val, 1);
    }
}

static void
bfin_sic_52x_forward_interrupts (struct hw *me, struct bfin_sic *sic)
{
  bfin_sic_forward_interrupts (me, &sic->bf52x.isr0, &sic->bf52x.imask0, &sic->bf52x.iar0);
  bfin_sic_forward_interrupts (me, &sic->bf52x.isr1, &sic->bf52x.imask1, &sic->bf52x.iar4);
}

static unsigned
bfin_sic_52x_io_write_buffer (struct hw *me, const void *source, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - sic->base;
  valuep = (void *)((unsigned long)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: Discard all SIC writes for now.  */
  switch (mmr_off)
    {
    case mmr_offset(swrst):
      /* XXX: This should trigger a software reset ...  */
      break;
    case mmr_offset(syscr):
      /* XXX: what to do ...  */
      break;
    case mmr_offset(bf52x.imask0):
    case mmr_offset(bf52x.imask1):
      bfin_sic_52x_forward_interrupts (me, sic);
      *value32p = value;
      break;
    case mmr_offset(bf52x.iar0) ... mmr_offset(bf52x.iar3):
    case mmr_offset(bf52x.iar4) ... mmr_offset(bf52x.iar7):
    case mmr_offset(bf52x.iwr0):
    case mmr_offset(bf52x.iwr1):
      *value32p = value;
      break;
    case mmr_offset(bf52x.isr0):
    case mmr_offset(bf52x.isr1):
      /* ISR is read-only.  */
      break;
    default:
      /* XXX: Should discard other writes.  */
      ;
    }

  return nr_bytes;
}

static unsigned
bfin_sic_52x_io_read_buffer (struct hw *me, void *dest, int space,
			     address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  mmr_off = addr - sic->base;
  valuep = (void *)((unsigned long)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(swrst):
    case mmr_offset(syscr):
    case mmr_offset(rvect):
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(bf52x.imask0):
    case mmr_offset(bf52x.imask1):
    case mmr_offset(bf52x.iar0) ... mmr_offset(bf52x.iar3):
    case mmr_offset(bf52x.iar4) ... mmr_offset(bf52x.iar7):
    case mmr_offset(bf52x.iwr0):
    case mmr_offset(bf52x.iwr1):
    case mmr_offset(bf52x.isr0):
    case mmr_offset(bf52x.isr1):
      dv_store_4 (dest, *value32p);
      break;
    default:
      if (nr_bytes == 2)
	dv_store_2 (dest, 0);
      else
	dv_store_4 (dest, 0);
      break;
    }

  return nr_bytes;
}

static void
bfin_sic_537_forward_interrupts (struct hw *me, struct bfin_sic *sic)
{
  bfin_sic_forward_interrupts (me, &sic->bf537.isr, &sic->bf537.imask, &sic->bf537.iar0);
}

static unsigned
bfin_sic_537_io_write_buffer (struct hw *me, const void *source, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - sic->base;
  valuep = (void *)((unsigned long)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: Discard all SIC writes for now.  */
  switch (mmr_off)
    {
    case mmr_offset(swrst):
      /* XXX: This should trigger a software reset ...  */
      break;
    case mmr_offset(syscr):
      /* XXX: what to do ...  */
      break;
    case mmr_offset(bf537.imask):
      bfin_sic_537_forward_interrupts (me, sic);
      *value32p = value;
      break;
    case mmr_offset(bf537.iar0):
    case mmr_offset(bf537.iar1):
    case mmr_offset(bf537.iar2):
    case mmr_offset(bf537.iar3):
    case mmr_offset(bf537.iwr):
      *value32p = value;
      break;
    case mmr_offset(bf537.isr):
      /* ISR is read-only.  */
      break;
    default:
      /* XXX: Should discard other writes.  */
      ;
    }

  return nr_bytes;
}

static unsigned
bfin_sic_537_io_read_buffer (struct hw *me, void *dest, int space,
			     address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  mmr_off = addr - sic->base;
  valuep = (void *)((unsigned long)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(swrst):
    case mmr_offset(syscr):
    case mmr_offset(rvect):
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(bf537.imask):
    case mmr_offset(bf537.iar0):
    case mmr_offset(bf537.iar1):
    case mmr_offset(bf537.iar2):
    case mmr_offset(bf537.iar3):
    case mmr_offset(bf537.isr):
    case mmr_offset(bf537.iwr):
      dv_store_4 (dest, *value32p);
      break;
    default:
      if (nr_bytes == 2)
	dv_store_2 (dest, 0);
      else
	dv_store_4 (dest, 0);
      break;
    }

  return nr_bytes;
}

static void
bfin_sic_54x_forward_interrupts (struct hw *me, struct bfin_sic *sic)
{
  bfin_sic_forward_interrupts (me, &sic->bf54x.isr0, &sic->bf54x.imask0, &sic->bf54x.iar0);
  bfin_sic_forward_interrupts (me, &sic->bf54x.isr1, &sic->bf54x.imask1, &sic->bf54x.iar4);
  bfin_sic_forward_interrupts (me, &sic->bf54x.isr2, &sic->bf54x.imask2, &sic->bf54x.iar8);
}

static unsigned
bfin_sic_54x_io_write_buffer (struct hw *me, const void *source, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - sic->base;
  valuep = (void *)((unsigned long)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: Discard all SIC writes for now.  */
  switch (mmr_off)
    {
    case mmr_offset(swrst):
      /* XXX: This should trigger a software reset ...  */
      break;
    case mmr_offset(syscr):
      /* XXX: what to do ...  */
      break;
    case mmr_offset(bf54x.imask0) ... mmr_offset(bf54x.imask2):
      bfin_sic_54x_forward_interrupts (me, sic);
      *value32p = value;
      break;
    case mmr_offset(bf54x.iar0) ... mmr_offset(bf54x.iar11):
    case mmr_offset(bf54x.iwr0) ... mmr_offset(bf54x.iwr2):
      *value32p = value;
      break;
    case mmr_offset(bf54x.isr0) ... mmr_offset(bf54x.isr2):
      /* ISR is read-only.  */
      break;
    default:
      /* XXX: Should discard other writes.  */
      ;
    }

  return nr_bytes;
}

static unsigned
bfin_sic_54x_io_read_buffer (struct hw *me, void *dest, int space,
			     address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  mmr_off = addr - sic->base;
  valuep = (void *)((unsigned long)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(swrst):
    case mmr_offset(syscr):
    case mmr_offset(rvect):
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(bf54x.imask0) ... mmr_offset(bf54x.imask2):
    case mmr_offset(bf54x.iar0) ... mmr_offset(bf54x.iar11):
    case mmr_offset(bf54x.iwr0) ... mmr_offset(bf54x.iwr2):
    case mmr_offset(bf54x.isr0) ... mmr_offset(bf54x.isr2):
      dv_store_4 (dest, *value32p);
      break;
    default:
      if (nr_bytes == 2)
	dv_store_2 (dest, 0);
      else
	dv_store_4 (dest, 0);
      break;
    }

  return nr_bytes;
}

static void
bfin_sic_561_forward_interrupts (struct hw *me, struct bfin_sic *sic)
{
  bfin_sic_forward_interrupts (me, &sic->bf561.isr0, &sic->bf561.imask0, &sic->bf561.iar0);
  bfin_sic_forward_interrupts (me, &sic->bf561.isr1, &sic->bf561.imask1, &sic->bf561.iar4);
}

static unsigned
bfin_sic_561_io_write_buffer (struct hw *me, const void *source, int space,
			      address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  if (nr_bytes == 4)
    value = dv_load_4 (source);
  else
    value = dv_load_2 (source);

  mmr_off = addr - sic->base;
  valuep = (void *)((unsigned long)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_WRITE ();

  /* XXX: Discard all SIC writes for now.  */
  switch (mmr_off)
    {
    case mmr_offset(swrst):
      /* XXX: This should trigger a software reset ...  */
      break;
    case mmr_offset(syscr):
      /* XXX: what to do ...  */
      break;
    case mmr_offset(bf561.imask0):
    case mmr_offset(bf561.imask1):
      bfin_sic_561_forward_interrupts (me, sic);
      *value32p = value;
      break;
    case mmr_offset(bf561.iar0) ... mmr_offset(bf561.iar3):
    case mmr_offset(bf561.iar4) ... mmr_offset(bf561.iar7):
    case mmr_offset(bf561.iwr0):
    case mmr_offset(bf561.iwr1):
      *value32p = value;
      break;
    case mmr_offset(bf561.isr0):
    case mmr_offset(bf561.isr1):
      /* ISR is read-only.  */
      break;
    default:
      /* XXX: Should discard other writes.  */
      ;
    }

  return nr_bytes;
}

static unsigned
bfin_sic_561_io_read_buffer (struct hw *me, void *dest, int space,
			     address_word addr, unsigned nr_bytes)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 mmr_off;
  bu16 *value16p;
  bu32 *value32p;
  void *valuep;

  mmr_off = addr - sic->base;
  valuep = (void *)((unsigned long)sic + mmr_base() + mmr_off);
  value16p = valuep;
  value32p = valuep;

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(swrst):
    case mmr_offset(syscr):
    case mmr_offset(rvect):
      dv_store_2 (dest, *value16p);
      break;
    case mmr_offset(bf561.imask0):
    case mmr_offset(bf561.imask1):
    case mmr_offset(bf561.iar0) ... mmr_offset(bf561.iar3):
    case mmr_offset(bf561.iar4) ... mmr_offset(bf561.iar7):
    case mmr_offset(bf561.iwr0):
    case mmr_offset(bf561.iwr1):
    case mmr_offset(bf561.isr0):
    case mmr_offset(bf561.isr1):
      dv_store_4 (dest, *value32p);
      break;
    default:
      if (nr_bytes == 2)
	dv_store_2 (dest, 0);
      else
	dv_store_4 (dest, 0);
      break;
    }

  return nr_bytes;
}

/* XXX: This doesn't handle DMA<->peripheral mappings.  */
#define BFIN_SIC_TO_CEC_PORTS \
  { "ivg7",  IVG7,  0, output_port, }, \
  { "ivg8",  IVG8,  0, output_port, }, \
  { "ivg9",  IVG9,  0, output_port, }, \
  { "ivg10", IVG10, 0, output_port, }, \
  { "ivg11", IVG11, 0, output_port, }, \
  { "ivg12", IVG12, 0, output_port, }, \
  { "ivg13", IVG13, 0, output_port, }, \
  { "ivg14", IVG14, 0, output_port, }, \
  { "ivg15", IVG15, 0, output_port, },

/* Give each SIC its own base to make it easier to extract the pin at
   runtime.  The pin is used as its bit position in the SIC MMRs.  */
#define ENC(sic, pin) (((sic) << 8) + (pin))
#define DEC_PIN(pin) ((pin) % 0x100)
#define DEC_SIC(pin) ((pin) >> 8)

static const struct hw_port_descriptor bfin_sic_50x_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  /* SIC0 */
  { "pll",             ENC(0,  0), 0, input_port, },
  { "dma_stat",        ENC(0,  1), 0, input_port, },
  { "ppi@0",           ENC(0,  2), 0, input_port, },
  { "sport@0_stat",    ENC(0,  3), 0, input_port, },
  { "sport@1_stat",    ENC(0,  4), 0, input_port, },
  { "uart2@0_stat",    ENC(0,  5), 0, input_port, },
  { "uart2@1_stat",    ENC(0,  6), 0, input_port, },
  { "spi@0",           ENC(0,  7), 0, input_port, },
  { "spi@1",           ENC(0,  8), 0, input_port, },
  { "can_stat",        ENC(0,  9), 0, input_port, },
  { "rsi_int0",        ENC(0, 10), 0, input_port, },
/*{ "reserved",        ENC(0, 11), 0, input_port, },*/
  { "counter@0",       ENC(0, 12), 0, input_port, },
  { "counter@1",       ENC(0, 13), 0, input_port, },
  { "dma@0",           ENC(0, 14), 0, input_port, },
  { "dma@1",           ENC(0, 15), 0, input_port, },
  { "dma@2",           ENC(0, 16), 0, input_port, },
  { "dma@3",           ENC(0, 17), 0, input_port, },
  { "dma@4",           ENC(0, 18), 0, input_port, },
  { "dma@5",           ENC(0, 19), 0, input_port, },
  { "dma@6",           ENC(0, 20), 0, input_port, },
  { "dma@7",           ENC(0, 21), 0, input_port, },
  { "dma@8",           ENC(0, 22), 0, input_port, },
  { "dma@9",           ENC(0, 23), 0, input_port, },
  { "dma@10",          ENC(0, 24), 0, input_port, },
  { "dma@11",          ENC(0, 25), 0, input_port, },
  { "can_rx",          ENC(0, 26), 0, input_port, },
  { "can_tx",          ENC(0, 27), 0, input_port, },
  { "twi@0",           ENC(0, 28), 0, input_port, },
  { "portf_irq_a",     ENC(0, 29), 0, input_port, },
  { "portf_irq_b",     ENC(0, 30), 0, input_port, },
/*{ "reserved",        ENC(0, 31), 0, input_port, },*/
  /* SIC1 */
  { "gptimer@0",       ENC(1,  0), 0, input_port, },
  { "gptimer@1",       ENC(1,  1), 0, input_port, },
  { "gptimer@2",       ENC(1,  2), 0, input_port, },
  { "gptimer@3",       ENC(1,  3), 0, input_port, },
  { "gptimer@4",       ENC(1,  4), 0, input_port, },
  { "gptimer@5",       ENC(1,  5), 0, input_port, },
  { "gptimer@6",       ENC(1,  6), 0, input_port, },
  { "gptimer@7",       ENC(1,  7), 0, input_port, },
  { "portg_irq_a",     ENC(1,  8), 0, input_port, },
  { "portg_irq_b",     ENC(1,  9), 0, input_port, },
  { "mdma@0",          ENC(1, 10), 0, input_port, },
  { "mdma@1",          ENC(1, 11), 0, input_port, },
  { "wdog",            ENC(1, 12), 0, input_port, },
  { "porth_irq_a",     ENC(1, 13), 0, input_port, },
  { "porth_irq_b",     ENC(1, 14), 0, input_port, },
  { "acm_stat",        ENC(1, 15), 0, input_port, },
  { "acm_int",         ENC(1, 16), 0, input_port, },
/*{ "reserved",        ENC(1, 17), 0, input_port, },*/
/*{ "reserved",        ENC(1, 18), 0, input_port, },*/
  { "pwm@0_trip",      ENC(1, 19), 0, input_port, },
  { "pwm@0_sync",      ENC(1, 20), 0, input_port, },
  { "pwm@1_trip",      ENC(1, 21), 0, input_port, },
  { "pwm@1_sync",      ENC(1, 22), 0, input_port, },
  { "rsi_int1",        ENC(1, 23), 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const struct hw_port_descriptor bfin_sic_51x_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  /* SIC0 */
  { "pll",             ENC(0,  0), 0, input_port, },
  { "dma_stat",        ENC(0,  1), 0, input_port, },
  { "dmar0_block",     ENC(0,  2), 0, input_port, },
  { "dmar1_block",     ENC(0,  3), 0, input_port, },
  { "dmar0_over",      ENC(0,  4), 0, input_port, },
  { "dmar1_over",      ENC(0,  5), 0, input_port, },
  { "ppi@0",           ENC(0,  6), 0, input_port, },
  { "emac_stat",       ENC(0,  7), 0, input_port, },
  { "sport@0_stat",    ENC(0,  8), 0, input_port, },
  { "sport@1_stat",    ENC(0,  9), 0, input_port, },
  { "ptp_err",         ENC(0, 10), 0, input_port, },
/*{ "reserved",        ENC(0, 11), 0, input_port, },*/
  { "uart@0_stat",     ENC(0, 12), 0, input_port, },
  { "uart@1_stat",     ENC(0, 13), 0, input_port, },
  { "rtc",             ENC(0, 14), 0, input_port, },
  { "dma@0",           ENC(0, 15), 0, input_port, },
  { "dma@3",           ENC(0, 16), 0, input_port, },
  { "dma@4",           ENC(0, 17), 0, input_port, },
  { "dma@5",           ENC(0, 18), 0, input_port, },
  { "dma@6",           ENC(0, 19), 0, input_port, },
  { "twi@0",           ENC(0, 20), 0, input_port, },
  { "dma@7",           ENC(0, 21), 0, input_port, },
  { "dma@8",           ENC(0, 22), 0, input_port, },
  { "dma@9",           ENC(0, 23), 0, input_port, },
  { "dma@10",          ENC(0, 24), 0, input_port, },
  { "dma@11",          ENC(0, 25), 0, input_port, },
  { "otp",             ENC(0, 26), 0, input_port, },
  { "counter",         ENC(0, 27), 0, input_port, },
  { "dma@1",           ENC(0, 28), 0, input_port, },
  { "porth_irq_a",     ENC(0, 29), 0, input_port, },
  { "dma@2",           ENC(0, 30), 0, input_port, },
  { "porth_irq_b",     ENC(0, 31), 0, input_port, },
  /* SIC1 */
  { "gptimer@0",       ENC(1,  0), 0, input_port, },
  { "gptimer@1",       ENC(1,  1), 0, input_port, },
  { "gptimer@2",       ENC(1,  2), 0, input_port, },
  { "gptimer@3",       ENC(1,  3), 0, input_port, },
  { "gptimer@4",       ENC(1,  4), 0, input_port, },
  { "gptimer@5",       ENC(1,  5), 0, input_port, },
  { "gptimer@6",       ENC(1,  6), 0, input_port, },
  { "gptimer@7",       ENC(1,  7), 0, input_port, },
  { "portg_irq_a",     ENC(1,  8), 0, input_port, },
  { "portg_irq_b",     ENC(1,  9), 0, input_port, },
  { "mdma@0",          ENC(1, 10), 0, input_port, },
  { "mdma@1",          ENC(1, 11), 0, input_port, },
  { "wdog",            ENC(1, 12), 0, input_port, },
  { "portf_irq_a",     ENC(1, 13), 0, input_port, },
  { "portf_irq_b",     ENC(1, 14), 0, input_port, },
  { "spi@0",           ENC(1, 15), 0, input_port, },
  { "spi@1",           ENC(1, 16), 0, input_port, },
/*{ "reserved",        ENC(1, 17), 0, input_port, },*/
/*{ "reserved",        ENC(1, 18), 0, input_port, },*/
  { "rsi_int0",        ENC(1, 19), 0, input_port, },
  { "rsi_int1",        ENC(1, 20), 0, input_port, },
  { "pwm_trip",        ENC(1, 21), 0, input_port, },
  { "pwm_sync",        ENC(1, 22), 0, input_port, },
  { "ptp_stat",        ENC(1, 23), 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const struct hw_port_descriptor bfin_sic_52x_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  /* SIC0 */
  { "pll",             ENC(0,  0), 0, input_port, },
  { "dma_stat",        ENC(0,  1), 0, input_port, },
  { "dmar0_block",     ENC(0,  2), 0, input_port, },
  { "dmar1_block",     ENC(0,  3), 0, input_port, },
  { "dmar0_over",      ENC(0,  4), 0, input_port, },
  { "dmar1_over",      ENC(0,  5), 0, input_port, },
  { "ppi@0",           ENC(0,  6), 0, input_port, },
  { "emac_stat",       ENC(0,  7), 0, input_port, },
  { "sport@0_stat",    ENC(0,  8), 0, input_port, },
  { "sport@1_stat",    ENC(0,  9), 0, input_port, },
/*{ "reserved",        ENC(0, 10), 0, input_port, },*/
/*{ "reserved",        ENC(0, 11), 0, input_port, },*/
  { "uart@0_stat",     ENC(0, 12), 0, input_port, },
  { "uart@1_stat",     ENC(0, 13), 0, input_port, },
  { "rtc",             ENC(0, 14), 0, input_port, },
  { "dma@0",           ENC(0, 15), 0, input_port, },
  { "dma@3",           ENC(0, 16), 0, input_port, },
  { "dma@4",           ENC(0, 17), 0, input_port, },
  { "dma@5",           ENC(0, 18), 0, input_port, },
  { "dma@6",           ENC(0, 19), 0, input_port, },
  { "twi@0",           ENC(0, 20), 0, input_port, },
  { "dma@7",           ENC(0, 21), 0, input_port, },
  { "dma@8",           ENC(0, 22), 0, input_port, },
  { "dma@9",           ENC(0, 23), 0, input_port, },
  { "dma@10",          ENC(0, 24), 0, input_port, },
  { "dma@11",          ENC(0, 25), 0, input_port, },
  { "otp",             ENC(0, 26), 0, input_port, },
  { "counter",         ENC(0, 27), 0, input_port, },
  { "dma@1",           ENC(0, 28), 0, input_port, },
  { "porth_irq_a",     ENC(0, 29), 0, input_port, },
  { "dma@2",           ENC(0, 30), 0, input_port, },
  { "porth_irq_b",     ENC(0, 31), 0, input_port, },
  /* SIC1 */
  { "gptimer@0",       ENC(1,  0), 0, input_port, },
  { "gptimer@1",       ENC(1,  1), 0, input_port, },
  { "gptimer@2",       ENC(1,  2), 0, input_port, },
  { "gptimer@3",       ENC(1,  3), 0, input_port, },
  { "gptimer@4",       ENC(1,  4), 0, input_port, },
  { "gptimer@5",       ENC(1,  5), 0, input_port, },
  { "gptimer@6",       ENC(1,  6), 0, input_port, },
  { "gptimer@7",       ENC(1,  7), 0, input_port, },
  { "portg_irq_a",     ENC(1,  8), 0, input_port, },
  { "portg_irq_b",     ENC(1,  9), 0, input_port, },
  { "mdma@0",          ENC(1, 10), 0, input_port, },
  { "mdma@1",          ENC(1, 11), 0, input_port, },
  { "wdog",            ENC(1, 12), 0, input_port, },
  { "portf_irq_a",     ENC(1, 13), 0, input_port, },
  { "portf_irq_b",     ENC(1, 14), 0, input_port, },
  { "spi@0",           ENC(1, 15), 0, input_port, },
  { "nfc_stat",        ENC(1, 16), 0, input_port, },
  { "hostdp_stat",     ENC(1, 17), 0, input_port, },
  { "hostdp_done",     ENC(1, 18), 0, input_port, },
  { "usb_int0",        ENC(1, 20), 0, input_port, },
  { "usb_int1",        ENC(1, 21), 0, input_port, },
  { "usb_int2",        ENC(1, 22), 0, input_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_sic_52x_port_event (struct hw *me, int my_port, struct hw *source,
			 int source_port, int level)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 idx = DEC_SIC (my_port);
  bu32 pin = DEC_PIN (my_port);
  bu32 bit = 1 << pin;

  HW_TRACE ((me, "processing system int from %i (SIC %u pin %u)",
	     my_port, idx, pin));

  /* SIC only exists to forward interrupts from the system to the CEC.  */
  switch (idx)
    {
    case 0: sic->bf52x.isr0 |= bit; break;
    case 1: sic->bf52x.isr1 |= bit; break;
    }

  /* XXX: Handle SIC wakeup source ?
  if (sic->bf52x.iwr0 & bit)
    What to do ?;
  if (sic->bf52x.iwr1 & bit)
    What to do ?;
   */

  bfin_sic_52x_forward_interrupts (me, sic);
}

static const struct hw_port_descriptor bfin_sic_533_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  { "pll",             ENC(0,  0), 0, input_port, },
  { "dma_stat",        ENC(0,  1), 0, input_port, },
  { "ppi@0",           ENC(0,  2), 0, input_port, },
  { "sport@0_stat",    ENC(0,  3), 0, input_port, },
  { "sport@1_stat",    ENC(0,  4), 0, input_port, },
  { "spi@0",           ENC(0,  5), 0, input_port, },
  { "uart@0_stat",     ENC(0,  6), 0, input_port, },
  { "rtc",             ENC(0,  7), 0, input_port, },
  { "dma@0",           ENC(0,  8), 0, input_port, },
  { "dma@1",           ENC(0,  9), 0, input_port, },
  { "dma@2",           ENC(0, 10), 0, input_port, },
  { "dma@3",           ENC(0, 11), 0, input_port, },
  { "dma@4",           ENC(0, 12), 0, input_port, },
  { "dma@5",           ENC(0, 13), 0, input_port, },
  { "dma@6",           ENC(0, 14), 0, input_port, },
  { "dma@7",           ENC(0, 15), 0, input_port, },
  { "gptimer@0",       ENC(0, 16), 0, input_port, },
  { "gptimer@1",       ENC(0, 17), 0, input_port, },
  { "gptimer@2",       ENC(0, 18), 0, input_port, },
  { "portf_irq_a",     ENC(0, 19), 0, input_port, },
  { "portf_irq_b",     ENC(0, 20), 0, input_port, },
  { "mdma@0",          ENC(0, 21), 0, input_port, },
  { "mdma@1",          ENC(0, 22), 0, input_port, },
  { "wdog",            ENC(0, 23), 0, input_port, },
  { NULL, 0, 0, 0, },
};

/* The encoding here is uglier due to multiple sources being muxed into
   the same interrupt line.  So give each pin an arbitrary "SIC" so that
   the resulting id is unique across all ports.  */
static const struct hw_port_descriptor bfin_sic_537_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  { "pll",             ENC(0,  0), 0, input_port, },
  { "dma_stat",        ENC(0,  1), 0, input_port, },
  { "dmar0_block",     ENC(1,  1), 0, input_port, },
  { "dmar1_block",     ENC(2,  1), 0, input_port, },
  { "dmar0_over",      ENC(3,  1), 0, input_port, },
  { "dmar1_over",      ENC(4,  1), 0, input_port, },
  { "can_stat",        ENC(0,  2), 0, input_port, },
  { "emac_stat",       ENC(1,  2), 0, input_port, },
  { "sport@0_stat",    ENC(2,  2), 0, input_port, },
  { "sport@1_stat",    ENC(3,  2), 0, input_port, },
  { "ppi@0",           ENC(4,  2), 0, input_port, },
  { "spi@0",           ENC(5,  2), 0, input_port, },
  { "uart@0_stat",     ENC(6,  2), 0, input_port, },
  { "uart@1_stat",     ENC(7,  2), 0, input_port, },
  { "rtc",             ENC(0,  3), 0, input_port, },
  { "dma@0",           ENC(0,  4), 0, input_port, },
  { "dma@3",           ENC(0,  5), 0, input_port, },
  { "dma@4",           ENC(0,  6), 0, input_port, },
  { "dma@5",           ENC(0,  7), 0, input_port, },
  { "dma@6",           ENC(0,  8), 0, input_port, },
  { "twi@0",           ENC(0,  9), 0, input_port, },
  { "dma@7",           ENC(0, 10), 0, input_port, },
  { "dma@8",           ENC(0, 11), 0, input_port, },
  { "dma@9",           ENC(0, 12), 0, input_port, },
  { "dma@10",          ENC(0, 13), 0, input_port, },
  { "dma@11",          ENC(0, 14), 0, input_port, },
  { "can_rx",          ENC(0, 15), 0, input_port, },
  { "can_tx",          ENC(0, 16), 0, input_port, },
  { "dma@1",           ENC(0, 17), 0, input_port, },
  { "porth_irq_a",     ENC(1, 17), 0, input_port, },
  { "dma@2",           ENC(0, 18), 0, input_port, },
  { "porth_irq_b",     ENC(1, 18), 0, input_port, },
  { "gptimer@0",       ENC(0, 19), 0, input_port, },
  { "gptimer@1",       ENC(0, 20), 0, input_port, },
  { "gptimer@2",       ENC(0, 21), 0, input_port, },
  { "gptimer@3",       ENC(0, 22), 0, input_port, },
  { "gptimer@4",       ENC(0, 23), 0, input_port, },
  { "gptimer@5",       ENC(0, 24), 0, input_port, },
  { "gptimer@6",       ENC(0, 25), 0, input_port, },
  { "gptimer@7",       ENC(0, 26), 0, input_port, },
  { "portf_irq_a",     ENC(0, 27), 0, input_port, },
  { "portg_irq_a",     ENC(1, 27), 0, input_port, },
  { "portg_irq_b",     ENC(0, 28), 0, input_port, },
  { "mdma@0",          ENC(0, 29), 0, input_port, },
  { "mdma@1",          ENC(0, 30), 0, input_port, },
  { "wdog",            ENC(0, 31), 0, input_port, },
  { "portf_irq_b",     ENC(1, 31), 0, input_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_sic_537_port_event (struct hw *me, int my_port, struct hw *source,
			 int source_port, int level)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 idx = DEC_SIC (my_port);
  bu32 pin = DEC_PIN (my_port);
  bu32 bit = 1 << pin;

  HW_TRACE ((me, "processing system int from %i (SIC %u pin %u)",
	     my_port, idx, pin));

  /* SIC only exists to forward interrupts from the system to the CEC.  */
  sic->bf537.isr |= bit;

  /* XXX: Handle SIC wakeup source ?
  if (sic->bf537.iwr & bit)
    What to do ?;
   */

  bfin_sic_537_forward_interrupts (me, sic);
}

static const struct hw_port_descriptor bfin_sic_538_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  /* SIC0 */
  { "pll",             ENC(0,  0), 0, input_port, },
  { "dmac@0_stat",     ENC(0,  1), 0, input_port, },
  { "ppi@0",           ENC(0,  2), 0, input_port, },
  { "sport@0_stat",    ENC(0,  3), 0, input_port, },
  { "sport@1_stat",    ENC(0,  4), 0, input_port, },
  { "spi@0",           ENC(0,  5), 0, input_port, },
  { "uart@0_stat",     ENC(0,  6), 0, input_port, },
  { "rtc",             ENC(0,  7), 0, input_port, },
  { "dma@0",           ENC(0,  8), 0, input_port, },
  { "dma@1",           ENC(0,  9), 0, input_port, },
  { "dma@2",           ENC(0, 10), 0, input_port, },
  { "dma@3",           ENC(0, 11), 0, input_port, },
  { "dma@4",           ENC(0, 12), 0, input_port, },
  { "dma@5",           ENC(0, 13), 0, input_port, },
  { "dma@6",           ENC(0, 14), 0, input_port, },
  { "dma@7",           ENC(0, 15), 0, input_port, },
  { "gptimer@0",       ENC(0, 16), 0, input_port, },
  { "gptimer@1",       ENC(0, 17), 0, input_port, },
  { "gptimer@2",       ENC(0, 18), 0, input_port, },
  { "portf_irq_a",     ENC(0, 19), 0, input_port, },
  { "portf_irq_b",     ENC(0, 20), 0, input_port, },
  { "mdma@0",          ENC(0, 21), 0, input_port, },
  { "mdma@1",          ENC(0, 22), 0, input_port, },
  { "wdog",            ENC(0, 23), 0, input_port, },
  { "dmac@1_stat",     ENC(0, 24), 0, input_port, },
  { "sport@2_stat",    ENC(0, 25), 0, input_port, },
  { "sport@3_stat",    ENC(0, 26), 0, input_port, },
/*{ "reserved",        ENC(0, 27), 0, input_port, },*/
  { "spi@1",           ENC(0, 28), 0, input_port, },
  { "spi@2",           ENC(0, 29), 0, input_port, },
  { "uart@1_stat",     ENC(0, 30), 0, input_port, },
  { "uart@2_stat",     ENC(0, 31), 0, input_port, },
  /* SIC1 */
  { "can_stat",        ENC(1,  0), 0, input_port, },
  { "dma@8",           ENC(1,  1), 0, input_port, },
  { "dma@9",           ENC(1,  2), 0, input_port, },
  { "dma@10",          ENC(1,  3), 0, input_port, },
  { "dma@11",          ENC(1,  4), 0, input_port, },
  { "dma@12",          ENC(1,  5), 0, input_port, },
  { "dma@13",          ENC(1,  6), 0, input_port, },
  { "dma@14",          ENC(1,  7), 0, input_port, },
  { "dma@15",          ENC(1,  8), 0, input_port, },
  { "dma@16",          ENC(1,  9), 0, input_port, },
  { "dma@17",          ENC(1, 10), 0, input_port, },
  { "dma@18",          ENC(1, 11), 0, input_port, },
  { "dma@19",          ENC(1, 12), 0, input_port, },
  { "twi@0",           ENC(1, 13), 0, input_port, },
  { "twi@1",           ENC(1, 14), 0, input_port, },
  { "can_rx",          ENC(1, 15), 0, input_port, },
  { "can_tx",          ENC(1, 16), 0, input_port, },
  { "mdma@2",          ENC(1, 17), 0, input_port, },
  { "mdma@3",          ENC(1, 18), 0, input_port, },
  { NULL, 0, 0, 0, },
};

static const struct hw_port_descriptor bfin_sic_54x_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  /* SIC0 */
  { "pll",             ENC(0,  0), 0, input_port, },
  { "dmac@0_stat",     ENC(0,  1), 0, input_port, },
  { "eppi@0",          ENC(0,  2), 0, input_port, },
  { "sport@0_stat",    ENC(0,  3), 0, input_port, },
  { "sport@1_stat",    ENC(0,  4), 0, input_port, },
  { "spi@0",           ENC(0,  5), 0, input_port, },
  { "uart2@0_stat",    ENC(0,  6), 0, input_port, },
  { "rtc",             ENC(0,  7), 0, input_port, },
  { "dma@12",          ENC(0,  8), 0, input_port, },
  { "dma@0",           ENC(0,  9), 0, input_port, },
  { "dma@1",           ENC(0, 10), 0, input_port, },
  { "dma@2",           ENC(0, 11), 0, input_port, },
  { "dma@3",           ENC(0, 12), 0, input_port, },
  { "dma@4",           ENC(0, 13), 0, input_port, },
  { "dma@6",           ENC(0, 14), 0, input_port, },
  { "dma@7",           ENC(0, 15), 0, input_port, },
  { "gptimer@8",       ENC(0, 16), 0, input_port, },
  { "gptimer@9",       ENC(0, 17), 0, input_port, },
  { "gptimer@10",      ENC(0, 18), 0, input_port, },
  { "pint@0",          ENC(0, 19), 0, input_port, },
  { "pint@1",          ENC(0, 20), 0, input_port, },
  { "mdma@0",          ENC(0, 21), 0, input_port, },
  { "mdma@1",          ENC(0, 22), 0, input_port, },
  { "wdog",            ENC(0, 23), 0, input_port, },
  { "dmac@1_stat",     ENC(0, 24), 0, input_port, },
  { "sport@2_stat",    ENC(0, 25), 0, input_port, },
  { "sport@3_stat",    ENC(0, 26), 0, input_port, },
  { "mxvr",            ENC(0, 27), 0, input_port, },
  { "spi@1",           ENC(0, 28), 0, input_port, },
  { "spi@2",           ENC(0, 29), 0, input_port, },
  { "uart2@1_stat",    ENC(0, 30), 0, input_port, },
  { "uart2@2_stat",    ENC(0, 31), 0, input_port, },
  /* SIC1 */
  { "can@0_stat",      ENC(1,  0), 0, input_port, },
  { "dma@18",          ENC(1,  1), 0, input_port, },
  { "dma@19",          ENC(1,  2), 0, input_port, },
  { "dma@20",          ENC(1,  3), 0, input_port, },
  { "dma@21",          ENC(1,  4), 0, input_port, },
  { "dma@13",          ENC(1,  5), 0, input_port, },
  { "dma@14",          ENC(1,  6), 0, input_port, },
  { "dma@5",           ENC(1,  7), 0, input_port, },
  { "dma@23",          ENC(1,  8), 0, input_port, },
  { "dma@8",           ENC(1,  9), 0, input_port, },
  { "dma@9",           ENC(1, 10), 0, input_port, },
  { "dma@10",          ENC(1, 11), 0, input_port, },
  { "dma@11",          ENC(1, 12), 0, input_port, },
  { "twi@0",           ENC(1, 13), 0, input_port, },
  { "twi@1",           ENC(1, 14), 0, input_port, },
  { "can@0_rx",        ENC(1, 15), 0, input_port, },
  { "can@0_tx",        ENC(1, 16), 0, input_port, },
  { "mdma@2",          ENC(1, 17), 0, input_port, },
  { "mdma@3",          ENC(1, 18), 0, input_port, },
  { "mxvr_stat",       ENC(1, 19), 0, input_port, },
  { "mxvr_message",    ENC(1, 20), 0, input_port, },
  { "mxvr_packet",     ENC(1, 21), 0, input_port, },
  { "eppi@1",          ENC(1, 22), 0, input_port, },
  { "eppi@2",          ENC(1, 23), 0, input_port, },
  { "uart2@3_stat",    ENC(1, 24), 0, input_port, },
  { "hostdp",          ENC(1, 25), 0, input_port, },
/*{ "reserved",        ENC(1, 26), 0, input_port, },*/
  { "pixc_stat",       ENC(1, 27), 0, input_port, },
  { "nfc",             ENC(1, 28), 0, input_port, },
  { "atapi",           ENC(1, 29), 0, input_port, },
  { "can@1_stat",      ENC(1, 30), 0, input_port, },
  { "dmar",            ENC(1, 31), 0, input_port, },
  /* SIC2 */
  { "dma@15",          ENC(2,  0), 0, input_port, },
  { "dma@16",          ENC(2,  1), 0, input_port, },
  { "dma@17",          ENC(2,  2), 0, input_port, },
  { "dma@22",          ENC(2,  3), 0, input_port, },
  { "counter",         ENC(2,  4), 0, input_port, },
  { "key",             ENC(2,  5), 0, input_port, },
  { "can@1_rx",        ENC(2,  6), 0, input_port, },
  { "can@1_tx",        ENC(2,  7), 0, input_port, },
  { "sdh_mask0",       ENC(2,  8), 0, input_port, },
  { "sdh_mask1",       ENC(2,  9), 0, input_port, },
/*{ "reserved",        ENC(2, 10), 0, input_port, },*/
  { "usb_int0",        ENC(2, 11), 0, input_port, },
  { "usb_int1",        ENC(2, 12), 0, input_port, },
  { "usb_int2",        ENC(2, 13), 0, input_port, },
  { "usb_dma",         ENC(2, 14), 0, input_port, },
  { "otpsec",          ENC(2, 15), 0, input_port, },
/*{ "reserved",        ENC(2, 16), 0, input_port, },*/
/*{ "reserved",        ENC(2, 17), 0, input_port, },*/
/*{ "reserved",        ENC(2, 18), 0, input_port, },*/
/*{ "reserved",        ENC(2, 19), 0, input_port, },*/
/*{ "reserved",        ENC(2, 20), 0, input_port, },*/
/*{ "reserved",        ENC(2, 21), 0, input_port, },*/
  { "gptimer@0",       ENC(2, 22), 0, input_port, },
  { "gptimer@1",       ENC(2, 23), 0, input_port, },
  { "gptimer@2",       ENC(2, 24), 0, input_port, },
  { "gptimer@3",       ENC(2, 25), 0, input_port, },
  { "gptimer@4",       ENC(2, 26), 0, input_port, },
  { "gptimer@5",       ENC(2, 27), 0, input_port, },
  { "gptimer@6",       ENC(2, 28), 0, input_port, },
  { "gptimer@7",       ENC(2, 29), 0, input_port, },
  { "pint2",           ENC(2, 30), 0, input_port, },
  { "pint3",           ENC(2, 31), 0, input_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_sic_54x_port_event (struct hw *me, int my_port, struct hw *source,
			 int source_port, int level)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 idx = DEC_SIC (my_port);
  bu32 pin = DEC_PIN (my_port);
  bu32 bit = 1 << pin;

  HW_TRACE ((me, "processing system int from %i (SIC %u pin %u)",
	     my_port, idx, pin));

  /* SIC only exists to forward interrupts from the system to the CEC.  */
  switch (idx)
    {
    case 0: sic->bf54x.isr0 |= bit; break;
    case 1: sic->bf54x.isr1 |= bit; break;
    case 2: sic->bf54x.isr2 |= bit; break;
    }

  /* XXX: Handle SIC wakeup source ?
  if (sic->bf54x.iwr0 & bit)
    What to do ?;
  if (sic->bf54x.iwr1 & bit)
    What to do ?;
  if (sic->bf54x.iwr2 & bit)
    What to do ?;
   */

  bfin_sic_54x_forward_interrupts (me, sic);
}

static const struct hw_port_descriptor bfin_sic_561_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  /* SIC0 */
  { "pll",             ENC(0,  0), 0, input_port, },
  { "dmac@0_stat",     ENC(0,  1), 0, input_port, },
  { "dmac@1_stat",     ENC(0,  2), 0, input_port, },
  { "imdma_stat",      ENC(0,  3), 0, input_port, },
  { "ppi@0",           ENC(0,  4), 0, input_port, },
  { "ppi@1",           ENC(0,  5), 0, input_port, },
  { "sport@0_stat",    ENC(0,  6), 0, input_port, },
  { "sport@1_stat",    ENC(0,  7), 0, input_port, },
  { "spi@0",           ENC(0,  8), 0, input_port, },
  { "uart@0_stat",     ENC(0,  9), 0, input_port, },
/*{ "reserved",        ENC(0, 10), 0, input_port, },*/
  { "dma@12",          ENC(0, 11), 0, input_port, },
  { "dma@13",          ENC(0, 12), 0, input_port, },
  { "dma@14",          ENC(0, 13), 0, input_port, },
  { "dma@15",          ENC(0, 14), 0, input_port, },
  { "dma@16",          ENC(0, 15), 0, input_port, },
  { "dma@17",          ENC(0, 16), 0, input_port, },
  { "dma@18",          ENC(0, 17), 0, input_port, },
  { "dma@19",          ENC(0, 18), 0, input_port, },
  { "dma@20",          ENC(0, 19), 0, input_port, },
  { "dma@21",          ENC(0, 20), 0, input_port, },
  { "dma@22",          ENC(0, 21), 0, input_port, },
  { "dma@23",          ENC(0, 22), 0, input_port, },
  { "dma@0",           ENC(0, 23), 0, input_port, },
  { "dma@1",           ENC(0, 24), 0, input_port, },
  { "dma@2",           ENC(0, 25), 0, input_port, },
  { "dma@3",           ENC(0, 26), 0, input_port, },
  { "dma@4",           ENC(0, 27), 0, input_port, },
  { "dma@5",           ENC(0, 28), 0, input_port, },
  { "dma@6",           ENC(0, 29), 0, input_port, },
  { "dma@7",           ENC(0, 30), 0, input_port, },
  { "dma@8",           ENC(0, 31), 0, input_port, },
  /* SIC1 */
  { "dma@9",           ENC(1,  0), 0, input_port, },
  { "dma@10",          ENC(1,  1), 0, input_port, },
  { "dma@11",          ENC(1,  2), 0, input_port, },
  { "gptimer@0",       ENC(1,  3), 0, input_port, },
  { "gptimer@1",       ENC(1,  4), 0, input_port, },
  { "gptimer@2",       ENC(1,  5), 0, input_port, },
  { "gptimer@3",       ENC(1,  6), 0, input_port, },
  { "gptimer@4",       ENC(1,  7), 0, input_port, },
  { "gptimer@5",       ENC(1,  8), 0, input_port, },
  { "gptimer@6",       ENC(1,  9), 0, input_port, },
  { "gptimer@7",       ENC(1, 10), 0, input_port, },
  { "gptimer@8",       ENC(1, 11), 0, input_port, },
  { "gptimer@9",       ENC(1, 12), 0, input_port, },
  { "gptimer@10",      ENC(1, 13), 0, input_port, },
  { "gptimer@11",      ENC(1, 14), 0, input_port, },
  { "portf_irq_a",     ENC(1, 15), 0, input_port, },
  { "portf_irq_b",     ENC(1, 16), 0, input_port, },
  { "portg_irq_a",     ENC(1, 17), 0, input_port, },
  { "portg_irq_b",     ENC(1, 18), 0, input_port, },
  { "porth_irq_a",     ENC(1, 19), 0, input_port, },
  { "porth_irq_b",     ENC(1, 20), 0, input_port, },
  { "mdma@0",          ENC(1, 21), 0, input_port, },
  { "mdma@1",          ENC(1, 22), 0, input_port, },
  { "mdma@2",          ENC(1, 23), 0, input_port, },
  { "mdma@3",          ENC(1, 24), 0, input_port, },
  { "imdma@0",         ENC(1, 25), 0, input_port, },
  { "imdma@1",         ENC(1, 26), 0, input_port, },
  { "wdog",            ENC(1, 27), 0, input_port, },
/*{ "reserved",        ENC(1, 28), 0, input_port, },*/
/*{ "reserved",        ENC(1, 29), 0, input_port, },*/
  { "sup_irq_0",       ENC(1, 30), 0, input_port, },
  { "sup_irq_1",       ENC(1, 31), 0, input_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_sic_561_port_event (struct hw *me, int my_port, struct hw *source,
			 int source_port, int level)
{
  struct bfin_sic *sic = hw_data (me);
  bu32 idx = DEC_SIC (my_port);
  bu32 pin = DEC_PIN (my_port);
  bu32 bit = 1 << pin;

  HW_TRACE ((me, "processing system int from %i (SIC %u pin %u)",
	     my_port, idx, pin));

  /* SIC only exists to forward interrupts from the system to the CEC.  */
  switch (idx)
    {
    case 0: sic->bf561.isr0 |= bit; break;
    case 1: sic->bf561.isr1 |= bit; break;
    }

  /* XXX: Handle SIC wakeup source ?
  if (sic->bf561.iwr0 & bit)
    What to do ?;
  if (sic->bf561.iwr1 & bit)
    What to do ?;
   */

  bfin_sic_561_forward_interrupts (me, sic);
}

static const struct hw_port_descriptor bfin_sic_59x_ports[] =
{
  BFIN_SIC_TO_CEC_PORTS
  { "pll",             ENC(0,  0), 0, input_port, },
  { "dma_stat",        ENC(0,  1), 0, input_port, },
  { "ppi@0",           ENC(0,  2), 0, input_port, },
  { "sport@0_stat",    ENC(0,  3), 0, input_port, },
  { "sport@1_stat",    ENC(0,  4), 0, input_port, },
  { "spi@0",           ENC(0,  5), 0, input_port, },
  { "spi@1",           ENC(0,  6), 0, input_port, },
  { "uart@0_stat",     ENC(0,  7), 0, input_port, },
  { "dma@0",           ENC(0,  8), 0, input_port, },
  { "dma@1",           ENC(0,  9), 0, input_port, },
  { "dma@2",           ENC(0, 10), 0, input_port, },
  { "dma@3",           ENC(0, 11), 0, input_port, },
  { "dma@4",           ENC(0, 12), 0, input_port, },
  { "dma@5",           ENC(0, 13), 0, input_port, },
  { "dma@6",           ENC(0, 14), 0, input_port, },
  { "dma@7",           ENC(0, 15), 0, input_port, },
  { "dma@8",           ENC(0, 16), 0, input_port, },
  { "portf_irq_a",     ENC(0, 17), 0, input_port, },
  { "portf_irq_b",     ENC(0, 18), 0, input_port, },
  { "gptimer@0",       ENC(0, 19), 0, input_port, },
  { "gptimer@1",       ENC(0, 20), 0, input_port, },
  { "gptimer@2",       ENC(0, 21), 0, input_port, },
  { "portg_irq_a",     ENC(0, 22), 0, input_port, },
  { "portg_irq_b",     ENC(0, 23), 0, input_port, },
  { "twi@0",           ENC(0, 24), 0, input_port, },
/* XXX: 25 - 28 are supposed to be reserved; see comment in machs.c:bf592_dmac[]  */
  { "dma@9",           ENC(0, 25), 0, input_port, },
  { "dma@10",          ENC(0, 26), 0, input_port, },
  { "dma@11",          ENC(0, 27), 0, input_port, },
  { "dma@12",          ENC(0, 28), 0, input_port, },
/*{ "reserved",        ENC(0, 25), 0, input_port, },*/
/*{ "reserved",        ENC(0, 26), 0, input_port, },*/
/*{ "reserved",        ENC(0, 27), 0, input_port, },*/
/*{ "reserved",        ENC(0, 28), 0, input_port, },*/
  { "mdma@0",          ENC(0, 29), 0, input_port, },
  { "mdma@1",          ENC(0, 30), 0, input_port, },
  { "wdog",            ENC(0, 31), 0, input_port, },
  { NULL, 0, 0, 0, },
};

static void
attach_bfin_sic_regs (struct hw *me, struct bfin_sic *sic)
{
  address_word attach_address;
  int attach_space;
  unsigned attach_size;
  reg_property_spec reg;

  if (hw_find_property (me, "reg") == NULL)
    hw_abort (me, "Missing \"reg\" property");

  if (!hw_find_reg_array_property (me, "reg", 0, &reg))
    hw_abort (me, "\"reg\" property must contain three addr/size entries");

  hw_unit_address_to_attach_address (hw_parent (me),
				     &reg.address,
				     &attach_space, &attach_address, me);
  hw_unit_size_to_attach_size (hw_parent (me), &reg.size, &attach_size, me);

  if (attach_size != BFIN_MMR_SIC_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_SIC_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  sic->base = attach_address;
}

static void
bfin_sic_finish (struct hw *me)
{
  struct bfin_sic *sic;

  sic = HW_ZALLOC (me, struct bfin_sic);

  set_hw_data (me, sic);
  attach_bfin_sic_regs (me, sic);

  switch (hw_find_integer_property (me, "type"))
    {
    case 500 ... 509:
      set_hw_io_read_buffer (me, bfin_sic_52x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_52x_io_write_buffer);
      set_hw_ports (me, bfin_sic_50x_ports);
      set_hw_port_event (me, bfin_sic_52x_port_event);
      mmr_names = bf52x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf52x.imask0 = sic->bf52x.imask1 = 0;
      sic->bf52x.isr0 = sic->bf52x.isr1 = 0;
      sic->bf52x.iwr0 = sic->bf52x.iwr1 = 0xFFFFFFFF;
      sic->bf52x.iar0 = 0x00000000;
      sic->bf52x.iar1 = 0x22111000;
      sic->bf52x.iar2 = 0x33332222;
      sic->bf52x.iar3 = 0x44444433;
      sic->bf52x.iar4 = 0x55555555;
      sic->bf52x.iar5 = 0x06666655;
      sic->bf52x.iar6 = 0x33333003;
      sic->bf52x.iar7 = 0x00000000;	/* XXX: Find and fix */
      break;
    case 510 ... 519:
      set_hw_io_read_buffer (me, bfin_sic_52x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_52x_io_write_buffer);
      set_hw_ports (me, bfin_sic_51x_ports);
      set_hw_port_event (me, bfin_sic_52x_port_event);
      mmr_names = bf52x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf52x.imask0 = sic->bf52x.imask1 = 0;
      sic->bf52x.isr0 = sic->bf52x.isr1 = 0;
      sic->bf52x.iwr0 = sic->bf52x.iwr1 = 0xFFFFFFFF;
      sic->bf52x.iar0 = 0x00000000;
      sic->bf52x.iar1 = 0x11000000;
      sic->bf52x.iar2 = 0x33332222;
      sic->bf52x.iar3 = 0x44444433;
      sic->bf52x.iar4 = 0x55555555;
      sic->bf52x.iar5 = 0x06666655;
      sic->bf52x.iar6 = 0x33333000;
      sic->bf52x.iar7 = 0x00000000;	/* XXX: Find and fix */
      break;
    case 522 ... 527:
      set_hw_io_read_buffer (me, bfin_sic_52x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_52x_io_write_buffer);
      set_hw_ports (me, bfin_sic_52x_ports);
      set_hw_port_event (me, bfin_sic_52x_port_event);
      mmr_names = bf52x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf52x.imask0 = sic->bf52x.imask1 = 0;
      sic->bf52x.isr0 = sic->bf52x.isr1 = 0;
      sic->bf52x.iwr0 = sic->bf52x.iwr1 = 0xFFFFFFFF;
      sic->bf52x.iar0 = 0x00000000;
      sic->bf52x.iar1 = 0x11000000;
      sic->bf52x.iar2 = 0x33332222;
      sic->bf52x.iar3 = 0x44444433;
      sic->bf52x.iar4 = 0x55555555;
      sic->bf52x.iar5 = 0x06666655;
      sic->bf52x.iar6 = 0x33333000;
      sic->bf52x.iar7 = 0x00000000;	/* XXX: Find and fix */
      break;
    case 531 ... 533:
      set_hw_io_read_buffer (me, bfin_sic_537_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_537_io_write_buffer);
      set_hw_ports (me, bfin_sic_533_ports);
      set_hw_port_event (me, bfin_sic_537_port_event);
      mmr_names = bf537_mmr_names;

      /* Initialize the SIC.  */
      sic->bf537.imask = 0;
      sic->bf537.isr = 0;
      sic->bf537.iwr = 0xFFFFFFFF;
      sic->bf537.iar0 = 0x10000000;
      sic->bf537.iar1 = 0x33322221;
      sic->bf537.iar2 = 0x66655444;
      sic->bf537.iar3 = 0; /* XXX: fix this */
      break;
    case 534:
    case 536:
    case 537:
      set_hw_io_read_buffer (me, bfin_sic_537_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_537_io_write_buffer);
      set_hw_ports (me, bfin_sic_537_ports);
      set_hw_port_event (me, bfin_sic_537_port_event);
      mmr_names = bf537_mmr_names;

      /* Initialize the SIC.  */
      sic->bf537.imask = 0;
      sic->bf537.isr = 0;
      sic->bf537.iwr = 0xFFFFFFFF;
      sic->bf537.iar0 = 0x22211000;
      sic->bf537.iar1 = 0x43333332;
      sic->bf537.iar2 = 0x55555444;
      sic->bf537.iar3 = 0x66655555;
      break;
    case 538 ... 539:
      set_hw_io_read_buffer (me, bfin_sic_52x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_52x_io_write_buffer);
      set_hw_ports (me, bfin_sic_538_ports);
      set_hw_port_event (me, bfin_sic_52x_port_event);
      mmr_names = bf52x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf52x.imask0 = sic->bf52x.imask1 = 0;
      sic->bf52x.isr0 = sic->bf52x.isr1 = 0;
      sic->bf52x.iwr0 = sic->bf52x.iwr1 = 0xFFFFFFFF;
      sic->bf52x.iar0 = 0x10000000;
      sic->bf52x.iar1 = 0x33322221;
      sic->bf52x.iar2 = 0x66655444;
      sic->bf52x.iar3 = 0x00000000;
      sic->bf52x.iar4 = 0x32222220;
      sic->bf52x.iar5 = 0x44433333;
      sic->bf52x.iar6 = 0x00444664;
      sic->bf52x.iar7 = 0x00000000;	/* XXX: Find and fix */
      break;
    case 540 ... 549:
      set_hw_io_read_buffer (me, bfin_sic_54x_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_54x_io_write_buffer);
      set_hw_ports (me, bfin_sic_54x_ports);
      set_hw_port_event (me, bfin_sic_54x_port_event);
      mmr_names = bf54x_mmr_names;

      /* Initialize the SIC.  */
      sic->bf54x.imask0 = sic->bf54x.imask1 = sic->bf54x.imask2 = 0;
      sic->bf54x.isr0 = sic->bf54x.isr1 = sic->bf54x.isr2 = 0;
      sic->bf54x.iwr0 = sic->bf54x.iwr1 = sic->bf54x.iwr1 = 0xFFFFFFFF;
      sic->bf54x.iar0 = 0x10000000;
      sic->bf54x.iar1 = 0x33322221;
      sic->bf54x.iar2 = 0x66655444;
      sic->bf54x.iar3 = 0x00000000;
      sic->bf54x.iar4 = 0x32222220;
      sic->bf54x.iar5 = 0x44433333;
      sic->bf54x.iar6 = 0x00444664;
      sic->bf54x.iar7 = 0x00000000;
      sic->bf54x.iar8 = 0x44111111;
      sic->bf54x.iar9 = 0x44444444;
      sic->bf54x.iar10 = 0x44444444;
      sic->bf54x.iar11 = 0x55444444;
      break;
    case 561:
      set_hw_io_read_buffer (me, bfin_sic_561_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_561_io_write_buffer);
      set_hw_ports (me, bfin_sic_561_ports);
      set_hw_port_event (me, bfin_sic_561_port_event);
      mmr_names = bf561_mmr_names;

      /* Initialize the SIC.  */
      sic->bf561.imask0 = sic->bf561.imask1 = 0;
      sic->bf561.isr0 = sic->bf561.isr1 = 0;
      sic->bf561.iwr0 = sic->bf561.iwr1 = 0xFFFFFFFF;
      sic->bf561.iar0 = 0x00000000;
      sic->bf561.iar1 = 0x11111000;
      sic->bf561.iar2 = 0x21111111;
      sic->bf561.iar3 = 0x22222222;
      sic->bf561.iar4 = 0x33333222;
      sic->bf561.iar5 = 0x43333333;
      sic->bf561.iar6 = 0x21144444;
      sic->bf561.iar7 = 0x00006552;
      break;
    case 590 ... 599:
      set_hw_io_read_buffer (me, bfin_sic_537_io_read_buffer);
      set_hw_io_write_buffer (me, bfin_sic_537_io_write_buffer);
      set_hw_ports (me, bfin_sic_59x_ports);
      set_hw_port_event (me, bfin_sic_537_port_event);
      mmr_names = bf537_mmr_names;

      /* Initialize the SIC.  */
      sic->bf537.imask = 0;
      sic->bf537.isr = 0;
      sic->bf537.iwr = 0xFFFFFFFF;
      sic->bf537.iar0 = 0x00000000;
      sic->bf537.iar1 = 0x33322221;
      sic->bf537.iar2 = 0x55444443;
      sic->bf537.iar3 = 0x66600005;
      break;
    default:
      hw_abort (me, "no support for SIC on this Blackfin model yet");
    }
}

const struct hw_descriptor dv_bfin_sic_descriptor[] =
{
  {"bfin_sic", bfin_sic_finish,},
  {NULL, NULL},
};
