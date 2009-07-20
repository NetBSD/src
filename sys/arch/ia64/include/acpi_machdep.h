/*	$NetBSD: acpi_machdep.h,v 1.2 2009/07/20 04:41:37 kiyohara Exp $	*/

ACPI_STATUS acpi_md_OsInitialize(void);
ACPI_STATUS acpi_md_OsTerminate(void);
ACPI_PHYSICAL_ADDRESS acpi_md_OsGetRootPointer(void); 

#define acpi_md_OsIn8(x)	inb((x))
#define acpi_md_OsIn16(x)	inw((x))
#define acpi_md_OsIn32(x)	inl((x))

#define acpi_md_OsOut8(x, v)	outb((x), (v))
#define acpi_md_OsOut16(x, v)	outw((x), (v))
#define acpi_md_OsOut32(x, v)	outl((x), (v))

ACPI_STATUS acpi_md_OsInstallInterruptHandler(UINT32, ACPI_OSD_HANDLER,
					      void *, void **); 
void acpi_md_OsRemoveInterruptHandler(void *);

ACPI_STATUS acpi_md_OsMapMemory(ACPI_PHYSICAL_ADDRESS, UINT32, void **);
void acpi_md_OsUnmapMemory(void *, UINT32);
ACPI_STATUS acpi_md_OsGetPhysicalAddress(void *LogicalAddress,
					 ACPI_PHYSICAL_ADDRESS *PhysicalAddress);

BOOLEAN acpi_md_OsReadable(void *, UINT32);
BOOLEAN acpi_md_OsWritable(void *, UINT32);
void acpi_md_OsEnableInterrupt(void); 
void acpi_md_OsDisableInterrupt(void);

int acpi_md_sleep(int);
void acpi_md_callback(void);
