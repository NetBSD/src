/*	$NetBSD: nouveau_acpi.h,v 1.3.4.1 2024/10/04 11:40:51 martin Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NOUVEAU_ACPI_H__
#define __NOUVEAU_ACPI_H__

#include <linux/acpi.h>

#define ROM_BIOS_PAGE 4096

#if defined(CONFIG_ACPI) && defined(CONFIG_X86)
bool nouveau_is_optimus(void);
bool nouveau_is_v1_dsm(void);
void nouveau_register_dsm_handler(void);
void nouveau_unregister_dsm_handler(void);
void nouveau_switcheroo_optimus_dsm(void);
int nouveau_acpi_get_bios_chunk(uint8_t *bios, int offset, int len);
#ifdef __NetBSD__
bool nouveau_acpi_rom_supported(struct acpi_devnode *);
#else
bool nouveau_acpi_rom_supported(struct device *);
#endif
void *nouveau_acpi_edid(struct drm_device *, struct drm_connector *);
#else
static inline bool nouveau_is_optimus(void) { return false; };
static inline bool nouveau_is_v1_dsm(void) { return false; };
static inline void nouveau_register_dsm_handler(void) {}
static inline void nouveau_unregister_dsm_handler(void) {}
static inline void nouveau_switcheroo_optimus_dsm(void) {}
#ifdef __NetBSD__
static inline bool nouveau_acpi_rom_supported(struct acpi_devnode *acpidev) { return false; }
#else
static inline bool nouveau_acpi_rom_supported(struct device *dev) { return false; }
#endif
static inline int nouveau_acpi_get_bios_chunk(uint8_t *bios, int offset, int len) { return -EINVAL; }
static inline void *nouveau_acpi_edid(struct drm_device *dev, struct drm_connector *connector) { return NULL; }
#endif

#endif
