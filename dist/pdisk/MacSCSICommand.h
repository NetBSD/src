/*
    File:       MacSCSICommand.h

    Contains:   SCSI specific definitions.

    Written by: Martin Minow

*/

/*
 * Copyright 1995, 1997 by Apple Computer, Inc.
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */

/*
 * Scsi-specific definitions.
 */
#ifndef __MacSCSICommand__
#define __MacSCSICommand__

/*
 * The 6-byte commands are used for most simple
 * I/O requests.
 */
struct SCSI_6_Byte_Command {                /* Six-byte command         */
    uint8_t       opcode;             /*  0                       */
    uint8_t       lbn3;               /*  1 lbn in low 5          */
    uint8_t       lbn2;               /*  2                       */
    uint8_t       lbn1;               /*  3                       */
    uint8_t       len;                /*  4                       */
    uint8_t       ctrl;               /*  5                       */
};
typedef struct SCSI_6_Byte_Command SCSI_6_Byte_Command;

struct SCSI_10_Byte_Command {               /* Ten-byte command         */
    uint8_t       opcode;             /*  0                       */
    uint8_t       lun;                /*  1                       */
    uint8_t       lbn4;               /*  2                       */
    uint8_t       lbn3;               /*  3                       */
    uint8_t       lbn2;               /*  4                       */
    uint8_t       lbn1;               /*  5                       */
    uint8_t       pad;                /*  6                       */
    uint8_t       len2;               /*  7                       */
    uint8_t       len1;               /*  8                       */
    uint8_t       ctrl;               /*  9                       */
};
typedef struct SCSI_10_Byte_Command SCSI_10_Byte_Command;

struct SCSI_12_Byte_Command {               /* Twelve-byte command      */
    uint8_t       opcode;             /*  0                       */
    uint8_t       lun;                /*  1                       */
    uint8_t       lbn4;               /*  2                       */
    uint8_t       lbn3;               /*  3                       */
    uint8_t       lbn2;               /*  4                       */
    uint8_t       lbn1;               /*  5                       */
    uint8_t       len4;               /*  6                       */
    uint8_t       len3;               /*  7                       */
    uint8_t       len2;               /*  8                       */
    uint8_t       len1;               /*  9                       */
    uint8_t       pad;                /* 10                       */
    uint8_t       ctrl;               /* 11                       */
};
typedef struct SCSI_12_Byte_Command SCSI_12_Byte_Command;

/*
 * This union defines all scsi commands.
 */
union SCSI_Command {
    SCSI_6_Byte_Command     scsi6;
    SCSI_10_Byte_Command    scsi10;
    SCSI_12_Byte_Command    scsi12;
    uint8_t           scsi[12];
};
typedef union SCSI_Command SCSI_Command, *SCSI_CommandPtr;

/*
 * Returned by a read-capacity command.
 */
struct SCSI_Capacity_Data {
    uint8_t       lbn4;               /* Number                   */
    uint8_t       lbn3;               /*  of                      */
    uint8_t       lbn2;               /*   logical                */
    uint8_t       lbn1;               /*    blocks                */
    uint8_t       len4;               /* Length                   */
    uint8_t       len3;               /*  of each                 */
    uint8_t       len2;               /*   logical block          */
    uint8_t       len1;               /*    in bytes              */
};
typedef struct SCSI_Capacity_Data SCSI_Capacity_Data;

struct SCSI_Inquiry_Data {                  /* Inquiry returns this     */
    uint8_t       devType;            /*  0 Device type,          */
    uint8_t       devTypeMod;         /*  1 Device type modifier  */
    uint8_t       version;            /*  2 ISO/ECMA/ANSI version */
    uint8_t       format;             /*  3 Response data format  */
    uint8_t       length;             /*  4 Additional Length     */
    uint8_t       reserved5;          /*  5 Reserved              */
    uint8_t       reserved6;          /*  6 Reserved              */
    uint8_t       flags;              /*  7 Capability flags      */
    uint8_t       vendor[8];          /*  8-15 Vendor-specific    */
    uint8_t       product[16];        /* 16-31 Product id         */
    uint8_t       revision[4];        /* 32-35 Product revision   */
    uint8_t       vendorSpecific[20]; /* 36-55 Vendor stuff       */
    uint8_t       moreReserved[40];   /* 56-95 Reserved           */
};
typedef struct SCSI_Inquiry_Data SCSI_Inquiry_Data;

/*
 * This bit may be set in SCSI_Inquiry_Data.devTypeMod
 */
enum {
    kScsiInquiryRMB = 0x80                  /* Removable medium if set  */
};
/*
 * These bits may be set in SCSI_Inquiry_Data.flags
 */
enum {
    kScsiInquiryRelAdr  = 0x80,             /* Has relative addressing  */
    kScsiInquiryWBus32  = 0x40,             /* Wide (32-bit) transfers  */
    kScsiInquiryWBus16  = 0x20,             /* Wide (16-bit) transfers  */
    kScsiInquirySync    = 0x10,             /* Synchronous transfers    */
    kScsiInquiryLinked  = 0x08,             /* Linked commands ok       */
    kScsiInquiryReserved = 0x04,
    kScsiInquiryCmdQue  = 0x02,             /* Tagged cmd queuing ok    */
    kScsiInquirySftRe   = 0x01              /* Soft reset alternative   */
};

/*
 * These bits may be set in SCSI_Inquiry_Data.devType
 */
enum {
    kScsiDevTypeDirect                  = 0,
    kScsiDevTypeSequential,
    kScsiDevTypePrinter,
    kScsiDevTypeProcessor,
    kScsiDevTypeWorm,                       /* Write-once, read mult    */
    kScsiDevTypeCDROM,
    kScsiDevTypeScanner,
    kScsiDevTypeOptical,
    kScsiDevTypeChanger,
    kScsiDevTypeComm,
    kScsiDevTypeGraphicArts0A,
    kScsiDevTypeGraphicArts0B,
    kScsiDevTypeFirstReserved,              /* Reserved sequence start  */
    kScsiDevTypeUnknownOrMissing        = 0x1F,
    kScsiDevTypeMask                    = 0x1F
};
/*
 * These are device type qualifiers. We need them to distinguish between "unknown"
 * and "missing" devices.
 */
enum {
    kScsiDevTypeQualifierConnected      = 0x00, /* Exists and is connected      */
    kScsiDevTypeQualifierNotConnected   = 0x20, /* Logical unit exists          */
    kScsiDevTypeQualifierReserved       = 0x40,
    kScsiDevTypeQualifierMissing        = 0x60, /* No such logical unit         */
    kScsiDevTypeQualifierVendorSpecific = 0x80, /* Other bits are unspecified   */
    kScsiDevTypeQualifierMask           = 0xE0
};
#define kScsiDevTypeMissing \
    (kScsiDevTypeUnknownOrMissing | kScsiDevTypeQualifierMissing)

/*
 * This is the data that is returned after a GetExtendedStatus
 * request. The errorCode gives a general indication of the error,
 * which may be qualified by the additionalSenseCode and
 * additionalSenseQualifier fields. These may be device (vendor)
 * specific values, however. The info[] field contains additional
 * information. For a media error, it contains the failing
 * logical block number (most-significant byte first).
 */
struct SCSI_Sense_Data {                /* Request Sense result         */
    uint8_t       errorCode;      /*  0   Class code, valid lbn   */
    uint8_t       segmentNumber;  /*  1   Segment number          */
    uint8_t       senseKey;       /*  2   Sense key and flags     */
    uint8_t       info[4];
    uint8_t       additionalSenseLength;
    uint8_t       reservedForCopy[4];
    uint8_t       additionalSenseCode;
    uint8_t       additionalSenseQualifier;   
    uint8_t       fruCode;        /* Field replacable unit code   */
    uint8_t       senseKeySpecific[2];
    uint8_t       additional[101];
};
typedef struct SCSI_Sense_Data SCSI_Sense_Data;
/*
 * The high-bit of errorCode signals whether there is a logical
 * block. The low value signals whether there is a valid sense
 */
#define kScsiSenseHasLBN            0x80    /* Logical block number set */
#define kScsiSenseInfoValid         0x70    /* Is sense key valid?      */
#define kScsiSenseInfoMask          0x70    /* Mask for sense info      */
/*
 * These bits may be set in the sense key
 */
#define kScsiSenseKeyMask           0x0F
#define kScsiSenseILI               0x20    /* Illegal logical Length   */
#define kScsiSenseEOM               0x40    /* End of media             */
#define kScsiSenseFileMark          0x80    /* End of file mark         */

/*
 * SCSI sense codes. (Returned after request sense).
 */
#define  kScsiSenseNone             0x00    /* No error                 */
#define  kScsiSenseRecoveredErr     0x01    /* Warning                  */
#define  kScsiSenseNotReady         0x02    /* Device not ready         */
#define  kScsiSenseMediumErr        0x03    /* Device medium error      */
#define  kScsiSenseHardwareErr      0x04    /* Device hardware error    */
#define  kScsiSenseIllegalReq       0x05    /* Illegal request for dev. */
#define  kScsiSenseUnitAtn          0x06    /* Unit attention (not err) */
#define  kScsiSenseDataProtect      0x07    /* Data protection          */
#define  kScsiSenseBlankCheck       0x08    /* Tape-specific error      */
#define  kScsiSenseVendorSpecific   0x09    /* Vendor-specific error    */
#define  kScsiSenseCopyAborted      0x0a    /* Copy request cancelled   */
#define  kScsiSenseAbortedCmd       0x0b    /* Initiator aborted cmd.   */
#define  kScsiSenseEqual            0x0c    /* Comparison equal         */
#define  kScsiSenseVolumeOverflow   0x0d    /* Write past end mark      */
#define  kScsiSenseMiscompare       0x0e    /* Comparison failed        */
#define  kScsiSenseCurrentErr       0x70
#define  kScsiSenseDeferredErr      0x71

/*
 * Mode sense parameter header
 */
struct SCSI_ModeParamHeader {
    uint8_t       modeDataLength;
    uint8_t       mediumType;
    uint8_t       deviceSpecific;
    uint8_t       blockDescriptorLength;
};
typedef struct SCSI_ModeParamHeader SCSI_ModeParamHeader;

struct SCSI_ModeParamBlockDescriptor {
    uint8_t       densityCode;
    uint8_t       numberOfBlocks[3];
    uint8_t       reserved;
    uint8_t       blockLength[3];
};
typedef struct SCSI_ModeParamBlockDescriptor SCSI_ModeParamBlockDescriptor;

union SCSI_ModeParamPage {
    uint8_t       data[1];
    struct {
	uint8_t   code;
	uint8_t   length;
    } page;
};
typedef union SCSI_ModeParamPage SCSI_ModeParamPage;

/*
 * LogSense parameter header
 */
struct SCSI_LogSenseParamHeader {
    uint8_t       pageCode;
    uint8_t       reserved;
    uint8_t       pageLength[2];
};
typedef struct SCSI_LogSenseParamHeader SCSI_LogSenseParamHeader;

/*
 * Log parameter pages are variable-length with a fixed length header.
 */
union SCSI_LogSenseParamPage {
    uint8_t       data[1];
    struct {
	uint8_t   parameterCode[2];
	uint8_t   flags;
	uint8_t   parameterLength;
    } page;
};
typedef union SCSI_LogSenseParamPage SCSI_LogSenseParamPage;

/*
 * SCSI command status (from status phase)
 */
#define  kScsiStatusGood            0x00    /* Normal completion        */
#define  kScsiStatusCheckCondition  0x02    /* Need GetExtendedStatus   */
#define  kScsiStatusConditionMet    0x04
#define  kScsiStatusBusy            0x08    /* Device busy (self-test?) */
#define  kScsiStatusIntermediate    0x10    /* Intermediate status      */
#define  kScsiStatusResConflict     0x18    /* Reservation conflict     */
#define  kScsiStatusQueueFull       0x28    /* Target can't do command  */
#define  kScsiStatusReservedMask    0x3e    /* Vendor specific?         */

/*
 * SCSI command codes. Commands defined as ...6, ...10, ...12, are
 * six-byte, ten-byte, and twelve-byte variants of the indicated command.
 */
/*
 * These commands are supported for all devices.
 */
#define kScsiCmdChangeDefinition    0x40
#define kScsiCmdCompare             0x39
#define kScsiCmdCopy                0x18
#define kScsiCmdCopyAndVerify       0x3a
#define kScsiCmdInquiry             0x12
#define kScsiCmdLogSelect           0x4c
#define kScsiCmdLogSense            0x4d
#define kScsiCmdModeSelect10        0x55
#define kScsiCmdModeSelect6         0x15
#define kScsiCmdModeSense10         0x5a
#define kScsiCmdModeSense6          0x1a
#define kScsiCmdReadBuffer          0x3c
#define kScsiCmdRecvDiagResult      0x1c
#define kScsiCmdRequestSense        0x03
#define kScsiCmdSendDiagnostic      0x1d
#define kScsiCmdTestUnitReady       0x00
#define kScsiCmdWriteBuffer         0x3b

/*
 * These commands are supported by direct-access devices only.
 */
#define kScsiCmdFormatUnit          0x04
#define kSCSICmdCopy                0x18
#define kSCSICmdCopyAndVerify       0x3a
#define kScsiCmdLockUnlockCache     0x36
#define kScsiCmdPrefetch            0x34
#define kScsiCmdPreventAllowRemoval 0x1e
#define kScsiCmdRead6               0x08
#define kScsiCmdRead10              0x28
#define kScsiCmdReadCapacity        0x25
#define kScsiCmdReadDefectData      0x37
#define kScsiCmdReadLong            0x3e
#define kScsiCmdReassignBlocks      0x07
#define kScsiCmdRelease             0x17
#define kScsiCmdReserve             0x16
#define kScsiCmdRezeroUnit          0x01
#define kScsiCmdSearchDataEql       0x31
#define kScsiCmdSearchDataHigh      0x30
#define kScsiCmdSearchDataLow       0x32
#define kScsiCmdSeek6               0x0b
#define kScsiCmdSeek10              0x2b
#define kScsiCmdSetLimits           0x33
#define kScsiCmdStartStopUnit       0x1b
#define kScsiCmdSynchronizeCache    0x35
#define kScsiCmdVerify              0x2f
#define kScsiCmdWrite6              0x0a
#define kScsiCmdWrite10             0x2a
#define kScsiCmdWriteAndVerify      0x2e
#define kScsiCmdWriteLong           0x3f
#define kScsiCmdWriteSame           0x41

/*
 * These commands are supported by sequential devices.
 */
#define kScsiCmdRewind              0x01
#define kScsiCmdWriteFilemarks      0x10
#define kScsiCmdSpace               0x11
#define kScsiCmdLoadUnload          0x1B
/*
 * ANSI SCSI-II for CD-ROM devices.
 */
#define kScsiCmdReadCDTableOfContents   0x43

/*
 * Message codes (for Msg In and Msg Out phases).
 */
#define kScsiMsgAbort               0x06
#define kScsiMsgAbortTag            0x0d
#define kScsiMsgBusDeviceReset      0x0c
#define kScsiMsgClearQueue          0x0e
#define kScsiMsgCmdComplete         0x00
#define kScsiMsgDisconnect          0x04
#define kScsiMsgIdentify            0x80
#define kScsiMsgIgnoreWideResdue    0x23
#define kScsiMsgInitiateRecovery    0x0f
#define kScsiMsgInitiatorDetectedErr 0x05
#define kScsiMsgLinkedCmdComplete   0x0a
#define kScsiMsgLinkedCmdCompleteFlag 0x0b
#define kScsiMsgParityErr           0x09
#define kScsiMsgRejectMsg           0x07
#define kScsiMsgModifyDataPtr       0x00 /* Extended msg        */
#define kScsiMsgNop                 0x08
#define kScsiMsgHeadOfQueueTag      0x21 /* Two byte msg        */
#define kScsiMsgOrderedQueueTag     0x22 /* Two byte msg        */
#define kScsiMsgSimpleQueueTag      0x20 /* Two byte msg        */
#define kScsiMsgReleaseRecovery     0x10
#define kScsiMsgRestorePointers     0x03
#define kScsiMsgSaveDataPointers    0x02
#define kScsiMsgSyncXferReq         0x01 /* Extended msg        */
#define kScsiMsgWideDataXferReq     0x03 /* Extended msg        */
#define kScsiMsgTerminateIOP        0x11
#define kScsiMsgExtended            0x01
#define kScsiMsgEnableDisconnectMask 0x40

#define kScsiMsgTwoByte             0x20
#define kScsiMsgTwoByteMin          0x20
#define kScsiMsgTwoByteMax          0x2f

/*
 * Default timeout times for SCSI commands (times are in Msec).
 */
#define kScsiNormalCompletionTime   (500L)          /* 1/2 second               */
/*
 * Dratted DAT tape.
 */
#define kScsiDATCompletionTime      (60L * 1000L);  /* One minute               */
/*
 * Yes, we do allow 90 seconds for spin-up of those dratted tape drives.
 */
#define kScsiSpinUpCompletionTime   (90L * 1000L)


#endif /* __MacSCSICommand__ */
