/* 
 * Copyright (C) 1995 Advanced RISC Machines Limited. All rights reserved.
 * 
 * This software may be freely used, copied, modified, and distributed
 * provided that the above copyright notice is preserved in all copies of the
 * software.
 */

/* -*-C-*-
 *
 * $Revision: 1.1.1.1 $
 *     $Date: 1999/02/10 22:09:21 $
 *
 */
#include <stdio.h>
#include <stdlib.h>

#include "adp.h"
#include "hsys.h"
#include "rxtx.h"
#include "drivers.h"
#include "buffers.h"
#include "devclnt.h"
#include "adperr.h"
#include "devsw.h"
#include "hostchan.h"
#include "logging.h"

/*
 * TODO: this should be adjustable - it could be done by defining
 *       a reason code for DevSW_Ioctl.  It could even be a
 *       per-devicechannel parameter.
 */
static const unsigned int allocsize = ADP_BUFFER_MIN_SIZE;

#define illegalDevChanID(type)  ((type) >= DC_NUM_CHANNELS)

/**********************************************************************/

/*
 *  Function: initialise_read
 *   Purpose: Set up a read request for another packet
 *
 *    Params:
 *      In/Out: ds      State structure to be initialised
 *
 *   Returns:
 *          OK: 0
 *       Error: -1
 */
static int initialise_read(DevSWState *ds)
{
    struct data_packet *dp;

    /*
     * try to claim the structure that will
     * eventually hold the new packet.
     */
    if ((ds->ds_nextreadpacket = DevSW_AllocatePacket(allocsize)) == NULL)
        return -1;

    /*
     * Calls into the device driver use the DriverCall structure: use
     * the buffer we have just allocated, and declare its size.  We
     * are also obliged to clear the driver's context pointer.
     */
    dp = &ds->ds_activeread.dc_packet;
    dp->buf_len = allocsize;
    dp->data = ds->ds_nextreadpacket->pk_buffer;

    ds->ds_activeread.dc_context = NULL;

    return 0;
}

/*
 *  Function: initialise_write
 *   Purpose: Set up a write request for another packet
 *
 *    Params:
 *       Input: packet  The packet to be written
 *
 *              type    The type of the packet
 *
 *      In/Out: dc      The structure to be intialised
 *
 *   Returns: Nothing
 */
static void initialise_write(DriverCall *dc, Packet *packet, DevChanID type)
{
    struct data_packet *dp = &dc->dc_packet;

    dp->len = packet->pk_length;
    dp->data = packet->pk_buffer;
    dp->type = type;

    /*
     * we are required to clear the state structure for the driver
     */
    dc->dc_context = NULL;
}

/*
 *  Function: enqueue_packet
 *   Purpose: move a newly read packet onto the appropriate queue
 *              of read packets
 *
 *    Params:
 *      In/Out: ds      State structure with new packet
 *
 *   Returns: Nothing
 */
static void enqueue_packet(DevSWState *ds)
{
    struct data_packet *dp = &ds->ds_activeread.dc_packet;
    Packet *packet = ds->ds_nextreadpacket;

    /*
     * transfer the length
     */
    packet->pk_length = dp->len;

    /*
     * take this packet out of the incoming slot
     */
    ds->ds_nextreadpacket = NULL;

    /*
     * try to put it on the correct input queue
     */
    if (illegalDevChanID(dp->type))
    {
        /* this shouldn't happen */
        WARN("Illegal type for Rx packet");
        DevSW_FreePacket(packet);
    }
    else
        Adp_addToQueue(&ds->ds_readqueue[dp->type], packet);
}

/*
 *  Function: flush_packet
 *   Purpose: Send a packet to the device driver
 *
 *    Params:
 *       Input: device  The device to be written to
 *
 *      In/Out: dc      Describes the packet to be sent
 *
 *   Returns: Nothing
 *
 * Post-conditions: If the whole packet was accepted by the device
 *                      driver, then dc->dc_packet.data will be
 *                      set to NULL.
 */
static void flush_packet(const DeviceDescr *device, DriverCall *dc)
{
    if (device->DeviceWrite(dc) > 0)
        /*
         * the whole packet was swallowed
         */
        dc->dc_packet.data = NULL;
}

/**********************************************************************/

/*
 * These are the externally visible functions.  They are documented in
 * devsw.h
 */
Packet *DevSW_AllocatePacket(const unsigned int length)
{
    Packet *pk;

    if ((pk = malloc(sizeof(*pk))) == NULL)
    {
        WARN("malloc failure");
        return NULL;
    }

    if ((pk->pk_buffer = malloc(length+CHAN_HEADER_SIZE)) == NULL)
    {
        WARN("malloc failure");
        free(pk);
        return NULL;
    }

    return pk;
}

void DevSW_FreePacket(Packet *pk)
{
    free(pk->pk_buffer);
    free(pk);
}

AdpErrs DevSW_Open(DeviceDescr *device, const char *name, const char *arg,
                   const DevChanID type)
{
    DevSWState *ds;

    /*
     * is this the very first open call for this driver?
     */
    if ((ds = (DevSWState *)(device->SwitcherState)) == NULL)
    {
        /*
         * yes, it is: initialise state
         */
        if ((ds = malloc(sizeof(*ds))) == NULL)
            /* give up */
            return adp_malloc_failure;

        (void)memset(ds, 0, sizeof(*ds));
        device->SwitcherState = (void *)ds;
    }

    /*
     * check that we haven't already been opened for this type
     */
    if ((ds->ds_opendevchans & (1 << type)) != 0)
        return adp_device_already_open;

    /*
     * if no opens have been done for this device, then do it now
     */
    if (ds->ds_opendevchans == 0)
        if (device->DeviceOpen(name, arg) < 0)
            return adp_device_open_failed;

    /*
     * open has finished
     */
    ds->ds_opendevchans |= (1 << type);
    return adp_ok;
}

AdpErrs DevSW_Match(const DeviceDescr *device, const char *name,
                    const char *arg)
{
    return (device->DeviceMatch(name, arg) == -1) ? adp_failed : adp_ok;
}

AdpErrs DevSW_Close(const DeviceDescr *device, const DevChanID type)
{
    DevSWState *ds = (DevSWState *)(device->SwitcherState);
    Packet *pk;

    if ((ds->ds_opendevchans & (1 << type)) == 0)
        return adp_device_not_open;

    ds->ds_opendevchans &= ~(1 << type);

    /*
     * if this is the last close for this channel, then inform the driver
     */
    if (ds->ds_opendevchans == 0)
        device->DeviceClose();

    /*
     * release all packets of the appropriate type
     */
    for (pk = Adp_removeFromQueue(&(ds->ds_readqueue[type]));
         pk != NULL;
         pk = Adp_removeFromQueue(&(ds->ds_readqueue[type])))
        DevSW_FreePacket(pk);

    /* that's all */
    return adp_ok;
}

AdpErrs DevSW_Read(const DeviceDescr *device, const DevChanID type,
                   Packet **packet, bool block)
{
  int read_err;
  DevSWState *ds = device->SwitcherState;

    /*
     * To try to get information out of the device driver as
     * quickly as possible, we try and read more packets, even
     * if a completed packet is already available.
     */

    /*
     * have we got a packet currently pending?
     */
  if (ds->ds_nextreadpacket == NULL)
    /*
       * no - set things up
       */
    if (initialise_read(ds) < 0) {
      /*
       * we failed to initialise the next packet, but can
       * still return a packet that has already arrived.
       */
      *packet = Adp_removeFromQueue(&ds->ds_readqueue[type]); 
      return adp_ok;
    }
  read_err = device->DeviceRead(&ds->ds_activeread, block);
  switch (read_err) {
  case 1:
    /*
     * driver has pulled in a complete packet, queue it up
     */
#ifdef RET_DEBUG
    printf("got a complete packet\n");
#endif
    enqueue_packet(ds);
    *packet = Adp_removeFromQueue(&ds->ds_readqueue[type]);
    return adp_ok;
  case 0:
    /*
     * OK, return the head of the read queue for the given type
     */
    /*    enqueue_packet(ds); */
    *packet = Adp_removeFromQueue(&ds->ds_readqueue[type]);
    return adp_ok;
  case -1:
#ifdef RET_DEBUG
    printf("got a bad packet\n");
#endif
    /* bad packet */
    *packet = NULL;
    return adp_bad_packet;
  default:
    panic("DevSW_Read: bad read status %d", read_err);
  }
  return 0; /* get rid of a potential compiler warning */
}


AdpErrs DevSW_FlushPendingWrite(const DeviceDescr *device)
{
    struct DriverCall *dc;
    struct data_packet *dp;

    dc = &((DevSWState *)(device->SwitcherState))->ds_activewrite;
    dp = &dc->dc_packet;

    /*
     * try to flush any packet that is still being written
     */
    if (dp->data != NULL)
    {
        flush_packet(device, dc);

        /* see if it has gone */
        if (dp->data != NULL)
           return adp_write_busy;
        else
           return adp_ok;
    }
    else
       return adp_ok;
}


AdpErrs DevSW_Write(const DeviceDescr *device, Packet *packet, DevChanID type)
{
    struct DriverCall *dc;
    struct data_packet *dp;

    dc = &((DevSWState *)(device->SwitcherState))->ds_activewrite;
    dp = &dc->dc_packet;

    if (illegalDevChanID(type))
        return adp_illegal_args;

    /*
     * try to flush any packet that is still being written
     */
    if (DevSW_FlushPendingWrite(device) != adp_ok)
       return adp_write_busy;

    /*
     * we can take this packet - set things up, then try to get rid of it
     */
    initialise_write(dc, packet, type);
    flush_packet(device, dc);

    return adp_ok;
}

AdpErrs DevSW_Ioctl(const DeviceDescr *device, const int opcode, void *args)
{
    return (device->DeviceIoctl(opcode, args) < 0) ? adp_failed : adp_ok;
}

bool DevSW_WriteFinished(const DeviceDescr *device)
{
    struct DriverCall *dc;
    struct data_packet *dp;

    dc = &((DevSWState *)(device->SwitcherState))->ds_activewrite;
    dp = &dc->dc_packet;

    return (dp == NULL || dp->data == NULL);
}

/* EOF devsw.c */
