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

#ifndef OSD_H
#define OSD_H

#include "config.h"

#include <sys/types.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "iscsiutil.h"

#define OSD_VENDOR	"NetBSD"
#define OSD_PRODUCT	"NetBSD/Intel OSD"
#define OSD_VERSION	0

/*
 * OSD Configuration
 */

#define CONFIG_OSD_CAPACITY_DFLT 1024
#define CONFIG_OSD_LUNS_DFLT     1
#define CONFIG_OSD_BASEDIR_DFLT  "/tmp/iscsi_osd"
#define CONFIG_OSD_CDB_LEN       128

/*
 * OSD Service Actions
 */

#define OSD_CREATE_GROUP 0x880B
#define OSD_REMOVE_GROUP 0x880C
#define OSD_CREATE       0x8802
#define OSD_REMOVE       0x880A
#define OSD_READ         0x8805
#define OSD_WRITE        0x8806
#define OSD_GET_ATTR     0x880E
#define OSD_SET_ATTR     0x880F

/*
 * OSD Arguments
 */

typedef struct osd_args_t {
  uint8_t   opcode;
  uint8_t   control;
  uint8_t   security;
  uint8_t   add_cdb_len;
  uint16_t  service_action;
  uint8_t   options_byte;
  uint8_t   second_options_byte;
  uint32_t  GroupID;
  uint64_t  UserID;
  uint32_t  SessionID;
  uint64_t  length;
  uint64_t  offset;
  uint32_t  set_attributes_list_length;
  uint32_t  get_attributes_page;
  uint32_t  get_attributes_list_length;
  uint32_t  get_attributes_allocation_length;
} osd_args_t;

#define OSD_ENCAP_CDB(ARGS, CDB)                                             \
(CDB)[0] = (ARGS)->opcode;                                                   \
(CDB)[1] = (ARGS)->control;                                                  \
(CDB)[6] = (ARGS)->security;                                                 \
(CDB)[7] = (ARGS)->add_cdb_len;                                              \
*((uint16_t *)((CDB)+8)) = ISCSI_HTONS((ARGS)->service_action);                    \
(CDB)[10] = (ARGS)->options_byte;                                            \
(CDB)[11] = (ARGS)->second_options_byte;                                     \
*((uint32_t *)((CDB)+12)) = ISCSI_HTONL((ARGS)->GroupID);                          \
*((uint64_t *)((CDB)+16)) = ISCSI_HTONLL((ARGS)->UserID);                          \
*((uint32_t *)((CDB)+24)) = ISCSI_HTONL((ARGS)->SessionID);                        \
*((uint64_t *)((CDB)+28)) = ISCSI_HTONLL((ARGS)->length);                          \
*((uint64_t *)((CDB)+36)) = ISCSI_HTONLL((ARGS)->offset);                          \
*((uint32_t *)((CDB)+44)) = ISCSI_HTONL((ARGS)->get_attributes_page);              \
*((uint32_t *)((CDB)+48)) = ISCSI_HTONL((ARGS)->get_attributes_list_length);       \
*((uint32_t *)((CDB)+52)) = ISCSI_HTONL((ARGS)->get_attributes_allocation_length); \
*((uint32_t *)((CDB)+72)) = ISCSI_HTONL((ARGS)->set_attributes_list_length);

#define OSD_DECAP_CDB(CDB, EXT_CDB, ARGS)                                           \
(ARGS)->opcode = (CDB)[0];                                                          \
(ARGS)->control = (CDB)[1];                                                         \
(ARGS)->security = (CDB)[6];                                                        \
(ARGS)->add_cdb_len = (CDB)[7];                                                     \
(ARGS)->service_action = ISCSI_NTOHS(*((uint16_t *)((CDB)+8)));                           \
(ARGS)->options_byte = (CDB)[10];                                                   \
(ARGS)->second_options_byte = (CDB)[11];                                            \
(ARGS)->GroupID = ISCSI_NTOHL(*((uint32_t *)((CDB)+12)));                                 \
(ARGS)->UserID = ISCSI_NTOHLL(*((uint64_t *)((EXT_CDB)-16+16)));                          \
(ARGS)->SessionID = ISCSI_NTOHL(*((uint32_t *)((EXT_CDB)-16+24)));                        \
(ARGS)->length = ISCSI_NTOHLL(*((uint64_t *)((EXT_CDB)-16+28)));                          \
(ARGS)->offset = ISCSI_NTOHLL(*((uint64_t *)((EXT_CDB)-16+36)));                          \
(ARGS)->get_attributes_page = ISCSI_NTOHL(*((uint32_t *)((EXT_CDB)-16+44)));              \
(ARGS)->get_attributes_list_length = ISCSI_NTOHL(*((uint32_t *)((EXT_CDB)-16+48)));       \
(ARGS)->get_attributes_allocation_length = ISCSI_NTOHL(*((uint32_t *)((EXT_CDB)-16+52))); \
(ARGS)->set_attributes_list_length = ISCSI_NTOHL(*((uint32_t *)((EXT_CDB)-16+72)));

#define OSD_PRINT_CDB(CDB, EXT_CDB)                                      \
PRINT("opcode         = 0x%x\n",   CDB[0]);                              \
PRINT("control        = 0x%x\n",   CDB[1]);                              \
PRINT("security       = 0x%x\n",   CDB[6]);                              \
PRINT("add_cdb_len    = %u\n",     CDB[7]);                              \
PRINT("service_action = 0x%x\n",   ISCSI_NTOHS(*(uint16_t*)(CDB+8)));          \
PRINT("options byte 1 = 0x%x\n",   CDB[10]);                             \
PRINT("options byte 2 = 0x%x\n",   CDB[11]);                             \
PRINT("group id       = 0x%x\n",   ISCSI_NTOHL(*(uint32_t*)(CDB+12)));         \
PRINT("user id        = 0x%llx\n", ISCSI_NTOHLL(*(uint64_t*)(EXT_CDB-16+16))); \
PRINT("session id     = 0x%x\n",   ISCSI_NTOHL(*(uint32_t*)(EXT_CDB-16+24)));  \
PRINT("length         = %llu\n",   ISCSI_NTOHLL(*(uint64_t*)(EXT_CDB-16+28))); \
PRINT("offset         = %llu\n",   ISCSI_NTOHLL(*(uint64_t*)(EXT_CDB-16+36))); \
PRINT("get attr page  = %u\n",     ISCSI_NTOHL(*(uint32_t*)(EXT_CDB-16+44)));  \
PRINT("get list len   = %u\n",     ISCSI_NTOHL(*(uint32_t*)(EXT_CDB-16+48)));  \
PRINT("get alloc len  = %u\n",     ISCSI_NTOHL(*(uint32_t*)(EXT_CDB-16+52)));  \
PRINT("set list len   = %u\n",     ISCSI_NTOHL(*(uint32_t*)(EXT_CDB-16+72)));

#endif /* OSD_H */
