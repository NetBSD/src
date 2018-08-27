/*	$NetBSD: intel_acpi.c,v 1.6 2018/08/27 04:58:24 riastradh Exp $	*/

/*
 * Intel ACPI functions
 *
 * _DSM related code stolen from nouveau_acpi.c.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intel_acpi.c,v 1.6 2018/08/27 04:58:24 riastradh Exp $");

#include <linux/pci.h>
#include <linux/acpi.h>
#include <drm/drmP.h>
#include "i915_drv.h"

#ifdef CONFIG_ACPI

#ifdef __NetBSD__
#include <dev/acpi/acpireg.h>
#define _COMPONENT ACPI_BUTTON_COMPONENT
ACPI_MODULE_NAME("acpi_intel_brightness")

static ACPI_OBJECT *
acpi_evaluate_dsm(ACPI_HANDLE handle, const uint8_t *uuid, int rev, int func,
    ACPI_OBJECT *argv4)
{
	ACPI_OBJECT_LIST arg;
	ACPI_OBJECT params[4];
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	arg.Count = 4;
	arg.Pointer = params;
	params[0].Type = ACPI_TYPE_BUFFER;
	params[0].Buffer.Length = 16;
	params[0].Buffer.Pointer = (char *)__UNCONST(uuid);
	params[1].Type = ACPI_TYPE_INTEGER;
	params[1].Integer.Value = rev;
	params[2].Type = ACPI_TYPE_INTEGER;
	params[2].Integer.Value = func;
	if (argv4 != NULL) {
		params[3] = *argv4;
	} else {
		params[3].Type = ACPI_TYPE_PACKAGE;
		params[3].Package.Count = 0;
		params[3].Package.Elements = NULL;
	}

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_LOCAL_BUFFER;

	rv = AcpiEvaluateObject(handle, "_DSM", &arg, &buf);
	if (ACPI_SUCCESS(rv))
		return (ACPI_OBJECT *)buf.Pointer;
	return NULL;
}

static inline ACPI_OBJECT *
acpi_evaluate_dsm_typed(ACPI_HANDLE handle, const uint8_t *uuid, int rev,
    int func, ACPI_OBJECT *argv4, ACPI_OBJECT_TYPE type)
{
	ACPI_OBJECT *obj;

	obj = acpi_evaluate_dsm(handle, uuid, rev, func, argv4);
	if (obj != NULL && obj->Type != type) {
		ACPI_FREE(obj);
		obj = NULL;
	}
	return obj;
}

#define	ACPI_INIT_DSM_ARGV4(cnt, eles)		\
{						\
	.Package.Type = ACPI_TYPE_PACKAGE,	\
	.Package.Count = (cnt),			\
	.Package.Elements = (eles)		\
}

static bool
acpi_check_dsm(ACPI_HANDLE handle, const uint8_t *uuid, int rev, uint64_t funcs)
{
	ACPI_OBJECT *obj;
	uint64_t mask = 0;
	int i;

	if (funcs == 0)
		return false;

	obj = acpi_evaluate_dsm(handle, uuid, rev, 0, NULL);
	if (obj == NULL)
		return false;

	if (obj->Type == ACPI_TYPE_INTEGER)
		mask = obj->Integer.Value;
	else if (obj->Type == ACPI_TYPE_BUFFER)
		for (i = 0; i < obj->Buffer.Length && i < 8; i++)
			mask |= (uint64_t)obj->Buffer.Pointer[i] << (i * 8);
	ACPI_FREE(obj);

	if ((mask & 0x1) == 0x1 && (mask & funcs) == funcs)
		return true;
	return false;
}
#endif

#define INTEL_DSM_REVISION_ID 1 /* For Calpella anyway... */
#define INTEL_DSM_FN_PLATFORM_MUX_INFO 1 /* No args */

static struct intel_dsm_priv {
#ifdef __NetBSD__
	ACPI_HANDLE dhandle;
#else
	acpi_handle dhandle;
#endif
} intel_dsm_priv;

static const u8 intel_dsm_guid[] = {
	0xd3, 0x73, 0xd8, 0x7e,
	0xd0, 0xc2,
	0x4f, 0x4e,
	0xa8, 0x54,
	0x0f, 0x13, 0x17, 0xb0, 0x1c, 0x2c
};

static const char *intel_dsm_port_name(u8 id)
{
	switch (id) {
	case 0:
		return "Reserved";
	case 1:
		return "Analog VGA";
	case 2:
		return "LVDS";
	case 3:
		return "Reserved";
	case 4:
		return "HDMI/DVI_B";
	case 5:
		return "HDMI/DVI_C";
	case 6:
		return "HDMI/DVI_D";
	case 7:
		return "DisplayPort_A";
	case 8:
		return "DisplayPort_B";
	case 9:
		return "DisplayPort_C";
	case 0xa:
		return "DisplayPort_D";
	case 0xb:
	case 0xc:
	case 0xd:
		return "Reserved";
	case 0xe:
		return "WiDi";
	default:
		return "bad type";
	}
}

static const char *intel_dsm_mux_type(u8 type)
{
	switch (type) {
	case 0:
		return "unknown";
	case 1:
		return "No MUX, iGPU only";
	case 2:
		return "No MUX, dGPU only";
	case 3:
		return "MUXed between iGPU and dGPU";
	default:
		return "bad type";
	}
}

static void intel_dsm_platform_mux_info(void)
{
	int i;
#ifdef __NetBSD__
	ACPI_OBJECT *pkg, *connector_count;
#else
	union acpi_object *pkg, *connector_count;
#endif

	pkg = acpi_evaluate_dsm_typed(intel_dsm_priv.dhandle, intel_dsm_guid,
			INTEL_DSM_REVISION_ID, INTEL_DSM_FN_PLATFORM_MUX_INFO,
			NULL, ACPI_TYPE_PACKAGE);
	if (!pkg) {
		DRM_DEBUG_DRIVER("failed to evaluate _DSM\n");
		return;
	}

#ifdef __NetBSD__
	connector_count = &pkg->Package.Elements[0];
	DRM_DEBUG_DRIVER("MUX info connectors: %lld\n",
		  (unsigned long long)connector_count->Integer.Value);
	for (i = 1; i < pkg->Package.Count; i++) {
		ACPI_OBJECT *obj = &pkg->Package.Elements[i];
		ACPI_OBJECT *connector_id = &obj->Package.Elements[0];
		ACPI_OBJECT *info = &obj->Package.Elements[1];
		DRM_DEBUG_DRIVER("Connector id: 0x%016llx\n",
			  (unsigned long long)connector_id->Integer.Value);
		DRM_DEBUG_DRIVER("  port id: %s\n",
		       intel_dsm_port_name(info->Buffer.Pointer[0]));
		DRM_DEBUG_DRIVER("  display mux info: %s\n",
		       intel_dsm_mux_type(info->Buffer.Pointer[1]));
		DRM_DEBUG_DRIVER("  aux/dc mux info: %s\n",
		       intel_dsm_mux_type(info->Buffer.Pointer[2]));
		DRM_DEBUG_DRIVER("  hpd mux info: %s\n",
		       intel_dsm_mux_type(info->Buffer.Pointer[3]));
	}
#else
	connector_count = &pkg->package.elements[0];
	DRM_DEBUG_DRIVER("MUX info connectors: %lld\n",
		  (unsigned long long)connector_count->integer.value);
	for (i = 1; i < pkg->package.count; i++) {
		union acpi_object *obj = &pkg->package.elements[i];
		union acpi_object *connector_id = &obj->package.elements[0];
		union acpi_object *info = &obj->package.elements[1];
		DRM_DEBUG_DRIVER("Connector id: 0x%016llx\n",
			  (unsigned long long)connector_id->integer.value);
		DRM_DEBUG_DRIVER("  port id: %s\n",
		       intel_dsm_port_name(info->buffer.pointer[0]));
		DRM_DEBUG_DRIVER("  display mux info: %s\n",
		       intel_dsm_mux_type(info->buffer.pointer[1]));
		DRM_DEBUG_DRIVER("  aux/dc mux info: %s\n",
		       intel_dsm_mux_type(info->buffer.pointer[2]));
		DRM_DEBUG_DRIVER("  hpd mux info: %s\n",
		       intel_dsm_mux_type(info->buffer.pointer[3]));
	}
#endif
	ACPI_FREE(pkg);
}

#ifdef __NetBSD__
static bool intel_dsm_pci_probe(ACPI_HANDLE dhandle)
#else
static bool intel_dsm_pci_probe(struct pci_dev *pdev)
#endif
{
#ifndef __NetBSD__
	acpi_handle dhandle;

	dhandle = ACPI_HANDLE(&pdev->dev);
#endif
	if (!dhandle)
		return false;

	if (!acpi_check_dsm(dhandle, intel_dsm_guid, INTEL_DSM_REVISION_ID,
			    1 << INTEL_DSM_FN_PLATFORM_MUX_INFO)) {
		DRM_DEBUG_KMS("no _DSM method for intel device\n");
		return false;
	}

	intel_dsm_priv.dhandle = dhandle;
	intel_dsm_platform_mux_info();

	return true;
}

#ifdef __NetBSD__

static int vga_count;
static bool has_dsm;

/* XXX from sys/dev/pci/vga_pcivar.h */
#define	DEVICE_IS_VGA_PCI(class, id)					\
	    (((PCI_CLASS(class) == PCI_CLASS_DISPLAY &&			\
	      PCI_SUBCLASS(class) == PCI_SUBCLASS_DISPLAY_VGA) ||	\
	     (PCI_CLASS(class) == PCI_CLASS_PREHISTORIC &&		\
	      PCI_SUBCLASS(class) == PCI_SUBCLASS_PREHISTORIC_VGA)) ? 1 : 0)

static int
intel_dsm_vga_match(const struct pci_attach_args *pa)
{

	if (!DEVICE_IS_VGA_PCI(pa->pa_class, pa->pa_id))
		return 0;

	vga_count++;
	struct acpi_devnode *node = acpi_pcidev_find(0 /*XXX segment*/,
	    pa->pa_bus, pa->pa_device, pa->pa_function);
	if (node != NULL)
		has_dsm |= intel_dsm_pci_probe(node->ad_handle);
	return 0;
}

static bool intel_dsm_detect(struct drm_device *dev)
{
	char acpi_method_name[255] = { 0 };

	vga_count = 0;
	has_dsm = false;
	pci_find_device(&dev->pdev->pd_pa, intel_dsm_vga_match);

	if (vga_count == 2 && has_dsm) {
		const char *name = acpi_name(intel_dsm_priv.dhandle);
		strlcpy(acpi_method_name, name, sizeof(acpi_method_name));
		DRM_DEBUG_DRIVER("VGA switcheroo: detected DSM switching method %s handle\n",
				 acpi_method_name);
		return true;
	}

	return false;
}
#else
static bool intel_dsm_detect(void)
{
	char acpi_method_name[255] = { 0 };
	struct acpi_buffer buffer = {sizeof(acpi_method_name), acpi_method_name};
	struct pci_dev *pdev = NULL;
	bool has_dsm = false;
	int vga_count = 0;

	while ((pdev = pci_get_class(PCI_CLASS_DISPLAY_VGA << 8, pdev)) != NULL) {
		vga_count++;
		has_dsm |= intel_dsm_pci_probe(pdev);
	}

	if (vga_count == 2 && has_dsm) {
		acpi_get_name(intel_dsm_priv.dhandle, ACPI_FULL_PATHNAME, &buffer);
		DRM_DEBUG_DRIVER("vga_switcheroo: detected DSM switching method %s handle\n",
				 acpi_method_name);
		return true;
	}

	return false;
}
#endif

#ifdef __NetBSD__
void intel_register_dsm_handler(struct drm_device *dev)
{
	if (!intel_dsm_detect(dev))
		return;
}
#else
void intel_register_dsm_handler(void)
{
	if (!intel_dsm_detect())
		return;
}
#endif

void intel_unregister_dsm_handler(void)
{
}

#endif	/* CONFIG_ACPI */
