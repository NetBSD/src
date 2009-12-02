/*	$NetBSD: lvm2app.h,v 1.1.1.1 2009/12/02 00:26:15 haad Exp $	*/

/*
 * Copyright (C) 2008,2009 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _LIB_LVM2APP_H
#define _LIB_LVM2APP_H

#include "libdevmapper.h"

#include <stdint.h>


/******************************** WARNING ***********************************
 *
 * NOTE: This API is under development and subject to change at any time.
 *
 * Please send feedback to lvm-devel@redhat.com
 *
 *********************************** WARNING ********************************/

/*************************** Design Overview ********************************/

/**
 * \mainpage LVM library API
 *
 * The API is designed around the following basic LVM objects:
 * 1) Physical Volume (PV) 2) Volume Group (VG) 3) Logical Volume (LV).
 *
 * The library provides functions to list the objects in a system,
 * get and set object properties (such as names, UUIDs, and sizes), as well
 * as create/remove objects and perform more complex operations and
 * transformations. Each object instance is represented by a handle, and
 * handles are passed to and from the functions to perform the operations.
 *
 * A central object in the library is the Volume Group, represented by the
 * VG handle, vg_t. Performing an operation on a PV or LV object first
 * requires obtaining a VG handle. Once the vg_t has been obtained, it can
 * be used to enumerate the pv_t's and lv_t's within that vg_t. Attributes
 * of these objects can then be queried.
 *
 * A volume group handle may be obtained with read or write permission.
 * Any attempt to change a property of a pv_t, vg_t, or lv_t without
 * obtaining write permission on the vg_t will fail with EPERM.
 *
 * An application first opening a VG read-only, then later wanting to change
 * a property of an object must first close the VG and re-open with write
 * permission. Currently liblvm provides no mechanism to determine whether
 * the VG has changed on-disk in between these operations - this is the
 * application's responsiblity. One way the application can ensure the VG
 * has not changed is to save the "vg_seqno" field after opening the VG with
 * READ permission. If the application later needs to modify the VG, it can
 * close the VG and re-open with WRITE permission. It should then check
 * whether the original "vg_seqno" obtained with READ permission matches
 * the new one obtained with WRITE permission.
 */

/**
 * Retrieve the library version.
 *
 * The library version is the same format as the full LVM version.
 * The format is as follows:
 *    LVM_MAJOR.LVM_MINOR.LVM_PATCHLEVEL(LVM_LIBAPI)[-LVM_RELEASE]
 * An application wishing to determine compatibility with a particular version
 * of the library should check at least the LVM_MAJOR, LVM_MINOR, and
 * LVM_LIBAPI numbers.  For example, assume the full LVM version is
 * 2.02.50(1)-1.  The application should verify the "2.02" and the "(1)".
 *
 * \return  A string describing the library version.
 */
const char *lvm_library_get_version(void);

/******************************** structures ********************************/

/**
 * Opaque structures - do not use directly.  Internal structures may change
 * without notice between releases, whereas this API will be changed much less
 * frequently.  Backwards compatibility will normally be preserved in future
 * releases.  On any occasion when the developers do decide to break backwards
 * compatibility in any significant way, the LVM_LIBAPI number (included in
 * the library's soname) will be incremented.
 */
struct lvm;
struct physical_volume;
struct volume_group;
struct logical_volume;

/**
 * lvm handle.
 *
 * This is the base handle that is needed to open and create objects such as
 * volume groups and logical volumes.  In addition, this handle provides a
 * context for error handling information, saving any error number (see
 * lvm_errno) and error message (see lvm_errmsg) that any function may
 * generate.
 */
typedef struct lvm *lvm_t;

/**
 * Volume group object.
 *
 * This object can be either a read-only object or a read-write object
 * depending on the mode it was returned by a function. Create functions
 * return a read-write object, but open functions have the argument mode to
 * define if the object can be modified or not.
 */
typedef struct volume_group *vg_t;

/**
 * Logical Volume object.
 *
 * This object is bound to a volume group and has the same mode of the volume
 * group.  Changes will be written to disk when the volume group gets
 * committed to disk.
 */
typedef struct logical_volume *lv_t;

/**
 * Physical volume object.
 *
 * This object is bound to a volume group and has the same mode of the volume
 * group.  Changes will be written to disk when the volume group gets
 * committed to disk.
 */
typedef struct physical_volume *pv_t;

/**
 * Logical Volume object list.
 *
 * Lists of these structures are returned by lvm_vg_list_pvs.
 */
typedef struct lvm_lv_list {
	struct dm_list list;
	lv_t lv;
} lv_list_t;

/**
 * Physical volume object list.
 *
 * Lists of these structures are returned by lvm_vg_list_pvs.
 */
typedef struct lvm_pv_list {
	struct dm_list list;
	pv_t pv;
} pv_list_t;

/**
 * String list.
 *
 * This string list contains read-only strings.
 * Lists of these structures are returned by lvm_list_vg_names and
 * lvm_list_vg_uuids.
 */
struct lvm_str_list {
	struct dm_list list;
	const char *str;
};

/*************************** generic lvm handling ***************************/
/**
 * Create a LVM handle.
 *
 * Once all LVM operations have been completed, use lvm_quit to release
 * the handle and any associated resources.
 *
 * \param system_dir
 * Set an alternative LVM system directory. Use NULL to use the
 * default value. If the environment variable LVM_SYSTEM_DIR is set,
 * it will override any system_dir setting.
 *
 * \return
 * A valid LVM handle is returned or NULL if there has been a
 * memory allocation problem. You have to check if an error occured
 * with the lvm_error function.
 */
lvm_t lvm_init(const char *system_dir);

/**
 * Destroy a LVM handle allocated with lvm_init.
 *
 * This function should be used after all LVM operations are complete or after
 * an unrecoverable error.  Destroying the LVM handle frees the memory and
 * other resources associated with the handle.  Once destroyed, the handle
 * cannot be used subsequently.
 *
 * \param   libh
 * Handle obtained from lvm_init.
 */
void lvm_quit(lvm_t libh);

/**
 * Reload the original configuration from the system directory.
 *
 * This function should be used when any LVM configuration changes in the LVM
 * system_dir or by another lvm_config* function, and the change is needed by
 * the application.
 *
 * \param   libh
 * Handle obtained from lvm_init.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_config_reload(lvm_t libh);

/**
 * Override the LVM configuration with a configuration string.
 *
 * This function is equivalent to the --config option on lvm commands.
 * Once this API has been used to over-ride the configuration,
 * use lvm_config_reload to apply the new settings.
 *
 * \param   libh
 * Handle obtained from lvm_init.
 *
 * \param   config_string
 * LVM configuration string to apply.  See the lvm.conf file man page
 * for the format of the config string.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_config_override(lvm_t libh, const char *config_string);

/**
 * Return stored error no describing last LVM API error.
 *
 * Users of liblvm should use lvm_errno to determine the details of a any
 * failure of the last call.  A basic success or fail is always returned by
 * every function, either by returning a 0 or -1, or a non-NULL / NULL.
 * If a function has failed, lvm_errno may be used to get a more specific
 * error code describing the failure.  In this way, lvm_errno may be used
 * after every function call, even after a 'get' function call that simply
 * returns a value.
 *
 * \param   libh
 * Handle obtained from lvm_init.
 *
 * \return
 * An errno value describing the last LVM error.
 */
int lvm_errno(lvm_t libh);

/**
 * Return stored error message describing last LVM error.
 *
 * This function may be used in conjunction with lvm_errno to obtain more
 * specific error information for a function that is known to have failed.
 *
 * \param   libh
 * Handle obtained from lvm_init.
 *
 * \return
 * An error string describing the last LVM error.
 */
const char *lvm_errmsg(lvm_t libh);

/**
 * Scan all devices on the system for VGs and LVM metadata.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_scan(lvm_t libh);

/*************************** volume group handling **************************/

/**
 * Return the list of volume group names.
 *
 * The memory allocated for the list is tied to the lvm_t handle and will be
 * released when lvm_quit is called.
 *
 * NOTE: This function normally does not scan devices in the system for LVM
 * metadata.  To scan the system, use lvm_scan.
 * NOTE: This function currently returns hidden VG names.  These names always
 * begin with a "#" and should be filtered out and not used.
 *
 * To process the list, use the dm_list iterator functions.  For example:
 *      vg_t vg;
 *      struct dm_list *vgnames;
 *      struct lvm_str_list *strl;
 *
 *      vgnames = lvm_list_vg_names(libh);
 *	dm_list_iterate_items(strl, vgnames) {
 *		vgname = strl->str;
 *              vg = lvm_vg_open(libh, vgname, "r");
 *              // do something with vg
 *              lvm_vg_close(vg);
 *      }
 *
 *
 * \return
 * A list with entries of type struct lvm_str_list, containing the
 * VG name strings of the Volume Groups known to the system.
 * NULL is returned if unable to allocate memory.
 * An empty list (verify with dm_list_empty) is returned if no VGs
 * exist on the system.
 */
struct dm_list *lvm_list_vg_names(lvm_t libh);

/**
 * Return the list of volume group uuids.
 *
 * The memory allocated for the list is tied to the lvm_t handle and will be
 * released when lvm_quit is called.
 *
 * NOTE: This function normally does not scan devices in the system for LVM
 * metadata.  To scan the system, use lvm_scan.
 * NOTE: This function currently returns hidden VG names.  These names always
 * begin with a "#" and should be filtered out and not used.
 *
 * \param   libh
 * Handle obtained from lvm_init.
 *
 * \return
 * A list with entries of type struct lvm_str_list, containing the
 * VG UUID strings of the Volume Groups known to the system.
 * NULL is returned if unable to allocate memory.
 * An empty list (verify with dm_list_empty) is returned if no VGs
 * exist on the system.
 */
struct dm_list *lvm_list_vg_uuids(lvm_t libh);

/**
 * Open an existing VG.
 *
 * Open a VG for reading or writing.
 *
 * \param   libh
 * Handle obtained from lvm_init.
 *
 * \param   vgname
 * Name of the VG to open.
 *
 * \param   mode
 * Open mode - either "r" (read) or "w" (read/write).
 * Any other character results in an error with EINVAL set.
 *
 * \param   flags
 * Open flags - currently ignored.
 *
 * \return  non-NULL VG handle (success) or NULL (failure).
 */
vg_t lvm_vg_open(lvm_t libh, const char *vgname, const char *mode,
		  uint32_t flags);

/**
 * Create a VG with default parameters.
 *
 * This function creates a Volume Group object in memory.
 * Upon success, other APIs may be used to set non-default parameters.
 * For example, to set a non-default extent size, use lvm_vg_set_extent_size.
 * Next, to add physical storage devices to the volume group, use
 * lvm_vg_extend for each device.
 * Once all parameters are set appropriately and all devices are added to the
 * VG, use lvm_vg_write to commit the new VG to disk, and lvm_vg_close to
 * release the VG handle.
 *
 * \param   libh
 * Handle obtained from lvm_init.
 *
 * \param   vg_name
 * Name of the VG to open.
 *
 * \return
 * non-NULL vg handle (success) or NULL (failure)
 */
vg_t lvm_vg_create(lvm_t libh, const char *vg_name);

 /**
 * Write a VG to disk.
 *
 * This function commits the Volume Group object referenced by the VG handle
 * to disk. Upon failure, retry the operation and/or release the VG handle
 * with lvm_vg_close.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_vg_write(vg_t vg);

/**
 * Remove a VG from the system.
 *
 * This function removes a Volume Group object in memory, and requires
 * calling lvm_vg_write to commit the removal to disk.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_vg_remove(vg_t vg);

/**
 * Close a VG opened with lvm_vg_create or lvm_vg_open.
 *
 * This function releases a VG handle and any resources associated with the
 * handle.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_vg_close(vg_t vg);

/**
 * Extend a VG by adding a device.
 *
 * This function requires calling lvm_vg_write to commit the change to disk.
 * After successfully adding a device, use lvm_vg_write to commit the new VG
 * to disk.  Upon failure, retry the operation or release the VG handle with
 * lvm_vg_close.
 * If the device is not initialized for LVM use, it will be initialized
 * before adding to the VG.  Although some internal checks are done,
 * the caller should be sure the device is not in use by other subsystems
 * before calling lvm_vg_extend.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \param   device
 * Absolute pathname of device to add to VG.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_vg_extend(vg_t vg, const char *device);

/**
 * Reduce a VG by removing an unused device.
 *
 * This function requires calling lvm_vg_write to commit the change to disk.
 * After successfully removing a device, use lvm_vg_write to commit the new VG
 * to disk.  Upon failure, retry the operation or release the VG handle with
 * lvm_vg_close.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \param   device
 * Name of device to remove from VG.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_vg_reduce(vg_t vg, const char *device);

/**
 * Set the extent size of a VG.
 *
 * This function requires calling lvm_vg_write to commit the change to disk.
 * After successfully setting a new extent size, use lvm_vg_write to commit
 * the new VG to disk.  Upon failure, retry the operation or release the VG
 * handle with lvm_vg_close.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \param   new_size
 * New extent size in bytes.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_vg_set_extent_size(vg_t vg, uint32_t new_size);

/**
 * Get whether or not a volume group is clustered.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * 1 if the VG is clustered, 0 if not
 */
uint64_t lvm_vg_is_clustered(vg_t vg);

/**
 * Get whether or not a volume group is exported.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * 1 if the VG is exported, 0 if not
 */
uint64_t lvm_vg_is_exported(vg_t vg);

/**
 * Get whether or not a volume group is a partial volume group.
 *
 * When one or more physical volumes belonging to the volume group
 * are missing from the system the volume group is a partial volume
 * group.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * 1 if the VG is PVs, 0 if not
 */
uint64_t lvm_vg_is_partial(vg_t vg);

/**
 * Get the current metadata sequence number of a volume group.
 *
 * The metadata sequence number is incrented for each metadata change.
 * Applications may use the sequence number to determine if any LVM objects
 * have changed from a prior query.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Metadata sequence number.
 */
uint64_t lvm_vg_get_seqno(const vg_t vg);

/**
 * Get the current name of a volume group.
 *
 * Memory is allocated using dm_malloc() and caller must free the memory
 * using dm_free().
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Copy of the uuid string.
 */
char *lvm_vg_get_uuid(const vg_t vg);

/**
 * Get the current uuid of a volume group.
 *
 * Memory is allocated using dm_malloc() and caller must free the memory
 * using dm_free().
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Copy of the name.
 */
char *lvm_vg_get_name(const vg_t vg);

/**
 * Get the current size in bytes of a volume group.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Size in bytes.
 */
uint64_t lvm_vg_get_size(const vg_t vg);

/**
 * Get the current unallocated space in bytes of a volume group.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Free size in bytes.
 */
uint64_t lvm_vg_get_free_size(const vg_t vg);

/**
 * Get the current extent size in bytes of a volume group.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Extent size in bytes.
 */
uint64_t lvm_vg_get_extent_size(const vg_t vg);

/**
 * Get the current number of total extents of a volume group.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Extent count.
 */
uint64_t lvm_vg_get_extent_count(const vg_t vg);

/**
 * Get the current number of free extents of a volume group.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Free extent count.
 */
uint64_t lvm_vg_get_free_extent_count(const vg_t vg);

/**
 * Get the current number of physical volumes of a volume group.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Physical volume count.
 */
uint64_t lvm_vg_get_pv_count(const vg_t vg);

/**
 * Get the maximum number of physical volumes allowed in a volume group.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Maximum number of physical volumes allowed in a volume group.
 */
uint64_t lvm_vg_get_max_pv(const vg_t vg);

/**
 * Get the maximum number of logical volumes allowed in a volume group.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * Maximum number of logical volumes allowed in a volume group.
 */
uint64_t lvm_vg_get_max_lv(const vg_t vg);

/************************** logical volume handling *************************/

/**
 * Return a list of LV handles for a given VG handle.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * A list of lv_list_t structures containing lv handles for this vg.
 * If no LVs exist on the given VG, NULL is returned.
 */
struct dm_list *lvm_vg_list_lvs(vg_t vg);

/**
 * Create a linear logical volume.
 * This function commits the change to disk and does _not_ require calling
 * lvm_vg_write.
 * NOTE: The commit behavior of this function is subject to change
 * as the API is developed.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \param   name
 * Name of logical volume to create.
 *
 * \param   size
 * Size of logical volume in extents.
 *
 * \return
 * non-NULL handle to an LV object created, or NULL if creation fails.
 *
 */
lv_t lvm_vg_create_lv_linear(vg_t vg, const char *name, uint64_t size);

/**
 * Activate a logical volume.
 *
 * This function is the equivalent of the lvm command "lvchange -ay".
 *
 * NOTE: This function cannot currently handle LVs with an in-progress pvmove or
 * lvconvert.
 *
 * \param   lv
 * Logical volume handle.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_lv_activate(lv_t lv);

/**
 * Deactivate a logical volume.
 *
 * This function is the equivalent of the lvm command "lvchange -an".
 *
 * \param   lv
 * Logical volume handle.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_lv_deactivate(lv_t lv);

/**
 * Remove a logical volume from a volume group.
 *
 * This function commits the change to disk and does _not_ require calling
 * lvm_vg_write.
 * NOTE: The commit behavior of this function is subject to change
 * as the API is developed.
 * Currently only removing linear LVs are possible.
 *
 * \param   lv
 * Logical volume handle.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_vg_remove_lv(lv_t lv);

/**
 * Get the current name of a logical volume.
 *
 * Memory is allocated using dm_malloc() and caller must free the memory
 * using dm_free().
 *
 * \param   lv
 * Logical volume handle.
 *
 * \return
 * Copy of the uuid string.
 */
char *lvm_lv_get_uuid(const lv_t lv);

/**
 * Get the current uuid of a logical volume.
 *
 * Memory is allocated using dm_malloc() and caller must free the memory
 * using dm_free().
 *
 * \param   lv
 * Logical volume handle.
 *
 * \return
 * Copy of the name.
 */
char *lvm_lv_get_name(const lv_t lv);

/**
 * Get the current size in bytes of a logical volume.
 *
 * \param   lv
 * Logical volume handle.
 *
 * \return
 * Size in bytes.
 */
uint64_t lvm_lv_get_size(const lv_t lv);

/**
 * Get the current activation state of a logical volume.
 *
 * \param   lv
 * Logical volume handle.
 *
 * \return
 * 1 if the LV is active in the kernel, 0 if not
 */
uint64_t lvm_lv_is_active(const lv_t lv);

/**
 * Get the current suspended state of a logical volume.
 *
 * \param   lv
 * Logical volume handle.
 *
 * \return
 * 1 if the LV is suspended in the kernel, 0 if not
 */
uint64_t lvm_lv_is_suspended(const lv_t lv);

/**
 * Resize logical volume to new_size bytes.
 *
 * NOTE: This function is currently not implemented.
 *
 * \param   lv
 * Logical volume handle.
 *
 * \param   new_size
 * New size in bytes.
 *
 * \return
 * 0 (success) or -1 (failure).
 *
 */
int lvm_lv_resize(const lv_t lv, uint64_t new_size);

/************************** physical volume handling ************************/

/**
 * Physical volume handling should not be needed anymore. Only physical volumes
 * bound to a vg contain useful information. Therefore the creation,
 * modification and the removal of orphan physical volumes is not suported.
 */

/**
 * Return a list of PV handles for a given VG handle.
 *
 * \param   vg
 * VG handle obtained from lvm_vg_create or lvm_vg_open.
 *
 * \return
 * A list of pv_list_t structures containing pv handles for this vg.
 * If no PVs exist on the given VG, NULL is returned.
 */
struct dm_list *lvm_vg_list_pvs(vg_t vg);

/**
 * Get the current uuid of a logical volume.
 *
 * Memory is allocated using dm_malloc() and caller must free the memory
 * using dm_free().
 *
 * \param   pv
 * Physical volume handle.
 *
 * \return
 * Copy of the uuid string.
 */
char *lvm_pv_get_uuid(const pv_t pv);

/**
 * Get the current name of a logical volume.
 *
 * Memory is allocated using dm_malloc() and caller must free the memory
 * using dm_free().
 *
 * \param   pv
 * Physical volume handle.
 *
 * \return
 * Copy of the name.
 */
char *lvm_pv_get_name(const pv_t pv);

/**
 * Get the current number of metadata areas in the physical volume.
 *
 * \param   pv
 * Physical volume handle.
 *
 * \return
 * Number of metadata areas in the PV.
 */
uint64_t lvm_pv_get_mda_count(const pv_t pv);

/**
 * Resize physical volume to new_size bytes.
 *
 * NOTE: This function is currently not implemented.
 *
 * \param   pv
 * Physical volume handle.
 *
 * \param   new_size
 * New size in bytes.
 *
 * \return
 * 0 (success) or -1 (failure).
 */
int lvm_pv_resize(const pv_t pv, uint64_t new_size);

#endif /* _LIB_LVM2APP_H */
