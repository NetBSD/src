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
#include "config.h"

#include <sys/types.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <unistd.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#include "scsi_cmd_codes.h"

#include "iscsi.h"
#include "util.h"
#include "device.h"
#include "osd.h"

/*
 * Globals
 */

static int      osd_luns = CONFIG_OSD_LUNS_DFLT;
static uint64_t osd_capacity = CONFIG_OSD_CAPACITY_DFLT * 1048576;
static char     base_dir[64] = CONFIG_OSD_BASEDIR_DFLT;

#ifndef __KERNEL__
void
device_set_var(const char *var, char *arg)
{
	if (strcmp(var, "capacity") == 0) {
		osd_capacity = strtoll(arg, (char **) NULL, 10) * 1048576;
	} else if (strcmp(var, "luns") == 0) {
		osd_luns = atoi(arg);
	} else if (strcmp(var, "directory") == 0) {
		(void) strlcpy(base_dir, arg, sizeof(base_dir));
	} else {
		(void) fprintf(stderr, "Unrecognised variable: `%s'\n", var);
	}
}
#endif

int 
device_init(globals_t *gp, char *dev)
{
	struct stat     st;
	char            FileName[1024];
	int             i;

	if (stat(base_dir, &st) < 0) {

		/* Create directory for OSD */

		if (mkdir(base_dir, 0755) != 0) {
			if (errno != EEXIST) {
				iscsi_trace_error(__FILE__, __LINE__, "error creating directory \"%s\" for OSD: errno %d\n", base_dir, errno);
				return -1;
			}
		}
		/* Create directory for LU */

		for (i = 0; i < osd_luns; i++) {
			sprintf(FileName, "%s/lun_%d", base_dir, i);
			if (mkdir(FileName, 0755) != 0) {
				if (errno != EEXIST) {
					iscsi_trace_error(__FILE__, __LINE__, "error creating \"%s\" for LU %d: errno %d\n", FileName, i, errno);
					return -1;
				}
			}
		}
	}
	/* Display LU info */

	return 0;
}

int 
osd_read_callback(void *arg)
{
	struct iovec   *sg = (struct iovec *) arg;
	int             i = 0;

	while (sg[i].iov_base != NULL) {
		iscsi_free_atomic(sg[i].iov_base);
		i++;
	}
	return 0;
}

int 
device_command(target_session_t * sess, target_cmd_t * cmd)
{
	iscsi_scsi_cmd_args_t *args = cmd->scsi_cmd;
	uint8_t  *data;
	char            FileName[1024];
	uint8_t        *write_data = NULL;
	uint8_t        *read_data = NULL;
	uint8_t        *set_list = NULL;
	uint8_t        *get_list = NULL;
	struct iovec    sg[3];
	int             sg_len = 0;
	int             rc;
	osd_args_t      osd_args;
	uint32_t        GroupID = 0;
	uint64_t        UserID = 0;
	char            string[1024];
	uint8_t  *get_data = NULL;
	uint32_t        page = 0;
	uint32_t        index = 0;
	int             attr_len = 0;

	iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "SCSI op 0x%x (lun %llu)\n", args->cdb[0], args->lun);

	if (args->lun >= osd_luns) {
		iscsi_trace(TRACE_SCSI_DEBUG, __FILE__, __LINE__, "invalid lun: %llu\n", args->lun);
		args->status = 0x01;
		return 0;
	}
	args->status = 1;

	switch (args->cdb[0]) {

	case TEST_UNIT_READY:

		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "TEST_UNIT_READY(lun %llu)\n", args->lun);
		args->status = 0;
		args->length = 0;
		break;

	case INQUIRY:

		iscsi_trace(TRACE_SCSI_CMD, __FILE__, __LINE__, "INQUIRY(lun %llu)\n", args->lun);
		data = args->send_data;
		memset(data, 0, args->cdb[4]);	/* Clear allocated buffer */
		data[0] = 0x0e;	/* Peripheral Device Type */
		/* data[1] |= 0x80;                        // Removable Bit */
		data[2] |= 0x02;/* ANSI-approved version */
		/* data[3] |= 0x80;                        // AENC */
		/* data[3] |= 0x40;                        // TrmIOP */
		/* data[3] |= 0x20;                        // NormACA */
		data[4] = args->cdb[4] - 4;	/* Additional length */
		/*
		 * data[7] |= 0x80;                        // Relative
		 * addressing
		 */
		data[7] |= 0x40;/* WBus32 */
		data[7] |= 0x20;/* WBus16 */
		/* data[7] |= 0x10;                        // Sync */
		/* data[7] |= 0x08;                        // Linked Commands */
		/* data[7] |= 0x04;                        // TransDis */
		/*
		 * data[7] |= 0x02;                        // Tagged Command
		 * Queueing
		 */
		/* data[7] |= 0x01;                        // SftRe */
		(void) memset(data + 8, 0x0, 32);
		strlcpy(data + 8, OSD_VENDOR, 8);	/* Vendor */
		strlcpy(data + 16, OSD_PRODUCT, 16);	/* Product ID */
		(void) snprintf(data + 32, 8, "%d", OSD_VERSION);	/* Product Revision */
		args->input = 1;
		args->length = args->cdb[4] + 1;
		args->status = 0;

		break;

	case 0x7F:

		OSD_DECAP_CDB(args->cdb, args->ext_cdb, &osd_args);
		/* OSD_PRINT_CDB(args->cdb, args->ext_cdb); */
		GroupID = osd_args.GroupID;
		UserID = osd_args.UserID;

		/*
	         * Transfer all data
	         */

		if (osd_args.set_attributes_list_length) {
			if ((set_list = iscsi_malloc_atomic(osd_args.set_attributes_list_length)) == NULL) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
				goto done;
			}
			sg[sg_len].iov_base = set_list;
			sg[sg_len].iov_len = osd_args.set_attributes_list_length;
			sg_len++;
		}
		if (osd_args.get_attributes_list_length) {
			if ((get_list = iscsi_malloc_atomic(osd_args.get_attributes_list_length)) == NULL) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
				goto done;
			}
			sg[sg_len].iov_base = get_list;
			sg[sg_len].iov_len = osd_args.get_attributes_list_length;
			sg_len++;
		}
		if (osd_args.service_action == OSD_WRITE) {
			if ((write_data = iscsi_malloc_atomic(osd_args.length)) == NULL) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
				goto done;
			}
			sg[sg_len].iov_base = write_data;
			sg[sg_len].iov_len = osd_args.length;
			sg_len++;
		}
		if (sg_len) {
			if (target_transfer_data(sess, args, sg, sg_len) != 0) {
				iscsi_trace_error(__FILE__, __LINE__, "target_transfer_data() failed\n");
				goto done;
			}
		}
		/*
	         * Set any attributes
	         */

		if (osd_args.set_attributes_list_length) {
			uint32_t        page, attr;
			uint16_t        len;
			int             i;

			iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "OSD_SET_ATTR(lun %llu, GroupID 0x%x, UserID 0x%llx)\n", args->lun, osd_args.GroupID, osd_args.UserID);
			for (i = 0; i < osd_args.set_attributes_list_length;) {
				page = ISCSI_NTOHL(*((uint32_t *) (&(set_list[i]))));
				i += 4;
				attr = ISCSI_NTOHL(*((uint32_t *) (&(set_list[i]))));
				i += 4;
				len = ISCSI_NTOHS(*((uint16_t *) (&(set_list[i]))));
				i += 2;
				sprintf(FileName, "%s/lun_%llu/0x%x/0x%llx.0x%x.%u",
					base_dir, args->lun, osd_args.GroupID, osd_args.UserID, page, attr);
				if ((rc = open(FileName, O_WRONLY | O_CREAT, 0644)) == -1) {
					iscsi_trace_error(__FILE__, __LINE__, "error opening \"%s\": errno %d\n", FileName, errno);
					goto done;
				}
				if (write(rc, set_list + i, len) != len) {
					iscsi_trace_error(__FILE__, __LINE__, "write() failed\n");
				}
				close(rc);
				i += len;
				iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "SET(0x%x,%u,%u>\n", page, attr, len);
			}
		}
		args->send_sg_len = 0;
		sg_len = 0;

		switch (osd_args.service_action) {

		case OSD_CREATE_GROUP:

			do {
				GroupID = rand() % 1048576 * 1024 + 1;
				sprintf(FileName, "%s/lun_%llu/0x%x", base_dir, args->lun, GroupID);
				rc = mkdir(FileName, 0755);
			} while (rc == -1 && errno == EEXIST);
			iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "OSD_CREATE_GROUP(lun %llu) --> 0x%x\n", args->lun, GroupID);
			args->status = 0;
			break;

		case OSD_REMOVE_GROUP:

			iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "OSD_REMOVE_GROUP(lun %llu, 0x%x)\n", args->lun, osd_args.GroupID);
			sprintf(FileName, "%s/lun_%llu/0x%x", base_dir, args->lun, osd_args.GroupID);
			if ((rc = rmdir(FileName)) == -1) {
				iscsi_trace_error(__FILE__, __LINE__, "rmdir(\"%s\") failed: errno %d\n", FileName, errno);
				goto done;
			}
			args->status = 0;
			break;

		case OSD_CREATE:

			UserID = rand() % 1048576 * 1024 + 1;
create_user_again:
			sprintf(FileName, "%s/lun_%llu/0x%x/0x%llx",
			     base_dir, args->lun, osd_args.GroupID, UserID);
			rc = open(FileName, O_CREAT | O_EXCL | O_RDWR, 0644);
			if ((rc == -1) && (errno == EEXIST)) {
				UserID = rand() % 1048576 * 1024 + 1;
				goto create_user_again;
			}
			close(rc);
			iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "OSD_CREATE(lun %llu, GroupID 0x%x) --> 0x%llx\n", args->lun, osd_args.GroupID, UserID);
			args->status = 0;

			break;

		case OSD_REMOVE:

			iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "OSD_REMOVE(lun %llu, 0x%llx)\n", args->lun, osd_args.UserID);
			sprintf(FileName, "%s/lun_%llu/0x%x/0x%llx",
				base_dir, args->lun, osd_args.GroupID, osd_args.UserID);
			if ((rc = unlink(FileName)) == -1) {
				iscsi_trace_error(__FILE__, __LINE__, "unlink(\"%s\") failed: errno %d\n", FileName, errno);
				goto done;
			}
			sprintf(string, "rm -f %s/lun_%llu/0x%x/0x%llx.*", base_dir, args->lun, osd_args.GroupID, osd_args.UserID);
			if (system(string) != 0) {
				iscsi_trace_error(__FILE__, __LINE__, "\"%s\" failed\n", string);
				return -1;
			}
			args->status = 0;
			break;

		case OSD_WRITE:
			iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "OSD_WRITE(lun %llu, GroupID 0x%x, UserID 0x%llx, length %llu, offset %llu)\n",
			      args->lun, osd_args.GroupID, osd_args.UserID, osd_args.length, osd_args.offset);
			sprintf(FileName, "%s/lun_%llu/0x%x/0x%llx",
				base_dir, args->lun, osd_args.GroupID, osd_args.UserID);
			if ((rc = open(FileName, O_WRONLY, 0644)) == -1) {
				iscsi_trace_error(__FILE__, __LINE__, "error opening \"%s\": errno %d\n", FileName, errno);
				goto write_done;
			}
			if (lseek(rc, osd_args.offset, SEEK_SET) == -1) {
				iscsi_trace_error(__FILE__, __LINE__, "error seeking \"%s\": errno %d\n", FileName, errno);
				goto write_done;
			}
			if (write(rc, write_data, osd_args.length) != osd_args.length) {
				iscsi_trace_error(__FILE__, __LINE__, "write() failed\n");
				goto write_done;
			}
			close(rc);
			args->status = 0;
write_done:
			break;

		case OSD_READ:
			iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "OSD_READ(lun %llu, GroupID 0x%x, UserID 0x%llx, length %llu, offset %llu)\n",
			      args->lun, osd_args.GroupID, osd_args.UserID, osd_args.length, osd_args.offset);
			sprintf(FileName, "%s/lun_%llu/0x%x/0x%llx",
				base_dir, args->lun, osd_args.GroupID, osd_args.UserID);
			if ((rc = open(FileName, O_RDONLY, 0644)) == -1) {
				iscsi_trace_error(__FILE__, __LINE__, "error opening \"%s\": errno %d\n", FileName, errno);
				goto read_done;
			}
			if ((read_data = iscsi_malloc_atomic(osd_args.length)) == NULL) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
				goto read_done;
			}
			if (lseek(rc, osd_args.offset, SEEK_SET) == -1) {
				iscsi_trace_error(__FILE__, __LINE__, "error seeking \"%s\": errno %d\n", FileName, errno);
				goto read_done;
			}
			if (read(rc, read_data, osd_args.length) != osd_args.length) {
				iscsi_trace_error(__FILE__, __LINE__, "read() failed\n");
				goto read_done;
			}
			close(rc);
			args->status = 0;
read_done:
			if (args->status == 0) {
				args->input = 1;
				sg[0].iov_base = read_data;
				sg[0].iov_len = osd_args.length;
				sg[1].iov_base = NULL;
				sg[1].iov_len = 0;
				args->send_data = (void *) sg;
				args->send_sg_len = 1;
				sg_len++;
				cmd->callback = osd_read_callback;
				cmd->callback_arg = sg;
			} else {
				if (read_data)
					iscsi_free_atomic(read_data);
				args->length = 0;	/* Need a better way of
							 * specifying an error.. */
			}
			break;

		case OSD_GET_ATTR:
			iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "OSD_GET_ATTR(lun %llu, GroupID 0x%x, UserID 0x%llx)\n",
			      args->lun, osd_args.GroupID, osd_args.UserID);
			args->status = 0;
			break;

		case OSD_SET_ATTR:
			args->status = 0;
			break;
		}

		if (args->status)
			goto done;

		/*
	         * Send back requested attributes
	         */

		if (osd_args.get_attributes_list_length || osd_args.get_attributes_page) {
			if ((get_data = iscsi_malloc_atomic(osd_args.get_attributes_allocation_length)) == NULL) {
				iscsi_trace_error(__FILE__, __LINE__, "iscsi_malloc_atomic() failed\n");
				goto done;
			}
		}
		if (osd_args.get_attributes_list_length) {
			int             i;

			for (i = 0; i < osd_args.get_attributes_list_length;) {
				page = ISCSI_NTOHL(*((uint32_t *) (&(get_list[i]))));
				i += 4;
				index = ISCSI_NTOHL(*((uint32_t *) (&(get_list[i]))));
				i += 4;
				iscsi_trace(TRACE_OSD, __FILE__, __LINE__, "GET(0x%x,%u)\n", page, index);

				switch (page) {
				case 0x40000001:
					switch (index) {
					case 0x1:
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(page);
						attr_len += 4;
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(index);
						attr_len += 4;
						*((uint16_t *) & get_data[attr_len]) = ISCSI_HTONS(4);
						attr_len += 2;
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(GroupID);
						attr_len += 4;
						break;
					default:
						iscsi_trace_error(__FILE__, __LINE__, "unknown attr index %u\n", index);
						goto done;
					}
					break;
				case 0x00000001:
					switch (index) {
					case 0x1:
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(page);
						attr_len += 4;
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(index);
						attr_len += 4;
						*((uint16_t *) & get_data[attr_len]) = ISCSI_HTONS(4);
						attr_len += 2;
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(GroupID);
						attr_len += 4;
						break;
					case 0x2:
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(page);
						attr_len += 4;
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(index);
						attr_len += 4;
						*((uint16_t *) & get_data[attr_len]) = ISCSI_HTONS(8);
						attr_len += 2;
						*((uint64_t *) & get_data[attr_len]) = ISCSI_HTONLL(UserID);
						attr_len += 8;
						break;
					default:
						iscsi_trace_error(__FILE__, __LINE__, "unknown attr index %u\n", index);
						goto done;
					}
					break;

					/* Vendor-specific */

				case 0x30000000:
					switch (index) {
					case 0x1:
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(page);
						attr_len += 4;
						*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(index);
						attr_len += 4;
						*((uint16_t *) & get_data[attr_len]) = ISCSI_HTONS(480);
						attr_len += 2;
						sprintf(FileName, "%s/lun_%llu/0x%x/0x%llx.0x%x.%u",
							base_dir, args->lun, osd_args.GroupID, osd_args.UserID, page, index);
						if ((rc = open(FileName, O_RDONLY, 0644)) == -1) {
							iscsi_trace_error(__FILE__, __LINE__, "error opening \"%s\": errno %d\n", FileName, errno);
						}
						if (read(rc, get_data + attr_len, 480) != 480) {
							iscsi_trace_error(__FILE__, __LINE__, "read() failed\n");
							goto done;
						}
						close(rc);
						attr_len += 480;
						break;
					default:
						iscsi_trace_error(__FILE__, __LINE__, "unknown vendor attr index %u\n", index);
						goto done;
					}
					break;

				default:
					iscsi_trace_error(__FILE__, __LINE__, "unknown page 0x%x\n", page);
					goto done;
				}
			}
		}
		if (osd_args.get_attributes_page) {

			/*
			 * Right now, if we get a request for an entire page,
			 * we return only one attribute.
			 */

			page = osd_args.get_attributes_page;

			switch (osd_args.get_attributes_page) {
			case 0x40000001:
				index = 1;
				*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(page);
				attr_len += 4;
				*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(index);
				attr_len += 4;
				*((uint16_t *) & get_data[attr_len]) = ISCSI_HTONS(4);
				attr_len += 2;
				*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(GroupID);
				attr_len += 4;
				break;

			case 0x00000001:
				index = 2;
				*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(page);
				attr_len += 4;
				*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(index);
				attr_len += 4;
				*((uint16_t *) & get_data[attr_len]) = ISCSI_HTONS(8);
				attr_len += 2;
				*((uint64_t *) & get_data[attr_len]) = ISCSI_HTONLL(UserID);
				attr_len += 8;
				break;

			case 0x30000000:
				index = 1;
				*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(page);
				attr_len += 4;
				*((uint32_t *) & get_data[attr_len]) = ISCSI_HTONL(index);
				attr_len += 4;
				*((uint16_t *) & get_data[attr_len]) = ISCSI_HTONS(480);
				attr_len += 2;
				sprintf(FileName, "%s/lun_%llu/0x%x/0x%llx.0x%x.%u",
					base_dir, args->lun, osd_args.GroupID, osd_args.UserID, page, index);
				if ((rc = open(FileName, O_RDONLY, 0644)) == -1) {
					iscsi_trace_error(__FILE__, __LINE__, "error opening \"%s\": errno %d\n", FileName, errno);
				}
				if (read(rc, get_data + attr_len, 480) != 480) {
					iscsi_trace_error(__FILE__, __LINE__, "read() failed\n");
					goto done;
				}
				close(rc);
				attr_len += 480;
				break;
			default:
				iscsi_trace_error(__FILE__, __LINE__, "page not yet supported\n");
				goto done;
			}
		}
		if (attr_len) {
			if (attr_len != osd_args.get_attributes_allocation_length) {
				iscsi_trace_error(__FILE__, __LINE__, "allocation lengths differ: got %u, expected %u\n",
					    osd_args.get_attributes_allocation_length, attr_len);
				goto done;
			}
			if (!args->status) {
				args->input = 1;
				sg[sg_len].iov_base = get_data;
				sg[sg_len].iov_len = osd_args.get_attributes_allocation_length;
				sg_len++;
				sg[sg_len].iov_base = NULL;
				sg[sg_len].iov_len = 0;
				args->send_data = (void *) sg;
				args->send_sg_len++;
				cmd->callback = osd_read_callback;
				cmd->callback_arg = sg;
			} else {
				if (get_data)
					iscsi_free_atomic(get_data);
			}
		}
		break;

	default:
		iscsi_trace_error(__FILE__, __LINE__, "UNKNOWN OPCODE 0x%x\n", args->cdb[0]);
		args->status = 0x01;
		break;
	}


done:
	iscsi_trace(TRACE_SCSI_DEBUG, __FILE__, __LINE__, "SCSI op 0x%x: done (status 0x%x)\n", args->cdb[0], args->status);
	if (set_list) {
		iscsi_free_atomic(set_list);
	}
	if (get_list) {
		iscsi_free_atomic(get_list);
	}
	if (write_data) {
		iscsi_free_atomic(write_data);
	}
	return 0;
}

/* ARGSUSED */
int 
device_shutdown(target_session_t *sess)
{
	return 0;
}
