#ifndef __NETBSD_DM_H__
#define __NETBSD_DM_H__

#include <prop/proplib.h>

#define DM_CMD_LEN 16

#define DM_IOCTL 0xfd

#define DM_IOCTL_CMD 0

#define NETBSD_DM_IOCTL       _IOWR(DM_IOCTL, DM_IOCTL_CMD, struct plistref)


/*
 * DM-ioctl dictionary.
 *
 * This contains general information about dm device.
 *
 * <dict>
 *     <key>command</key>
 *     <string>...</string>
 *
 *     <key>event_nr</key>
 *     <integer>...</integer>
 *
 *     <key>name</key>
 *     <string>...</string>
 *
 *     <key>uuid</key>
 *     <string>...</string>
 *
 *     <key>dev</key>
 *     <integer></integer> 
 *
 *     <key>flags</key>
 *     <integer></integer>
 *
 *     <key>version</key>
 *      <array>
 *       <integer>...</integer>
 *       <integer>...</integer>
 *       <integer>...</integer>
 *      </array>
 *
 *      <key>cmd_data</key>
 *       <array>
 *        <!-- See below for command
 *             specific dictionaries -->
 *       </array>
 * </dict>
 *
 * Available commands from _cmd_data_v4.
 *
 * create, reload, remove, remove_all, suspend,
 * resume, info, deps, rename, version, status,
 * table, waitevent, names, clear, mknodes,
 * targets, message, setgeometry
 *
 */

/*
 * DM_LIST_VERSIONS command dictionary entry.
 * Lists all available targets with their version.
 *
 * <array>
 *   <dict>
 *    <key>name<key>
 *    <string>...</string>
 *
 *    <key>version</key>
 *      <array>
 *       <integer>...</integer>
 *       <integer>...</integer>
 *       <integer>...</integer>
 *      </array>
 *   </dict>
 * </array>
 *
 */

#define DM_IOCTL_COMMAND      "command"
#define DM_IOCTL_VERSION      "version"
#define DM_IOCTL_OPEN         "open_count"
#define DM_IOCTL_DEV          "dev"
#define DM_IOCTL_NAME         "name"
#define DM_IOCTL_UUID         "uuid"
#define DM_IOCTL_EVENT        "event_nr"
#define DM_IOCTL_FLAGS        "flags"
#define DM_IOCTL_CMD_DATA     "cmd_data"

#define DM_TARGETS_NAME       "name"
#define DM_TARGETS_VERSION    "ver"

#define DM_DEV_NEWNAME        "newname"
#define DM_DEV_NAME           "name"
#define DM_DEV_DEV            "dev"

#define DM_TABLE_TYPE         "type"
#define DM_TABLE_START        "start"
#define DM_TABLE_STAT         "status"
#define DM_TABLE_LENGTH       "length"
#define DM_TABLE_PARAMS       "params"


/* Status bits */
#define DM_READONLY_FLAG	(1 << 0) /* In/Out */
#define DM_SUSPEND_FLAG		(1 << 1) /* In/Out */
#define DM_EXISTS_FLAG          (1 << 2) /* In/Out */ /* XXX. This flag is undocumented. */ 
#define DM_PERSISTENT_DEV_FLAG	(1 << 3) /* In */

/*
 * Flag passed into ioctl STATUS command to get table information
 * rather than current status.
 */
#define DM_STATUS_TABLE_FLAG	(1 << 4) /* In */

/*
 * Flags that indicate whether a table is present in either of
 * the two table slots that a device has.
 */
#define DM_ACTIVE_PRESENT_FLAG   (1 << 5) /* Out */
#define DM_INACTIVE_PRESENT_FLAG (1 << 6) /* Out */

/*
 * Indicates that the buffer passed in wasn't big enough for the
 * results.
 */
#define DM_BUFFER_FULL_FLAG	(1 << 8) /* Out */

/*
 * This flag is now ignored.
 */
#define DM_SKIP_BDGET_FLAG	(1 << 9) /* In */

/*
 * Set this to avoid attempting to freeze any filesystem when suspending.
 */
#define DM_SKIP_LOCKFS_FLAG	(1 << 10) /* In */

/*
 * Set this to suspend without flushing queued ios.
 */
#define DM_NOFLUSH_FLAG		(1 << 11) /* In */


#ifdef __LIB_DEVMAPPER__

#  define MAJOR(x) major((x))
#  define MINOR(x) minor((x))
#  define MKDEV(x,y) makedev((x),(y))

/* Name of device-mapper driver in kernel */
#define DM_NAME "dm"

/* Types for nbsd_get_dm_major */
#define DM_CHAR_MAJOR 1
#define DM_BLOCK_MAJOR 2	

/* libdm_netbsd.c */

int nbsd_get_dm_major(uint32_t *, int); /* Get dm device major numbers */

int nbsd_dmi_add_cmd(const char *, prop_dictionary_t);
int nbsd_dmi_add_version(const int [3], prop_dictionary_t);
int nbsd_dm_add_uint(const char *, uint64_t, prop_dictionary_t);
int nbsd_dm_add_str(const char *, char *, prop_dictionary_t );

struct dm_ioctl* nbsd_dm_dict_to_dmi(prop_dictionary_t, const int);

#endif /* __LIB_DEVMAPPER__ */

#endif /* __NETBSD_DM_H__ */
