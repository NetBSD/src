
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
 * Intel iSCSI Driver
 */

#ifndef _DRIVER_H_
#define _DRIVER_H_
#include "iscsi.h"
#include "iscsiutil.h"
#include "initiator.h"

/*
 * Driver configuration
 */

#define CONFIG_DRIVER_MAX_LUNS 1024

/*
 * Internal
 */

static int driver_init(void);
static int driver_shutdown(void);

typedef struct iscsi_driver_stats_t {
  unsigned num_tx, num_tx_queued; 
  unsigned num_rx, num_rx_queued;
  unsigned avg_tx, avg_rx;
  unsigned tx_queued, tx, tx_error, tx_overflow;
  unsigned rx_queued, rx, rx_error, rx_overflow;
  unsigned aborts_success, aborts_failed;
  unsigned device_resets, bus_resets, host_resets;
  iscsi_spin_t lock;
} iscsi_driver_stats_t;

/* 
 * Kernel Interface
 */

int iscsi_proc_info (char *, char **, off_t, int, int, int);
int iscsi_detect(Scsi_Host_Template *);
int iscsi_release(struct Scsi_Host *);
int iscsi_ioctl(Scsi_Device *dev, int cmd, void *arg);
int iscsi_command(Scsi_Cmnd *SCpnt);
int iscsi_queuecommand(Scsi_Cmnd *, void (*done)(Scsi_Cmnd *));
int iscsi_bios_param(Disk *, kdev_t, int *);
int iscsi_abort_handler (Scsi_Cmnd *SCpnt);
int iscsi_device_reset_handler (Scsi_Cmnd *);
int iscsi_bus_reset_handler (Scsi_Cmnd *);
int iscsi_host_reset_handler(Scsi_Cmnd *);

#if LINUX_VERSION_CODE >= LinuxVersionCode(2,3,0)
int iscsi_revoke (Scsi_Device *ptr);
void iscsi_select_queue_depths(struct Scsi_Host *, Scsi_Device *);
#endif

#if LINUX_VERSION_CODE < LinuxVersionCode(2,4,0)
extern struct proc_dir_entry iscsi_proc_dir;
#endif
 
#if LINUX_VERSION_CODE >= LinuxVersionCode(2,4,0)
#define ISCSI {		                                        \
        name: "Intel iSCSI Driver",                             \
        proc_dir: NULL,                                         \
        proc_info: iscsi_proc_info,                             \
        detect: iscsi_detect,                                   \
        bios_param: iscsi_bios_param,                           \
        release: iscsi_release,                                 \
        revoke: NULL,                                           \
        ioctl: iscsi_ioctl,                                     \
        command: iscsi_command,                                 \
        queuecommand: iscsi_queuecommand,                       \
        select_queue_depths: iscsi_select_queue_depths,         \
        eh_strategy_handler: NULL,                              \
        eh_abort_handler: iscsi_abort_handler,                  \
        eh_device_reset_handler: iscsi_device_reset_handler,    \
        eh_bus_reset_handler: iscsi_bus_reset_handler,          \
        eh_host_reset_handler: iscsi_host_reset_handler,        \
        slave_attach: NULL,                                     \
        can_queue: CONFIG_INITIATOR_QUEUE_DEPTH,       		\
        this_id: -1,             				\
        sg_tablesize: 128,                                	\
        cmd_per_lun: 1, /* linked commands not supported */     \
        present: 0,                                             \
        unchecked_isa_dma: 0,                                   \
        use_clustering: 0,                                      \
        use_new_eh_code: 1,                                     \
	emulated: 0,	 					\
        next: NULL,                                             \
        module: NULL,                                           \
        info: NULL,                                             \
	proc_name: "iscsi" 		 	 		\
}
#elif LINUX_VERSION_CODE >= LinuxVersionCode(2,2,0)
#define ISCSI {		                                        \
        name: "Intel iSCSI Driver",                             \
	proc_dir: &iscsi_proc_dir,                              \
        proc_info: iscsi_proc_info,                             \
        detect: iscsi_detect,                                   \
        bios_param: iscsi_bios_param,                           \
        release: iscsi_release,                                 \
        ioctl: iscsi_ioctl,                                     \
        command: iscsi_command,                                 \
        queuecommand: iscsi_queuecommand,                       \
        eh_strategy_handler: NULL,                              \
        eh_abort_handler: iscsi_abort_handler,                  \
        eh_device_reset_handler: iscsi_device_reset_handler,    \
        eh_bus_reset_handler: iscsi_bus_reset_handler,          \
        eh_host_reset_handler: iscsi_host_reset_handler,        \
        use_new_eh_code: 1,                                     \
        can_queue: CONFIG_INITIATOR_QUEUE_DEPTH,       		\
        sg_tablesize: SG_ALL,                                	\
        cmd_per_lun: 1, /* linked commands not supported */	\
        this_id: -1,             				\
        present: 0,                                             \
        unchecked_isa_dma: 0,                                   \
        use_clustering: 0,                                      \
        slave_attach: NULL,                                     \
        next: NULL,                                             \
        module: NULL,                                           \
        info: NULL,                                             \
	emulated: 0 	 					\
}
#endif /* LINUX_VERSION_CODE */
#endif /* _DRIVER_H_ */
