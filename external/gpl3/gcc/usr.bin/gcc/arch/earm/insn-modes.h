/* Generated automatically from machmode.def and config/arm/arm-modes.def
   by genmodes.  */

#ifndef GCC_INSN_MODES_H
#define GCC_INSN_MODES_H

enum machine_mode
{
  VOIDmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:169 */
  BLKmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:173 */
  CCmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:201 */
  CC_NOOVmode,             /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:40 */
  CC_Zmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:41 */
  CC_SWPmode,              /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:42 */
  CCFPmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:43 */
  CCFPEmode,               /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:44 */
  CC_DNEmode,              /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:45 */
  CC_DEQmode,              /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:46 */
  CC_DLEmode,              /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:47 */
  CC_DLTmode,              /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:48 */
  CC_DGEmode,              /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:49 */
  CC_DGTmode,              /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:50 */
  CC_DLEUmode,             /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:51 */
  CC_DLTUmode,             /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:52 */
  CC_DGEUmode,             /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:53 */
  CC_DGTUmode,             /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:54 */
  CC_Cmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:55 */
  CC_Nmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:56 */
  BImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:176 */
  QImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:181 */
  HImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:182 */
  SImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:183 */
  DImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:184 */
  TImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:185 */
  EImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:67 */
  OImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:68 */
  CImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:69 */
  XImode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:70 */
  QQmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:204 */
  HQmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:205 */
  SQmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:206 */
  DQmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:207 */
  TQmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:208 */
  UQQmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:210 */
  UHQmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:211 */
  USQmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:212 */
  UDQmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:213 */
  UTQmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:214 */
  HAmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:216 */
  SAmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:217 */
  DAmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:218 */
  TAmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:219 */
  UHAmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:221 */
  USAmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:222 */
  UDAmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:223 */
  UTAmode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:224 */
  HFmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:29 */
  SFmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:196 */
  DFmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:197 */
  XFmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:26 */
  SDmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:236 */
  DDmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:237 */
  TDmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:238 */
  CQImode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:232 */
  CHImode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:232 */
  CSImode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:232 */
  CDImode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:232 */
  CTImode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:232 */
  CEImode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:232 */
  COImode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:232 */
  CCImode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:232 */
  CXImode,                 /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:232 */
  HCmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:233 */
  SCmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:233 */
  DCmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:233 */
  XCmode,                  /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/machmode.def:233 */
  V4QImode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:59 */
  V2HImode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:59 */
  V8QImode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:60 */
  V4HImode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:60 */
  V2SImode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:60 */
  V16QImode,               /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:61 */
  V8HImode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:61 */
  V4SImode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:61 */
  V2DImode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:61 */
  V4HFmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:62 */
  V2SFmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:62 */
  V8HFmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:63 */
  V4SFmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:63 */
  V2DFmode,                /* /u1/netbsd-HEAD/src/external/gpl3/gcc/dist/gcc/config/arm/arm-modes.def:63 */
  MAX_MACHINE_MODE,

  MIN_MODE_RANDOM = VOIDmode,
  MAX_MODE_RANDOM = BLKmode,

  MIN_MODE_CC = CCmode,
  MAX_MODE_CC = CC_Nmode,

  MIN_MODE_INT = QImode,
  MAX_MODE_INT = XImode,

  MIN_MODE_PARTIAL_INT = VOIDmode,
  MAX_MODE_PARTIAL_INT = VOIDmode,

  MIN_MODE_FRACT = QQmode,
  MAX_MODE_FRACT = TQmode,

  MIN_MODE_UFRACT = UQQmode,
  MAX_MODE_UFRACT = UTQmode,

  MIN_MODE_ACCUM = HAmode,
  MAX_MODE_ACCUM = TAmode,

  MIN_MODE_UACCUM = UHAmode,
  MAX_MODE_UACCUM = UTAmode,

  MIN_MODE_FLOAT = HFmode,
  MAX_MODE_FLOAT = XFmode,

  MIN_MODE_DECIMAL_FLOAT = SDmode,
  MAX_MODE_DECIMAL_FLOAT = TDmode,

  MIN_MODE_COMPLEX_INT = CQImode,
  MAX_MODE_COMPLEX_INT = CXImode,

  MIN_MODE_COMPLEX_FLOAT = HCmode,
  MAX_MODE_COMPLEX_FLOAT = XCmode,

  MIN_MODE_VECTOR_INT = V4QImode,
  MAX_MODE_VECTOR_INT = V2DImode,

  MIN_MODE_VECTOR_FRACT = VOIDmode,
  MAX_MODE_VECTOR_FRACT = VOIDmode,

  MIN_MODE_VECTOR_UFRACT = VOIDmode,
  MAX_MODE_VECTOR_UFRACT = VOIDmode,

  MIN_MODE_VECTOR_ACCUM = VOIDmode,
  MAX_MODE_VECTOR_ACCUM = VOIDmode,

  MIN_MODE_VECTOR_UACCUM = VOIDmode,
  MAX_MODE_VECTOR_UACCUM = VOIDmode,

  MIN_MODE_VECTOR_FLOAT = V4HFmode,
  MAX_MODE_VECTOR_FLOAT = V2DFmode,

  NUM_MACHINE_MODES = MAX_MACHINE_MODE
};

#define CONST_MODE_SIZE const
#define CONST_MODE_BASE_ALIGN const
#define CONST_MODE_IBIT const
#define CONST_MODE_FBIT const

#endif /* insn-modes.h */
