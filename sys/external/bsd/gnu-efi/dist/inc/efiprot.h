/*	$NetBSD: efiprot.h,v 1.1.1.1 2014/04/01 16:16:07 jakllsch Exp $	*/

#ifndef _EFI_PROT_H
#define _EFI_PROT_H

/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    efiprot.h

Abstract:

    EFI Protocols



Revision History

--*/

//
//  FPSWA library protocol
//
#define FPSWA_PROTOCOL          \
    { 0xc41b6531, 0x97b9, 0x11d3, {0x9a, 0x29, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d} }

//
// Device Path protocol
//

#define DEVICE_PATH_PROTOCOL    \
    { 0x9576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }


//
// Block IO protocol
//

#define BLOCK_IO_PROTOCOL \
    { 0x964e5b21, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }
#define EFI_BLOCK_IO_INTERFACE_REVISION   0x00010000
#define EFI_BLOCK_IO_INTERFACE_REVISION2  0x00020001
#define EFI_BLOCK_IO_INTERFACE_REVISION3  ((2<<16) | 31)

INTERFACE_DECL(_EFI_BLOCK_IO);

typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_RESET) (
    IN struct _EFI_BLOCK_IO     *This,
    IN BOOLEAN                  ExtendedVerification
    );

typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_READ) (
    IN struct _EFI_BLOCK_IO     *This,
    IN UINT32                   MediaId,
    IN EFI_LBA                  LBA,
    IN UINTN                    BufferSize,
    OUT VOID                    *Buffer
    );


typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_WRITE) (
    IN struct _EFI_BLOCK_IO     *This,
    IN UINT32                   MediaId,
    IN EFI_LBA                  LBA,
    IN UINTN                    BufferSize,
    IN VOID                     *Buffer
    );


typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_FLUSH) (
    IN struct _EFI_BLOCK_IO     *This
    );



typedef struct {
    UINT32              MediaId;
    BOOLEAN             RemovableMedia;
    BOOLEAN             MediaPresent;

    BOOLEAN             LogicalPartition;
    BOOLEAN             ReadOnly;
    BOOLEAN             WriteCaching;

    UINT32              BlockSize;
    UINT32              IoAlign;

    EFI_LBA             LastBlock;

    /* revision 2 */
    EFI_LBA             LowestAlignedLba;
    UINT32              LogicalBlocksPerPhysicalBlock;
    /* revision 3 */
    UINT32              OptimalTransferLengthGranularity;
} EFI_BLOCK_IO_MEDIA;

typedef struct _EFI_BLOCK_IO {
    UINT64                  Revision;

    EFI_BLOCK_IO_MEDIA      *Media;

    EFI_BLOCK_RESET         Reset;
    EFI_BLOCK_READ          ReadBlocks;
    EFI_BLOCK_WRITE         WriteBlocks;
    EFI_BLOCK_FLUSH         FlushBlocks;

} EFI_BLOCK_IO;



//
// Disk Block IO protocol
//

#define DISK_IO_PROTOCOL \
    { 0xce345171, 0xba0b, 0x11d2,  {0x8e, 0x4f, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }
#define EFI_DISK_IO_INTERFACE_REVISION   0x00010000

INTERFACE_DECL(_EFI_DISK_IO);

typedef
EFI_STATUS
(EFIAPI *EFI_DISK_READ) (
    IN struct _EFI_DISK_IO      *This,
    IN UINT32                   MediaId,
    IN UINT64                   Offset,
    IN UINTN                    BufferSize,
    OUT VOID                    *Buffer
    );


typedef
EFI_STATUS
(EFIAPI *EFI_DISK_WRITE) (
    IN struct _EFI_DISK_IO      *This,
    IN UINT32                   MediaId,
    IN UINT64                   Offset,
    IN UINTN                    BufferSize,
    IN VOID                     *Buffer
    );


typedef struct _EFI_DISK_IO {
    UINT64              Revision;
    EFI_DISK_READ       ReadDisk;
    EFI_DISK_WRITE      WriteDisk;
} EFI_DISK_IO;


//
// Simple file system protocol
//

#define SIMPLE_FILE_SYSTEM_PROTOCOL \
    { 0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

INTERFACE_DECL(_EFI_FILE_IO_INTERFACE);
INTERFACE_DECL(_EFI_FILE_HANDLE);

typedef
EFI_STATUS
(EFIAPI *EFI_VOLUME_OPEN) (
    IN struct _EFI_FILE_IO_INTERFACE    *This,
    OUT struct _EFI_FILE_HANDLE         **Root
    );

#define EFI_FILE_IO_INTERFACE_REVISION   0x00010000

typedef struct _EFI_FILE_IO_INTERFACE {
    UINT64                  Revision;
    EFI_VOLUME_OPEN         OpenVolume;
} EFI_FILE_IO_INTERFACE;

//
//
//

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_OPEN) (
    IN struct _EFI_FILE_HANDLE  *File,
    OUT struct _EFI_FILE_HANDLE **NewHandle,
    IN CHAR16                   *FileName,
    IN UINT64                   OpenMode,
    IN UINT64                   Attributes
    );

// Open modes
#define EFI_FILE_MODE_READ      0x0000000000000001
#define EFI_FILE_MODE_WRITE     0x0000000000000002
#define EFI_FILE_MODE_CREATE    0x8000000000000000

// File attributes
#define EFI_FILE_READ_ONLY      0x0000000000000001
#define EFI_FILE_HIDDEN         0x0000000000000002
#define EFI_FILE_SYSTEM         0x0000000000000004
#define EFI_FILE_RESERVIED      0x0000000000000008
#define EFI_FILE_DIRECTORY      0x0000000000000010
#define EFI_FILE_ARCHIVE        0x0000000000000020
#define EFI_FILE_VALID_ATTR     0x0000000000000037

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_CLOSE) (
    IN struct _EFI_FILE_HANDLE  *File
    );

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_DELETE) (
    IN struct _EFI_FILE_HANDLE  *File
    );

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_READ) (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    OUT VOID                    *Buffer
    );

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_WRITE) (
    IN struct _EFI_FILE_HANDLE  *File,
    IN OUT UINTN                *BufferSize,
    IN VOID                     *Buffer
    );

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_POSITION) (
    IN struct _EFI_FILE_HANDLE  *File,
    IN UINT64                   Position
    );

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_POSITION) (
    IN struct _EFI_FILE_HANDLE  *File,
    OUT UINT64                  *Position
    );

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_INFO) (
    IN struct _EFI_FILE_HANDLE  *File,
    IN EFI_GUID                 *InformationType,
    IN OUT UINTN                *BufferSize,
    OUT VOID                    *Buffer
    );

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_INFO) (
    IN struct _EFI_FILE_HANDLE  *File,
    IN EFI_GUID                 *InformationType,
    IN UINTN                    BufferSize,
    IN VOID                     *Buffer
    );

typedef
EFI_STATUS
(EFIAPI *EFI_FILE_FLUSH) (
    IN struct _EFI_FILE_HANDLE  *File
    );



#define EFI_FILE_HANDLE_REVISION         0x00010000
typedef struct _EFI_FILE_HANDLE {
    UINT64                  Revision;
    EFI_FILE_OPEN           Open;
    EFI_FILE_CLOSE          Close;
    EFI_FILE_DELETE         Delete;
    EFI_FILE_READ           Read;
    EFI_FILE_WRITE          Write;
    EFI_FILE_GET_POSITION   GetPosition;
    EFI_FILE_SET_POSITION   SetPosition;
    EFI_FILE_GET_INFO       GetInfo;
    EFI_FILE_SET_INFO       SetInfo;
    EFI_FILE_FLUSH          Flush;
} EFI_FILE, *EFI_FILE_HANDLE;


//
// File information types
//

#define EFI_FILE_INFO_ID   \
    { 0x9576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

typedef struct {
    UINT64                  Size;
    UINT64                  FileSize;
    UINT64                  PhysicalSize;
    EFI_TIME                CreateTime;
    EFI_TIME                LastAccessTime;
    EFI_TIME                ModificationTime;
    UINT64                  Attribute;
    CHAR16                  FileName[1];
} EFI_FILE_INFO;

//
// The FileName field of the EFI_FILE_INFO data structure is variable length.
// Whenever code needs to know the size of the EFI_FILE_INFO data structure, it needs to
// be the size of the data structure without the FileName field.  The following macro 
// computes this size correctly no matter how big the FileName array is declared.
// This is required to make the EFI_FILE_INFO data structure ANSI compilant. 
//

#define SIZE_OF_EFI_FILE_INFO EFI_FIELD_OFFSET(EFI_FILE_INFO,FileName)

#define EFI_FILE_SYSTEM_INFO_ID    \
    { 0x9576e93, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

typedef struct {
    UINT64                  Size;
    BOOLEAN                 ReadOnly;
    UINT64                  VolumeSize;
    UINT64                  FreeSpace;
    UINT32                  BlockSize;
    CHAR16                  VolumeLabel[1];
} EFI_FILE_SYSTEM_INFO;

//
// The VolumeLabel field of the EFI_FILE_SYSTEM_INFO data structure is variable length.
// Whenever code needs to know the size of the EFI_FILE_SYSTEM_INFO data structure, it needs
// to be the size of the data structure without the VolumeLable field.  The following macro 
// computes this size correctly no matter how big the VolumeLable array is declared.
// This is required to make the EFI_FILE_SYSTEM_INFO data structure ANSI compilant. 
//

#define SIZE_OF_EFI_FILE_SYSTEM_INFO EFI_FIELD_OFFSET(EFI_FILE_SYSTEM_INFO,VolumeLabel)

#define EFI_FILE_SYSTEM_VOLUME_LABEL_INFO_ID    \
    { 0xDB47D7D3,0xFE81, 0x11d3, {0x9A, 0x35, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D} }

typedef struct {
    CHAR16                  VolumeLabel[1];
} EFI_FILE_SYSTEM_VOLUME_LABEL_INFO;

#define SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL_INFO EFI_FIELD_OFFSET(EFI_FILE_SYSTEM_VOLUME_LABEL_INFO,VolumeLabel)

//
// Load file protocol
//


#define LOAD_FILE_PROTOCOL \
    { 0x56EC3091, 0x954C, 0x11d2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B} }

INTERFACE_DECL(_EFI_LOAD_FILE_INTERFACE);

typedef
EFI_STATUS
(EFIAPI *EFI_LOAD_FILE) (
    IN struct _EFI_LOAD_FILE_INTERFACE  *This,
    IN EFI_DEVICE_PATH                  *FilePath,
    IN BOOLEAN                          BootPolicy,
    IN OUT UINTN                        *BufferSize,
    IN VOID                             *Buffer OPTIONAL
    );

typedef struct _EFI_LOAD_FILE_INTERFACE {
    EFI_LOAD_FILE                       LoadFile;
} EFI_LOAD_FILE_INTERFACE;


//
// Device IO protocol
//

#define DEVICE_IO_PROTOCOL \
    { 0xaf6ac311, 0x84c3, 0x11d2, {0x8e, 0x3c, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

INTERFACE_DECL(_EFI_DEVICE_IO_INTERFACE);

typedef enum {
    IO_UINT8,
    IO_UINT16,
    IO_UINT32,
    IO_UINT64,
//
// Specification Change: Copy from MMIO to MMIO vs. MMIO to buffer, buffer to MMIO
//
    MMIO_COPY_UINT8,
    MMIO_COPY_UINT16,
    MMIO_COPY_UINT32,
    MMIO_COPY_UINT64
} EFI_IO_WIDTH;

#define EFI_PCI_ADDRESS(_bus,_dev,_func) \
    ( (UINT64) ( (((UINTN)_bus) << 24) + (((UINTN)_dev) << 16) + (((UINTN)_func) << 8) ) )


typedef
EFI_STATUS
(EFIAPI *EFI_DEVICE_IO) (
    IN struct _EFI_DEVICE_IO_INTERFACE *This,
    IN EFI_IO_WIDTH                 Width,
    IN UINT64                       Address,
    IN UINTN                        Count,
    IN OUT VOID                     *Buffer
    );

typedef struct {
    EFI_DEVICE_IO                   Read;
    EFI_DEVICE_IO                   Write;
} EFI_IO_ACCESS;

typedef 
EFI_STATUS
(EFIAPI *EFI_PCI_DEVICE_PATH) (
    IN struct _EFI_DEVICE_IO_INTERFACE  *This,
    IN UINT64                           Address,
    IN OUT EFI_DEVICE_PATH              **PciDevicePath
    );

typedef enum {
    EfiBusMasterRead,
    EfiBusMasterWrite,
    EfiBusMasterCommonBuffer
} EFI_IO_OPERATION_TYPE;

typedef
EFI_STATUS
(EFIAPI *EFI_IO_MAP) (
    IN struct _EFI_DEVICE_IO_INTERFACE  *This,
    IN EFI_IO_OPERATION_TYPE            Operation,
    IN EFI_PHYSICAL_ADDRESS             *HostAddress,
    IN OUT UINTN                        *NumberOfBytes,
    OUT EFI_PHYSICAL_ADDRESS            *DeviceAddress,
    OUT VOID                            **Mapping
    );

typedef
EFI_STATUS
(EFIAPI *EFI_IO_UNMAP) (
    IN struct _EFI_DEVICE_IO_INTERFACE  *This,
    IN VOID                             *Mapping
    );

typedef
EFI_STATUS
(EFIAPI *EFI_IO_ALLOCATE_BUFFER) (
    IN struct _EFI_DEVICE_IO_INTERFACE  *This,
    IN EFI_ALLOCATE_TYPE                Type,
    IN EFI_MEMORY_TYPE                  MemoryType,
    IN UINTN                            Pages,
    IN OUT EFI_PHYSICAL_ADDRESS         *HostAddress
    );

typedef
EFI_STATUS
(EFIAPI *EFI_IO_FLUSH) (
    IN struct _EFI_DEVICE_IO_INTERFACE  *This
    );

typedef
EFI_STATUS
(EFIAPI *EFI_IO_FREE_BUFFER) (
    IN struct _EFI_DEVICE_IO_INTERFACE  *This,
    IN UINTN                            Pages,
    IN EFI_PHYSICAL_ADDRESS             HostAddress
    );

typedef struct _EFI_DEVICE_IO_INTERFACE {
    EFI_IO_ACCESS                       Mem;
    EFI_IO_ACCESS                       Io;
    EFI_IO_ACCESS                       Pci;
    EFI_IO_MAP                          Map;
    EFI_PCI_DEVICE_PATH                 PciDevicePath;
    EFI_IO_UNMAP                        Unmap;
    EFI_IO_ALLOCATE_BUFFER              AllocateBuffer;
    EFI_IO_FLUSH                        Flush;
    EFI_IO_FREE_BUFFER                  FreeBuffer;
} EFI_DEVICE_IO_INTERFACE;


//
// Unicode Collation protocol
//

#define UNICODE_COLLATION_PROTOCOL \
    { 0x1d85cd7f, 0xf43d, 0x11d2, {0x9a, 0xc,  0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d} }

#define UNICODE_BYTE_ORDER_MARK       (CHAR16)(0xfeff)

INTERFACE_DECL(_EFI_UNICODE_COLLATION_INTERFACE);

typedef
INTN
(EFIAPI *EFI_UNICODE_STRICOLL) (
    IN struct _EFI_UNICODE_COLLATION_INTERFACE  *This,
    IN CHAR16                         *s1,
    IN CHAR16                         *s2
    );

typedef
BOOLEAN
(EFIAPI *EFI_UNICODE_METAIMATCH) (
    IN struct _EFI_UNICODE_COLLATION_INTERFACE  *This,
    IN CHAR16                         *String,
    IN CHAR16                         *Pattern
    );

typedef
VOID
(EFIAPI *EFI_UNICODE_STRLWR) (
    IN struct _EFI_UNICODE_COLLATION_INTERFACE  *This,
    IN OUT CHAR16                       *Str
    );

typedef
VOID
(EFIAPI *EFI_UNICODE_STRUPR) (
    IN struct _EFI_UNICODE_COLLATION_INTERFACE  *This,
    IN OUT CHAR16                       *Str
    );

typedef
VOID
(EFIAPI *EFI_UNICODE_FATTOSTR) (
    IN struct _EFI_UNICODE_COLLATION_INTERFACE  *This,
    IN UINTN                            FatSize,
    IN CHAR8                            *Fat,
    OUT CHAR16                          *String
    );

typedef
BOOLEAN
(EFIAPI *EFI_UNICODE_STRTOFAT) (
    IN struct _EFI_UNICODE_COLLATION_INTERFACE  *This,
    IN CHAR16                           *String,
    IN UINTN                            FatSize,
    OUT CHAR8                           *Fat
    );


typedef struct _EFI_UNICODE_COLLATION_INTERFACE {

    // general
    EFI_UNICODE_STRICOLL                StriColl;
    EFI_UNICODE_METAIMATCH              MetaiMatch;
    EFI_UNICODE_STRLWR                  StrLwr;
    EFI_UNICODE_STRUPR                  StrUpr;

    // for supporting fat volumes
    EFI_UNICODE_FATTOSTR                FatToStr;
    EFI_UNICODE_STRTOFAT                StrToFat;

    CHAR8                               *SupportedLanguages;
} EFI_UNICODE_COLLATION_INTERFACE;

/* Graphics output protocol */
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
  { \
    0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } \
  }

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
  UINT32            RedMask;
  UINT32            GreenMask;
  UINT32            BlueMask;
  UINT32            ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
  PixelRedGreenBlueReserved8BitPerColor,
  PixelBlueGreenRedReserved8BitPerColor,
  PixelBitMask,
  PixelBltOnly,
  PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
  UINT32                     Version;
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat;
  EFI_PIXEL_BITMASK          PixelInformation;
  UINT32                     PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

/**
  Return the current video mode information.

  @param  This       Protocol instance pointer.
  @param  ModeNumber The mode number to return information on.
  @param  SizeOfInfo A pointer to the size, in bytes, of the Info buffer.
  @param  Info       A pointer to callee allocated buffer that returns information about ModeNumber.

  @retval EFI_SUCCESS           Mode information returned.
  @retval EFI_BUFFER_TOO_SMALL  The Info buffer was too small.
  @retval EFI_DEVICE_ERROR      A hardware error occurred trying to retrieve the video mode.
  @retval EFI_NOT_STARTED       Video display is not initialized. Call SetMode ()
  @retval EFI_INVALID_PARAMETER One of the input args was NULL.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE) (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
;

/**
  Return the current video mode information.

  @param  This              Protocol instance pointer.
  @param  ModeNumber        The mode number to be set.

  @retval EFI_SUCCESS       Graphics mode was changed.
  @retval EFI_DEVICE_ERROR  The device had an error and could not complete the request.
  @retval EFI_UNSUPPORTED   ModeNumber is not supported by this device.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE) (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
  IN  UINT32                       ModeNumber
  );

typedef struct {
  UINT8 Blue;
  UINT8 Green;
  UINT8 Red;
  UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef union {
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Pixel;
  UINT32                        Raw;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION;

typedef enum {
  EfiBltVideoFill,
  EfiBltVideoToBltBuffer,
  EfiBltBufferToVideo, 
  EfiBltVideoToVideo,
  EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

/**
  The following table defines actions for BltOperations:

  <B>EfiBltVideoFill</B> - Write data from the  BltBuffer pixel (SourceX, SourceY) 
  directly to every pixel of the video display rectangle 
  (DestinationX, DestinationY) (DestinationX + Width, DestinationY + Height). 
  Only one pixel will be used from the BltBuffer. Delta is NOT used.

  <B>EfiBltVideoToBltBuffer</B> - Read data from the video display rectangle 
  (SourceX, SourceY) (SourceX + Width, SourceY + Height) and place it in 
  the BltBuffer rectangle (DestinationX, DestinationY ) 
  (DestinationX + Width, DestinationY + Height). If DestinationX or 
  DestinationY is not zero then Delta must be set to the length in bytes 
  of a row in the BltBuffer.

  <B>EfiBltBufferToVideo</B> - Write data from the  BltBuffer rectangle 
  (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the 
  video display rectangle (DestinationX, DestinationY) 
  (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is 
  not zero then Delta must be set to the length in bytes of a row in the 
  BltBuffer.

  <B>EfiBltVideoToVideo</B> - Copy from the video display rectangle (SourceX, SourceY)
  (SourceX + Width, SourceY + Height) .to the video display rectangle 
  (DestinationX, DestinationY) (DestinationX + Width, DestinationY + Height). 
  The BltBuffer and Delta  are not used in this mode.

  @param  This         Protocol instance pointer.
  @param  BltBuffer    Buffer containing data to blit into video buffer. This
                       buffer has a size of Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
  @param  BltOperation Operation to perform on BlitBuffer and video memory
  @param  SourceX      X coordinate of source for the BltBuffer.
  @param  SourceY      Y coordinate of source for the BltBuffer.
  @param  DestinationX X coordinate of destination for the BltBuffer.
  @param  DestinationY Y coordinate of destination for the BltBuffer.
  @param  Width        Width of rectangle in BltBuffer in pixels.
  @param  Height       Hight of rectangle in BltBuffer in pixels.
  @param  Delta        OPTIONAL

  @retval EFI_SUCCESS           The Blt operation completed.
  @retval EFI_INVALID_PARAMETER BltOperation is not valid.
  @retval EFI_DEVICE_ERROR      A hardware error occured writting to the video buffer.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT) (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL            *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           *BltBuffer,   OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta         OPTIONAL
  );

typedef struct {
  UINT32                                 MaxMode;
  UINT32                                 Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION   *Info;
  UINTN                                  SizeOfInfo;
  EFI_PHYSICAL_ADDRESS                   FrameBufferBase;
  UINTN                                  FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
  EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE  QueryMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE    SetMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT         Blt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE        *Mode;
};

INTERFACE_DECL(_EFI_SERVICE_BINDING);

typedef
EFI_STATUS
(EFIAPI *EFI_SERVICE_BINDING_CREATE_CHILD) (
    IN struct _EFI_SERVICE_BINDING *This,
    IN EFI_HANDLE                  *ChildHandle
    );

typedef
EFI_STATUS
(EFIAPI *EFI_SERVICE_BINDING_DESTROY_CHILD) (
    IN struct _EFI_SERVICE_BINDING *This,
    IN EFI_HANDLE                  ChildHandle
    );

typedef struct _EFI_SERVICE_BINDING {
    EFI_SERVICE_BINDING_CREATE_CHILD  CreateChild;
    EFI_SERVICE_BINDING_DESTROY_CHILD DestroyChild;
} EFI_SERVICE_BINDING;

#endif

