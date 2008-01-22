/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software. 
 *
 * Intel License Agreement 
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met: 
 *
 * -Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *  following disclaimer. 
 *
 * -Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *  following disclaimer in the documentation and/or other materials provided with the distribution. 
 *
 * -The name of Intel Corporation may not be used to endorse or promote products derived from this software
 *  without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE. 
 */

/* 
 * Intel SCSI device driver for iSCSI
 */

#ifdef __linux__
#include <linux/blk.h>
#include <linux/string.h>
#include <scsi.h>
#include <hosts.h>
#include <sd.h>
#endif

#include "util.h"
#include "util.c"
#include "driver.h"
#include "iscsi.h"
#include "iscsi.c"
#include "tests.h"
#include "tests.c"
#include "osd_ops.h"
#include "osd_ops.c"
#include "parameters.h"
#include "parameters.c"
#include "initiator.h"
#include "initiator.c"

/*
 * Version-specific include files
 */

#if LINUX_VERSION_CODE >= LinuxVersionCode(2,4,0)
#include <linux/in.h>
#else
struct proc_dir_entry iscsi_proc_dir = {PROC_SCSI_NOT_PRESENT, 5, "iscsi", S_IFDIR | S_IRUGO | S_IXUGO, 2};
#endif

#ifdef MODULE
Scsi_Host_Template driver_template = ISCSI;
#include "scsi_module.c"
#endif

/*
 * Globals
 */

static initiator_cmd_t *g_cmd;
static iscsi_queue_t g_cmd_q;
static struct iovec **g_iov;
static iscsi_queue_t g_iovec_q;
static iscsi_driver_stats_t g_stats;

/*
 * Definitions
 */

/* 
 * Starting with kernel 2.4.10, we define a license string. This source is under BSD License. 
 * Consult <linux/include/linux/module.h> for details
 */
 
MODULE_AUTHOR("Intel Corporation, <http://www.intel.com>");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,10) 
MODULE_LICENSE("Dual BSD/GPL"); /*  This source is under BSD License. This is the closest ident that module.h provides  */
#  endif 

/* 
 * Private
 */

static int driver_init(void) {
  int i;

  iscsi_trace(TRACE_SCSI_DEBUG, __FILE__, __LINE__, "initializing iSCSI driver\n");
  if ((g_cmd=iscsi_malloc_atomic(sizeof(initiator_cmd_t)*CONFIG_INITIATOR_QUEUE_DEPTH))==NULL) {
    iscsi_trace_error("iscsi_malloc_atomic() failed\n");
    return -1;
  }
  if ((g_iov=iscsi_malloc_atomic(sizeof(struct iovec*)*CONFIG_INITIATOR_QUEUE_DEPTH))==NULL) {
    iscsi_trace_error("iscsi_malloc_atomic() failed\n");
    iscsi_free_atomic(g_cmd);
    return -1;
  }
  for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
    g_iov[i] = NULL;
    g_cmd[i].ptr = NULL;
  }
#define DI_ERROR {                                                       \
      for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {                   \
        if (g_cmd[i].ptr != NULL) iscsi_free_atomic(g_cmd[i].ptr);       \
      }                                                                  \
      for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {                   \
        if (g_iov[i] != NULL) iscsi_free_atomic(g_iov[i]);               \
      }                                                                  \
      iscsi_free_atomic(g_iov);                                          \
      iscsi_free_atomic(g_cmd);                                          \
      return -1;                                                         \
}
  for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
    if ((g_iov[i]=iscsi_malloc_atomic(sizeof(struct iovec)*SG_ALL))==NULL) {
      iscsi_trace_error("iscsi_malloc_atomic() failed\n");
      DI_ERROR;
    }
  }
  if (iscsi_queue_init(&g_cmd_q, CONFIG_INITIATOR_QUEUE_DEPTH)!=0) {
    iscsi_trace_error("iscsi_queue_init() failed\n");
    DI_ERROR;
  }
  if (iscsi_queue_init(&g_iovec_q, CONFIG_INITIATOR_QUEUE_DEPTH)!=0) {
    iscsi_trace_error("iscsi_queue_init() failed\n");
    DI_ERROR;
  }

  for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
    if ((g_cmd[i].ptr = iscsi_malloc_atomic(sizeof(iscsi_scsi_cmd_args_t)))==NULL) {
      iscsi_trace_error("iscsi_malloc_atomic() failed\n");
      DI_ERROR;
    }
    g_cmd[i].type = ISCSI_SCSI_CMD;
    if (iscsi_queue_insert(&g_cmd_q, &g_cmd[i])!=0) {
      iscsi_trace_error("iscsi_queue_insert() failed\n");
      DI_ERROR;
    }
    if (iscsi_queue_insert(&g_iovec_q, g_iov[i])!=0) {
      iscsi_trace_error("iscsi_queue_insert() failed\n");
      DI_ERROR;
    }
  }
  memset(&g_stats, 0, sizeof(g_stats));
  iscsi_spin_init(&g_stats.lock);
  if (initiator_init()!=0) {
    iscsi_trace_error("initiator_init() failed\n");
    DI_ERROR;
  }
  iscsi_trace(TRACE_SCSI_DEBUG, "iSCSI initialization complete\n");
  return 0;
}

static int driver_shutdown(void) {
  int i;

  iscsi_trace(TRACE_SCSI_DEBUG, "shutting down iSCSI driver\n");
  if (initiator_shutdown()!=0) {
    iscsi_trace_error("initiator_shutdown() failed\n");
    return -1;
  }
  iscsi_spin_destroy(&g_stats.lock);
  iscsi_queue_destroy(&g_iovec_q);
  iscsi_queue_destroy(&g_cmd_q);
  for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
    iscsi_free_atomic(g_iov[i]);
    iscsi_free_atomic(g_cmd[i].ptr);
  }
  iscsi_free_atomic(g_cmd);
  iscsi_free_atomic(g_iov);
  iscsi_trace(TRACE_SCSI_DEBUG, "iSCSI driver shutdown complete\n");

  return 0;
}

/*
 * Public
 */

int iscsi_detect(Scsi_Host_Template *tptr) {
  struct Scsi_Host *ptr;
   
  iscsi_trace(TRACE_SCSI_DEBUG, "detecting iSCSI host\n");
  spin_unlock(&io_request_lock);
  if (driver_init()!=0) {
    iscsi_trace_error("driver_init() failed\n");
    spin_lock(&io_request_lock);
    return 0; /*No 'SCSI' host detected, return 0 */
   	
  }
  ptr = scsi_register(tptr, 0);
  ptr->max_id = CONFIG_INITIATOR_NUM_TARGETS;
  ptr->max_lun = CONFIG_DRIVER_MAX_LUNS;
  ptr->max_cmd_len = 255;
  iscsi_trace(TRACE_SCSI_DEBUG, "iSCSI host detected\n");
   spin_lock(&io_request_lock);
  return 1; 
}

int iscsi_release(struct Scsi_Host *host) {
  iscsi_trace(TRACE_SCSI_DEBUG, "releasing iSCSI host\n");
  driver_shutdown();
  iscsi_trace(TRACE_SCSI_DEBUG, "iSCSI host released\n");
  return 0;
}

int iscsi_bios_param(Disk *disk, kdev_t dev, int *ip) {
  ip[0] = 32;                                  /*  heads */
  ip[1] = 63;                                  /*  sectors */
  if((ip[2] = disk->capacity >> 11) > 1024) {  /*  cylinders, test for big disk */
    ip[0] = 255;                               /*  heads */
    ip[1] = 63;                                /*  sectors */
    ip[2] = disk->capacity / (255 * 63);       /*  cylinders */
  }
  iscsi_trace(TRACE_SCSI_DEBUG, "%u sectors, H/S/C: %u/%u/%u\n", disk->capacity, ip[0], ip[1], ip[2]);
  return 0;
}

int iscsi_command(Scsi_Cmnd *SCpnt) {
  iscsi_trace(TRACE_SCSI_DEBUG, "0x%p: op 0x%x, chan %i, target %i, lun %i, bufflen %i, sg %i\n", 
        SCpnt, SCpnt->cmnd[0], SCpnt->channel, SCpnt->target, SCpnt->lun, 
        SCpnt->request_bufflen, SCpnt->use_sg);
  iscsi_trace_error("NOT IMPLEMENTED\n");
  return -1;
}

int iscsi_done(void *ptr) {
  initiator_cmd_t *cmd = (initiator_cmd_t *) ptr;
  iscsi_scsi_cmd_args_t *scsi_cmd = (iscsi_scsi_cmd_args_t *) cmd->ptr;
  Scsi_Cmnd *SCpnt = (Scsi_Cmnd *) cmd->callback_arg;
  unsigned long flags = 0;
  if (SCpnt==0) {
	  return 0;
  } 
  if (cmd->status==0) {
    SCpnt->result = scsi_cmd->status;
  } else {
    SCpnt->result = -1;
  } 
  iscsi_trace(TRACE_SCSI_DEBUG, "scsi_arg 0x%p SCpnt 0x%p op 0x%x done (result %i)\n",
        scsi_cmd, SCpnt, SCpnt->cmnd[0], SCpnt->result);
  if ((scsi_cmd->input)&&(scsi_cmd->output)) {
    iscsi_trace_error("bidi xfers not implemented\n");
    return -1;
  } else if (scsi_cmd->input) {
    iscsi_spin_lock_irqsave(&g_stats.lock, &flags);
    if ((g_stats.rx+SCpnt->request_bufflen)<g_stats.rx) {
      g_stats.rx_overflow++;
    }
    g_stats.rx += SCpnt->request_bufflen;
    g_stats.rx_queued -= SCpnt->request_bufflen;
    if (g_stats.num_rx_queued) g_stats.num_rx_queued--;
    g_stats.num_rx++;
    iscsi_spin_unlock_irqrestore(&g_stats.lock, &flags);
  } else if (scsi_cmd->output) {
    iscsi_spin_lock_irqsave(&g_stats.lock, &flags);
    if ((g_stats.tx+SCpnt->request_bufflen)<g_stats.tx) {
      g_stats.tx_overflow++;
    }
    g_stats.tx += SCpnt->request_bufflen;
    g_stats.tx_queued -= SCpnt->request_bufflen;
    if (g_stats.num_tx_queued) g_stats.num_tx_queued--;
    g_stats.num_tx++;
    iscsi_spin_unlock_irqrestore(&g_stats.lock, &flags);
  }
  if (SCpnt->host_scribble) {
    unsigned char *hs;
    hs = SCpnt->host_scribble;
    SCpnt->host_scribble = NULL; /*  for abort */
    if (iscsi_queue_insert(&g_iovec_q, hs)!=0) {
      iscsi_trace_error("iscsi_queue_insert() failed\n");
      return -1;
    }  
  }
  iscsi_free_atomic(scsi_cmd->ahs);
  if (iscsi_queue_insert(&g_cmd_q, cmd)!=0) {
    iscsi_trace_error("iscsi_queue_insert() failed\n");
  cmd->callback_arg = NULL;    /*  for abort */
    return -1;
  } 
  cmd->callback_arg = NULL;    /*  for abort */
  if (SCpnt->result==0) {
    SCpnt->scsi_done(SCpnt);
  } else {
    iscsi_trace_error("SCSI cmd 0x%x failed at iSCSI level (ignoring)\n", SCpnt->cmnd[0]);
  }
  return 0;
}

int iscsi_queuecommand(Scsi_Cmnd *SCpnt, void (*done)(Scsi_Cmnd *)) {
  initiator_cmd_t *cmd;
  iscsi_scsi_cmd_args_t *scsi_cmd;
  unsigned char *data;
  unsigned length, trans_len;
  int input, output;
  unsigned long flags =0;
  /*  Tagged command queuing is handled from within this SCSI driver. */
  if ((SCpnt->device->tagged_supported)&&(SCpnt->device->tagged_queue)) {
    SCpnt->tag = SCpnt->device->current_tag++;
  }

  spin_unlock(&io_request_lock);

  iscsi_trace(TRACE_SCSI_DEBUG, "SCpnt %p: tid %i lun %i op 0x%x tag %u len %i sg %i buff 0x%p\n",
        SCpnt, SCpnt->target, SCpnt->lun, SCpnt->cmnd[0], SCpnt->tag, SCpnt->request_bufflen, 
        SCpnt->use_sg, SCpnt->buffer);


  /*  Determine direction of data transfer */

  trans_len = length = output = input = 0;
  if ((SCpnt->cmnd[0]!=TEST_UNIT_READY)&&(SCpnt->request_bufflen)) {
    if (SCpnt->sc_data_direction == SCSI_DATA_WRITE) {
      output = 1; input = 0;
      length = trans_len = SCpnt->request_bufflen;
      iscsi_spin_lock_irqsave(&g_stats.lock, &flags);
      g_stats.num_tx_queued++;
      g_stats.tx_queued += trans_len;
      iscsi_spin_unlock_irqrestore(&g_stats.lock, &flags);
    } else if (SCpnt->sc_data_direction == SCSI_DATA_READ) {
      length = output = 0; input = 1;
      trans_len = SCpnt->request_bufflen;
      iscsi_spin_lock_irqsave(&g_stats.lock, &flags);
      g_stats.num_rx_queued++;
      g_stats.rx_queued += trans_len;
      iscsi_spin_unlock_irqrestore(&g_stats.lock, &flags);
    }
  } 

  /*  Convert scatterlist to iovec */

  if (SCpnt->use_sg) {
    struct scatterlist *sg = (struct scatterlist *) SCpnt->buffer;
    struct iovec *iov;
    int i;

    iov = iscsi_queue_remove(&g_iovec_q);
    if (iov == NULL) {
      iscsi_trace_error("iscsi_queue_remove() failed\n");
      spin_lock(&io_request_lock);
      return -1;
    }
    for (i=0; i<SCpnt->use_sg; i++) {
      iov[i].iov_base = sg[i].address; 
      iov[i].iov_len = sg[i].length;
    }
    data = SCpnt->host_scribble = (unsigned char *)iov;
  } else {
    data = SCpnt->buffer;
    SCpnt->host_scribble = NULL;
  } 

  /*  Get free cmd structure */

  if ((cmd=iscsi_queue_remove(&g_cmd_q))==NULL) {
    iscsi_trace_error("iscsi_queue_remove() failed\n");
    spin_lock(&io_request_lock);	
    return -1;
  } 
  scsi_cmd = (iscsi_scsi_cmd_args_t *) cmd->ptr;
  memset(scsi_cmd, 0, sizeof(*scsi_cmd));
  scsi_cmd->send_data = output?data:0;
  scsi_cmd->send_sg_len = output?SCpnt->use_sg:0;
  scsi_cmd->recv_data = input?data:NULL;
  scsi_cmd->recv_sg_len = input?SCpnt->use_sg:0;
  scsi_cmd->input = input;
  scsi_cmd->output = output;
  scsi_cmd->length = length;
  scsi_cmd->lun = SCpnt->lun;
  scsi_cmd->lun = scsi_cmd->lun << 32;
  scsi_cmd->trans_len = trans_len;
  scsi_cmd->cdb = SCpnt->cmnd;

  /*  AHS for CDBs larget than 16 bytes */

  if (SCpnt->cmd_len>16) {
    iscsi_trace(TRACE_ISCSI_DEBUG, "creating AHS for extended CDB (%i bytes)\n", SCpnt->cmd_len);
    if ((scsi_cmd->ahs=iscsi_malloc_atomic(SCpnt->cmd_len-16))==NULL) {
      iscsi_trace_error("iscsi_malloc_atomic() failed\n");
      spin_lock(&io_request_lock);
      return -1;
    }
    memset(scsi_cmd->ahs, 0, 4);
    *((uint16_t *)scsi_cmd->ahs) = HTONS(SCpnt->cmd_len-15);      /*  AHS length */
    scsi_cmd->ahs[2] = 0x01;                                      /*  Type */
    memcpy(scsi_cmd->ahs+4, SCpnt->cmnd+16, SCpnt->cmd_len-16);   /*  Copy in remaining CDB */
    scsi_cmd->ahs_len = SCpnt->cmd_len-16;
  }

  SCpnt->scsi_done = done;     /*  The midlayer's callback called in iscsi_done */
  SCpnt->result = 0x02;        /*  Default to a check condition */
  cmd->callback = iscsi_done;  /*  This driver's callback called by initiator library */
  cmd->callback_arg = SCpnt;
  cmd->isid = SCpnt->target;
  if (initiator_enqueue(cmd)!=0) {
    iscsi_trace_error("initiator_enqueue() failed\n");
    if (SCpnt->cmd_len>16) iscsi_free_atomic(scsi_cmd->ahs);
    spin_lock(&io_request_lock);
    return -1;
  }
    spin_lock(&io_request_lock);
  return 0;
}

int iscsi_proc_info (char *buffer, char **start, off_t offset, int length, int hostno, int writing) {
  unsigned char *info = NULL;
  uint32_t	infolen = 8192;
  int len = 0;

  iscsi_trace(TRACE_SCSI_DEBUG, "buffer = 0x%p, offset %u, length = %i, hostno %i, writing %i\n",
        buffer, (unsigned) offset, length, hostno, writing);

  /* writing resets counters */

  if (writing) {
    iscsi_spin_lock(&g_stats.lock);
    g_stats.num_tx = g_stats.num_tx_queued = 0;
    g_stats.num_rx = g_stats.num_rx_queued = 0;
    g_stats.tx_queued = g_stats.tx = g_stats.tx_overflow = g_stats.tx_error = 0;
    g_stats.rx_queued = g_stats.rx = g_stats.rx_overflow = g_stats.rx_error = 0;
    g_stats.aborts_success = g_stats.aborts_failed = 0;
    g_stats.device_resets = g_stats.bus_resets = g_stats.host_resets = 0;
    iscsi_spin_unlock(&g_stats.lock);
    return 0;
  } else {
    if ((info=iscsi_malloc_atomic(infolen))==NULL) {
      iscsi_trace_error("iscsi_malloc_atomic() failed\n");
      return -1;
    }
    len += snprintf(info, infolen, "%s\n\n", driver_template.name);
    len += snprintf(&info[len], infolen - len, "Write file to reset counters (e.g., \"echo reset > /proc/scsi/iscsi/2\").\n\n");
    len += snprintf(&info[len], infolen - len, "--------------------\n");
    len += snprintf(&info[len], infolen - len, "Driver Configuration\n");

    len += snprintf(&info[len], infolen - len, "--------------------\n\n");
    len += snprintf(&info[len], infolen - len, "  CONFIG_INITIATOR_NUM_TARGETS: %u\n", CONFIG_INITIATOR_NUM_TARGETS);
    len += snprintf(&info[len], infolen - len, "  CONFIG_INITIATOR_QUEUE_DEPTH: %u\n\n", CONFIG_INITIATOR_QUEUE_DEPTH);
    len += snprintf(&info[len], infolen - len, "---------------\n");
    len += snprintf(&info[len], infolen - len, "SCSI Statistics\n");
    len += snprintf(&info[len], infolen - len, "---------------\n\n");
    len += snprintf(&info[len], infolen - len, "  Tx:\n");
    len += snprintf(&info[len], infolen - len, "    queued:          %u\n", g_stats.num_tx_queued);
    len += snprintf(&info[len], infolen - len, "    completed:       %u\n", g_stats.num_tx);
    len += snprintf(&info[len], infolen - len, "    avg size:        %u\n", g_stats.num_tx?(g_stats.tx/g_stats.num_tx):0);
    len += snprintf(&info[len], infolen - len, "    total bytes:     %u MB\n", g_stats.tx/1048576 + g_stats.tx_overflow*4096);
    len += snprintf(&info[len], infolen - len, "    total overflow:  %u\n", g_stats.tx_overflow);
    len += snprintf(&info[len], infolen - len, "  Rx:\n");
    len += snprintf(&info[len], infolen - len, "    queued:          %u\n", g_stats.num_rx_queued);
    len += snprintf(&info[len], infolen - len, "    completed:       %u\n", g_stats.num_rx);
    len += snprintf(&info[len], infolen - len, "    avg size:        %u\n", g_stats.num_rx?(g_stats.rx/g_stats.num_rx):0);
    len += snprintf(&info[len], infolen - len, "    total bytes:     %u MB\n", g_stats.rx/1048576 + g_stats.rx_overflow*4096);
    len += snprintf(&info[len], infolen - len, "    total overflow:  %u\n", g_stats.rx_overflow);
    len += snprintf(&info[len], infolen - len, "  Errors:\n");
    len += snprintf(&info[len], infolen - len, "    aborts:          %u\n", g_stats.aborts_success);
    /* len += snprintf(&info[len], infolen - len, "    failed aborts:   %u\n", g_stats.aborts_failed); */
    len += snprintf(&info[len], infolen - len, "    device resets:   %u\n", g_stats.device_resets);
    len += snprintf(&info[len], infolen - len, "    bus resets:      %u\n", g_stats.bus_resets);
    len += snprintf(&info[len], infolen - len, "    host resets:     %u\n", g_stats.host_resets);
    len += snprintf(&info[len], infolen - len, "    Tx error bytes:  %u\n", g_stats.tx_error);
    len += snprintf(&info[len], infolen - len, "    Rx error bytes:  %u\n\n", g_stats.rx_error);
    len += snprintf(&info[len], infolen - len, "--------------------\n");
    len += snprintf(&info[len], infolen - len, "iSCSI Initiator Info\n");
    len += snprintf(&info[len], infolen - len, "--------------------\n\n");

    if ((len += initiator_info(&info[len], infolen, len))==-1) {
      iscsi_trace_error("initiator_info() failed\n");
      if (info != NULL) iscsi_free_atomic(info);
      return -1;
    }
    if (offset>len-1) {
      len = 0;
    } else if (offset+length>len-1) {
      len -= offset;
    } else {
      len = length;
    }
    *start = buffer;
    memcpy(buffer, info+offset, len);
    if (info != NULL) iscsi_free_atomic(info);
    return len;
  }
}

int iscsi_ioctl (Scsi_Device *dev, int cmd, void *argp) {
  int i;
  int lun = 0;

  /*  Run tests for each target */

  for (i=0; i<CONFIG_INITIATOR_NUM_TARGETS; i++) {
    if (test_all(i, lun)!=0) {
      iscsi_trace_error("test_all() failed\n");
      return -1;
    }
  }
  return 0;
}

void iscsi_select_queue_depths(struct Scsi_Host *host, Scsi_Device *scsi_devs) {
  struct scsi_device *device;

  for (device = scsi_devs; device; device = device->next) {
    if (device->host != host) {
      iscsi_trace_error("got device for different host\n");
      continue;
    }
    if (device->tagged_supported) {
      iscsi_trace(TRACE_SCSI_DEBUG, "target %i lun %i supports TCQ\n", device->id, device->lun);
      device->tagged_queue = 1;
      device->current_tag = 0;
      device->queue_depth = CONFIG_INITIATOR_QUEUE_DEPTH;
      iscsi_trace(TRACE_SCSI_DEBUG, "device queue depth set to %i\n", device->queue_depth);
    } else {
      iscsi_trace(TRACE_SCSI_DEBUG, "target %i lun %i does NOT support TCQ\n", device->id, device->lun);
      device->queue_depth = 1;
    }
  }
}

/*
 * Error Handling Routines
 */


int iscsi_abort_handler (Scsi_Cmnd *SCpnt) {
  iscsi_scsi_cmd_args_t *scsi_cmd;
  initiator_session_t *sess;
  int i;
  unsigned long flags;

  spin_unlock_irq(&io_request_lock);
  iscsi_trace_error("aborting SCSI cmd 0x%p (op 0x%x, tid %i, lun %i)\n", 
              SCpnt, SCpnt->cmnd[0], SCpnt->target, SCpnt->lun);


  for (i=0; i<CONFIG_INITIATOR_QUEUE_DEPTH; i++) {
   
    /*  Find the cmd ptr in g_cmd.  We look for the callback_arg that's equal */
    /*  to SCpnt. iscsi_done() sets callback_arg to NULL when a command */
    /*  completes.  So we know that any non_NULL callback_arg is associated */
    /*  with an outstanding command. */

    if (g_cmd[i].callback_arg == SCpnt) {

      /*  Abort the command */

      if (initiator_abort(&g_cmd[i])!=0) {
        iscsi_trace_error("initiator_abort() failed\n");
        spin_lock_irq(&io_request_lock);
        return FAILED;
      }

      /*  Update counters */

      scsi_cmd = (iscsi_scsi_cmd_args_t *) g_cmd[i].ptr;
      if ((scsi_cmd->input)&&(scsi_cmd->output)) {
        iscsi_trace_error("bidi xfers not implemented\n");
        spin_lock_irq(&io_request_lock);
        return FAILED;
      } else if (scsi_cmd->input) {
        iscsi_spin_lock_irqsave(&g_stats.lock,&flags);
        g_stats.rx_error += SCpnt->request_bufflen; 
        g_stats.rx_queued -= SCpnt->request_bufflen;
	iscsi_spin_unlock_irqrestore(&g_stats.lock, &flags);
      } else if (scsi_cmd->output) {
	iscsi_spin_lock_irqsave(&g_stats.lock, &flags);
        g_stats.tx_error += SCpnt->request_bufflen;
        g_stats.tx_queued -= SCpnt->request_bufflen;
	iscsi_spin_unlock_irqrestore(&g_stats.lock, &flags);
      }
      break;
    } 
  }

  /*  Destroy session */

  if (g_target[SCpnt->target].has_session) {
    sess = g_target[SCpnt->target].sess;

#if LINUX_VERSION_CODE >= LinuxVersionCode(2,4,0)
    if (in_interrupt()) {
      iscsi_trace_error("aborting within interrupt (killing Tx and Rx threads)\n");
#endif
      iscsi_trace_error("killing Tx and Rx threads\n");
      kill_proc(sess->rx_worker.pid, SIGKILL, 1);
      kill_proc(sess->tx_worker.pid, SIGKILL, 1);
      sess->tx_worker.state = 0;
      sess->rx_worker.state = 0;
#if LINUX_VERSION_CODE >= LinuxVersionCode(2,4,0)
    } else {
      iscsi_trace_error("aborting outside interrupt (gracefully ending Tx and Rx)\n");
    }
#endif

    iscsi_trace(TRACE_ISCSI_DEBUG, "destroying session\n");
    if (session_destroy_i(sess)!=0) {
      iscsi_trace_error("session_destroy() failed\n");
      g_stats.aborts_failed++;
      spin_lock_irq(&io_request_lock);
      return FAILED;
    }
  } else {
    iscsi_trace(TRACE_ISCSI_DEBUG, "no session\n");
  }

  g_stats.aborts_success++;

  iscsi_trace_error("successfully aborted SCSI cmd 0x%p (op 0x%x, tid %i, lun %i)\n",
              SCpnt, SCpnt->cmnd[0], SCpnt->target, SCpnt->lun);
  spin_lock_irq(&io_request_lock);
  return SUCCESS;
}

int iscsi_device_reset_handler (Scsi_Cmnd *SCpnt) {
  iscsi_trace_error("***********************\n");
  iscsi_trace_error("*** DEVICE %i RESET ***\n", SCpnt->target);
  iscsi_trace_error("***********************\n");
  g_stats.device_resets++;
  return SUCCESS;
}

int iscsi_bus_reset_handler (Scsi_Cmnd *SCpnt) {
  iscsi_trace_error("********************\n");
  iscsi_trace_error("*** BUS %i RESET ***\n", SCpnt->target);
  iscsi_trace_error("********************\n");
  g_stats.bus_resets++;
  return SUCCESS;
}

int iscsi_host_reset_handler(Scsi_Cmnd *SCpnt) {
  iscsi_trace_error("*********************\n");
  iscsi_trace_error("*** HOST RESET %i ***\n", SCpnt->target);
  iscsi_trace_error("*********************\n");
  g_stats.host_resets++;
  return SUCCESS;
}
