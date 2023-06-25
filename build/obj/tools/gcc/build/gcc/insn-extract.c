/* Generated automatically by the program `genextract'
   from the machine description file `md'.  */

#define IN_TARGET_CODE 1
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "rtl.h"
#include "insn-config.h"
#include "recog.h"
#include "diagnostic-core.h"

/* This variable is used as the "location" of any missing operand
   whose numbers are skipped by a given pattern.  */
static rtx junk ATTRIBUTE_UNUSED;

void
insn_extract (rtx_insn *insn)
{
  rtx *ro = recog_data.operand;
  rtx **ro_loc = recog_data.operand_loc;
  rtx pat = PATTERN (insn);
  int i ATTRIBUTE_UNUSED; /* only for peepholes */

  if (flag_checking)
    {
      memset (ro, 0xab, sizeof (*ro) * MAX_RECOG_OPERANDS);
      memset (ro_loc, 0xab, sizeof (*ro_loc) * MAX_RECOG_OPERANDS);
    }

  switch (INSN_CODE (insn))
    {
    default:
      /* Control reaches here if insn_extract has been called with an
         unrecognizable insn (code -1), or an insn whose INSN_CODE
         corresponds to a DEFINE_EXPAND in the machine description;
         either way, a bug.  */
      if (INSN_CODE (insn) < 0)
        fatal_insn ("unrecognizable insn:", insn);
      else
        fatal_insn ("insn with invalid code number:", insn);

    case 552:
    case 551:
    case 550:
    case 549:
    case 548:
    case 547:
    case 546:
    case 545:
    case 544:
    case 543:
    case 542:
    case 541:
      for (i = XVECLEN (pat, 0) - 1; i >= 0; i--)
          ro[i] = *(ro_loc[i] = &XVECEXP (pat, 0, i));
      break;

    case 429:  /* atomic_test_and_set_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0);
      recog_data.dup_num[1] = 1;
      break;

    case 428:  /* atomic_compare_and_swapsi_1 */
    case 427:  /* atomic_compare_and_swaphi_1 */
    case 426:  /* atomic_compare_and_swapqi_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 2);
      recog_data.dup_num[0] = 4;
      recog_data.dup_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 1);
      recog_data.dup_num[1] = 3;
      recog_data.dup_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 0);
      recog_data.dup_num[2] = 2;
      recog_data.dup_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0);
      recog_data.dup_num[3] = 2;
      recog_data.dup_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 2);
      recog_data.dup_num[4] = 4;
      recog_data.dup_loc[5] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 1);
      recog_data.dup_num[5] = 3;
      recog_data.dup_loc[6] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0);
      recog_data.dup_num[6] = 2;
      break;

    case 424:  /* stack_tie */
      ro[0] = *(ro_loc[0] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 1));
      break;

    case 423:  /* ctrapsi4_cf */
    case 422:  /* ctraphi4_cf */
    case 421:  /* ctrapqi4_cf */
    case 420:  /* ctrapsi4 */
    case 419:  /* ctraphi4 */
    case 418:  /* ctrapqi4 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (pat, 1));
      break;

    case 416:  /* cosxf2 */
    case 415:  /* cosdf2 */
    case 414:  /* cossf2 */
    case 413:  /* sinxf2 */
    case 412:  /* sindf2 */
    case 411:  /* sinsf2 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 0));
      break;

    case 405:  /* indirect_jump */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 1));
      break;

    case 403:  /* *unlink */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 0;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
      recog_data.dup_num[1] = 0;
      break;

    case 402:  /* *link */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 2), 1), 1));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 1);
      recog_data.dup_num[0] = 0;
      break;

    case 401:  /* *m68k_load_multiple_automod */
    case 399:  /* *m68k_store_multiple_automod */
      ro[0] = *(ro_loc[0] = &PATTERN (insn));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      break;

    case 400:  /* *m68k_load_multiple */
    case 398:  /* *m68k_store_multiple */
      ro[0] = *(ro_loc[0] = &PATTERN (insn));
      ro[1] = *(ro_loc[1] = &XVECEXP (pat, 0, 0));
      break;

    case 425:  /* ib */
    case 417:  /* trap */
    case 397:  /* *return */
    case 396:  /* nop */
    case 395:  /* blockage */
      break;

    case 388:  /* *dbge_si */
    case 387:  /* *dbge_hi */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0);
      recog_data.dup_num[0] = 0;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[1] = 0;
      break;

    case 386:  /* *dbne_si */
    case 385:  /* *dbne_hi */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0);
      recog_data.dup_num[0] = 0;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[1] = 0;
      break;

    case 384:  /* *tablejump_pcrel_hi */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 0), 0));
      break;

    case 383:  /* *tablejump_pcrel_si */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 0), 0));
      break;

    case 382:  /* *tablejump_internal */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 1));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 0), 0));
      break;

    case 381:  /* jump */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 378:  /* scc0_di_5200 */
    case 377:  /* scc0_di */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (pat, 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 375:  /* *insv_bfset_reg */
    case 374:  /* *insv_bfclr_reg */
    case 370:  /* *insv_bfset_mem */
    case 369:  /* *insv_bfclr_mem */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 0), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 0), 2));
      break;

    case 368:  /* *insv_bfchg_mem */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 0), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 0), 2));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 0), 0);
      recog_data.dup_num[0] = 0;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 1);
      recog_data.dup_num[1] = 1;
      recog_data.dup_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 2);
      recog_data.dup_num[2] = 2;
      break;

    case 373:  /* *extv_bfextu_reg */
    case 372:  /* *extv_bfexts_reg */
    case 367:  /* *extzv_bfextu_mem */
    case 366:  /* *extv_bfexts_mem */
    case 365:  /* *extv_8_16_reg */
    case 363:  /* *extzv_8_16_reg */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 2));
      break;

    case 364:  /* *extv_32_mem */
    case 362:  /* *extzv_32_mem */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 2));
      break;

    case 376:  /* *insv_bfins_reg */
    case 371:  /* *insv_bfins_mem */
    case 361:  /* *insv_8_16_reg */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 0), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 0), 2));
      ro[3] = *(ro_loc[3] = &XEXP (pat, 1));
      break;

    case 360:  /* *insv_32_mem */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 0), 2));
      ro[2] = *(ro_loc[2] = &XEXP (pat, 1));
      break;

    case 359:  /* *bclrmemqi_ext */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 0), 2), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 0), 2), 1));
      break;

    case 358:  /* bclrmemqi */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 0), 2), 1));
      break;

    case 357:  /* *bclrdreg */
    case 356:  /* *bchgdreg */
    case 355:  /* *bsetdreg */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 354:  /* *bsetmemqi_ext */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 0;
      break;

    case 353:  /* bsetmemqi */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 0;
      break;

    case 329:  /* subreg1lshrdi_const32 */
    case 318:  /* subregsi1ashrdi_const32 */
    case 317:  /* subreghi1ashrdi_const32 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      break;

    case 304:  /* ashldi_sexthi */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 303:  /* ashldi_extsi */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 302:  /* *m68k.md:4400 */
    case 300:  /* *m68k.md:4386 */
    case 279:  /* *m68k.md:4042 */
    case 277:  /* *m68k.md:4028 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 0);
      recog_data.dup_num[0] = 0;
      break;

    case 243:  /* udivmodhi4 */
    case 242:  /* divmodhi4 */
    case 241:  /* *m68k.md:3585 */
    case 240:  /* *m68k.md:3567 */
    case 239:  /* *m68k.md:3543 */
    case 238:  /* *m68k.md:3525 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 209:  /* const_smulsi3_highpart */
    case 207:  /* const_umulsi3_highpart */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 1));
      break;

    case 208:  /* *m68k.md:3320 */
    case 206:  /* *m68k.md:3277 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 1), 0));
      break;

    case 205:  /* *m68k.md:3241 */
    case 203:  /* *m68k.md:3206 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 204:  /* *m68k.md:3230 */
    case 202:  /* *m68k.md:3191 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0), 1), 0);
      recog_data.dup_num[1] = 2;
      break;

    case 262:  /* iorsi_zexthi_ashl16 */
    case 200:  /* umulhisi3 */
    case 196:  /* mulhisi3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      break;

    case 173:  /* subdi_dishl32 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 0);
      recog_data.dup_num[0] = 0;
      break;

    case 172:  /* subdi_sexthishl32 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 218:  /* mulxf3_floatqi_68881 */
    case 217:  /* muldf3_floatqi_68881 */
    case 216:  /* mulsf3_floatqi_68881 */
    case 215:  /* mulxf3_floathi_68881 */
    case 214:  /* muldf3_floathi_68881 */
    case 213:  /* mulsf3_floathi_68881 */
    case 212:  /* mulxf3_floatsi_68881 */
    case 211:  /* muldf3_floatsi_68881 */
    case 210:  /* mulsf3_floatsi_68881 */
    case 166:  /* addxf3_floatqi_68881 */
    case 165:  /* adddf3_floatqi_68881 */
    case 164:  /* addsf3_floatqi_68881 */
    case 163:  /* addxf3_floathi_68881 */
    case 162:  /* adddf3_floathi_68881 */
    case 161:  /* addsf3_floathi_68881 */
    case 160:  /* addxf3_floatsi_68881 */
    case 159:  /* adddf3_floatsi_68881 */
    case 158:  /* addsf3_floatsi_68881 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      break;

    case 271:  /* *m68k.md:3945 */
    case 268:  /* *m68k.md:3921 */
    case 261:  /* *m68k.md:3831 */
    case 258:  /* *m68k.md:3807 */
    case 252:  /* *m68k.md:3733 */
    case 249:  /* *m68k.md:3709 */
    case 157:  /* *m68k.md:2773 */
    case 154:  /* *m68k.md:2677 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 0;
      break;

    case 352:  /* *m68k.md:5363 */
    case 350:  /* rotrhi_lowpart */
    case 347:  /* *rotlqi3_lowpart */
    case 345:  /* *rotlhi3_lowpart */
    case 341:  /* *m68k.md:5232 */
    case 339:  /* *m68k.md:5216 */
    case 328:  /* *m68k.md:4930 */
    case 326:  /* *m68k.md:4914 */
    case 314:  /* *m68k.md:4684 */
    case 312:  /* *m68k.md:4668 */
    case 270:  /* *m68k.md:3937 */
    case 267:  /* *m68k.md:3913 */
    case 260:  /* *m68k.md:3823 */
    case 257:  /* *m68k.md:3800 */
    case 251:  /* *m68k.md:3725 */
    case 248:  /* *m68k.md:3701 */
    case 180:  /* *m68k.md:3019 */
    case 178:  /* *m68k.md:3003 */
    case 156:  /* *m68k.md:2750 */
    case 153:  /* *m68k.md:2628 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 0);
      recog_data.dup_num[0] = 0;
      break;

    case 232:  /* divxf3_floatqi_68881 */
    case 231:  /* divdf3_floatqi_68881 */
    case 230:  /* divsf3_floatqi_68881 */
    case 229:  /* divxf3_floathi_68881 */
    case 228:  /* divdf3_floathi_68881 */
    case 227:  /* divsf3_floathi_68881 */
    case 226:  /* divxf3_floatsi_68881 */
    case 225:  /* divdf3_floatsi_68881 */
    case 224:  /* divsf3_floatsi_68881 */
    case 189:  /* subxf3_floatqi_68881 */
    case 188:  /* subdf3_floatqi_68881 */
    case 187:  /* subsf3_floatqi_68881 */
    case 186:  /* subxf3_floathi_68881 */
    case 185:  /* subdf3_floathi_68881 */
    case 184:  /* subsf3_floathi_68881 */
    case 183:  /* subxf3_floatsi_68881 */
    case 182:  /* subdf3_floatsi_68881 */
    case 181:  /* subsf3_floatsi_68881 */
    case 176:  /* *m68k.md:2987 */
    case 151:  /* *m68k.md:2561 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      break;

    case 351:  /* rotrqi3 */
    case 349:  /* rotrhi3 */
    case 348:  /* rotrsi3 */
    case 346:  /* rotlqi3 */
    case 344:  /* rotlhi3 */
    case 343:  /* rotlsi3 */
    case 340:  /* lshrqi3 */
    case 338:  /* lshrhi3 */
    case 337:  /* lshrsi3 */
    case 336:  /* lshrsi_17_24 */
    case 333:  /* *lshrdi3_const */
    case 327:  /* ashrqi3 */
    case 325:  /* ashrhi3 */
    case 324:  /* ashrsi3 */
    case 322:  /* ashrdi_const */
    case 316:  /* *m68k.md:4703 */
    case 313:  /* ashlqi3 */
    case 311:  /* ashlhi3 */
    case 310:  /* ashlsi3 */
    case 309:  /* ashlsi_17_24 */
    case 307:  /* *ashldi3 */
    case 269:  /* xorqi3 */
    case 266:  /* xorhi3 */
    case 265:  /* xorsi3_5200 */
    case 264:  /* xorsi3_internal */
    case 259:  /* iorqi3 */
    case 256:  /* iorhi3 */
    case 255:  /* iorsi3_5200 */
    case 254:  /* iorsi3_internal */
    case 250:  /* andqi3 */
    case 247:  /* andhi3 */
    case 246:  /* andsi3_5200 */
    case 245:  /* andsi3_internal */
    case 244:  /* *andsi3_split */
    case 237:  /* divdf3_cf */
    case 236:  /* divsf3_cf */
    case 235:  /* divxf3_68881 */
    case 234:  /* divdf3_68881 */
    case 233:  /* divsf3_68881 */
    case 223:  /* fmuldf3_cf */
    case 222:  /* fmulsf3_cf */
    case 221:  /* mulxf3_68881 */
    case 220:  /* mulsf_68881 */
    case 219:  /* muldf_68881 */
    case 199:  /* *mulsi3_cf */
    case 198:  /* *mulsi3_68020 */
    case 195:  /* mulhi3 */
    case 194:  /* subdf3_cf */
    case 193:  /* subsf3_cf */
    case 192:  /* subxf3_68881 */
    case 191:  /* subdf3_68881 */
    case 190:  /* subsf3_68881 */
    case 179:  /* subqi3 */
    case 177:  /* subhi3 */
    case 175:  /* subsi3 */
    case 171:  /* adddf3_cf */
    case 170:  /* addsf3_cf */
    case 169:  /* addxf3_68881 */
    case 168:  /* adddf3_68881 */
    case 167:  /* addsf3_68881 */
    case 155:  /* addqi3 */
    case 152:  /* addhi3 */
    case 150:  /* *addsi3_5200 */
    case 149:  /* *addsi3_internal */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 148:  /* addsi_lshrsi_31 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 174:  /* subdi3 */
    case 147:  /* adddi3 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 394:  /* *symbolic_call_value_bsr */
    case 393:  /* *symbolic_call_value_jsr */
    case 392:  /* *non_symbolic_call_value */
    case 390:  /* *sibcall_value */
    case 263:  /* iorsi_zext */
    case 253:  /* iordi_zext */
    case 201:  /* *mulhisisi3_z */
    case 197:  /* *mulhisisi3_s */
    case 146:  /* adddi_dishl32 */
    case 145:  /* *adddi_dilshr32_cf */
    case 144:  /* *adddi_dilshr32 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 143:  /* adddi_sexthishl32 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 142:  /* adddi_lshrdi_63 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 121:  /* fix_truncdfqi2 */
    case 120:  /* fix_truncdfhi2 */
    case 119:  /* fix_truncdfsi2 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 93:  /* extendplussidi */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      break;

    case 321:  /* *ashrdi_const32_mem */
    case 92:  /* extendsidi2 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 410:  /* truncxfsf2 */
    case 409:  /* truncxfdf2 */
    case 408:  /* extenddfxf2 */
    case 407:  /* extendsfxf2 */
    case 342:  /* rotlsi_16 */
    case 335:  /* lshrsi_16 */
    case 334:  /* lshrsi_31 */
    case 332:  /* *lshrdi_const63 */
    case 331:  /* *lshrdi_const32 */
    case 330:  /* *lshrdi3_const1 */
    case 323:  /* ashrsi_31 */
    case 320:  /* *ashrdi_const32 */
    case 319:  /* *ashrdi3_const1 */
    case 315:  /* ashrsi_16 */
    case 308:  /* ashlsi_16 */
    case 306:  /* *ashldi3_const32 */
    case 305:  /* *ashldi3_const1 */
    case 301:  /* one_cmplqi2 */
    case 299:  /* one_cmplhi2 */
    case 298:  /* one_cmplsi2_5200 */
    case 297:  /* one_cmplsi2_internal */
    case 296:  /* *clzsi2_cf */
    case 295:  /* *clzsi2_68k */
    case 294:  /* absdf2_cf */
    case 293:  /* abssf2_cf */
    case 292:  /* absxf2_68881 */
    case 291:  /* absdf2_68881 */
    case 290:  /* abssf2_68881 */
    case 289:  /* sqrtdf2_cf */
    case 288:  /* sqrtsf2_cf */
    case 287:  /* sqrtxf2_68881 */
    case 286:  /* sqrtdf2_68881 */
    case 285:  /* sqrtsf2_68881 */
    case 284:  /* negdf2_cf */
    case 283:  /* negsf2_cf */
    case 282:  /* negxf2_68881 */
    case 281:  /* negdf2_68881 */
    case 280:  /* negsf2_68881 */
    case 278:  /* negqi2 */
    case 276:  /* neghi2 */
    case 275:  /* negsi2_5200 */
    case 274:  /* negsi2_internal */
    case 273:  /* negdi2_5200 */
    case 272:  /* negdi2_internal */
    case 141:  /* fixdfsi2_cf */
    case 140:  /* fixsfsi2_cf */
    case 139:  /* fixxfsi2_68881 */
    case 138:  /* fixdfsi2_68881 */
    case 137:  /* fixsfsi2_68881 */
    case 136:  /* fixdfhi2_cf */
    case 135:  /* fixsfhi2_cf */
    case 134:  /* fixxfhi2_68881 */
    case 133:  /* fixdfhi2_68881 */
    case 132:  /* fixsfhi2_68881 */
    case 131:  /* fixdfqi2_cf */
    case 130:  /* fixsfqi2_cf */
    case 129:  /* fixxfqi2_68881 */
    case 128:  /* fixdfqi2_68881 */
    case 127:  /* fixsfqi2_68881 */
    case 126:  /* ftruncdf2_cf */
    case 125:  /* ftruncsf2_cf */
    case 124:  /* ftruncxf2_68881 */
    case 123:  /* ftruncdf2_68881 */
    case 122:  /* ftruncsf2_68881 */
    case 118:  /* floatqidf2_cf */
    case 117:  /* floatqisf2_cf */
    case 116:  /* floatqixf2_68881 */
    case 115:  /* floatqidf2_68881 */
    case 114:  /* floatqisf2_68881 */
    case 113:  /* floathidf2_cf */
    case 112:  /* floathisf2_cf */
    case 111:  /* floathixf2_68881 */
    case 110:  /* floathidf2_68881 */
    case 109:  /* floathisf2_68881 */
    case 108:  /* floatsidf2_cf */
    case 107:  /* floatsisf2_cf */
    case 106:  /* floatsixf2_68881 */
    case 105:  /* floatsidf2_68881 */
    case 104:  /* floatsisf2_68881 */
    case 103:  /* *truncdfsf2_68881 */
    case 102:  /* truncdfsf2_cf */
    case 101:  /* *m68k.md:2062 */
    case 100:  /* extendsfdf2_cf */
    case 99:  /* *m68k.md:2008 */
    case 98:  /* *68k_extendqisi2 */
    case 97:  /* *cfv4_extendqisi2 */
    case 96:  /* extendqihi2 */
    case 95:  /* *68k_extendhisi2 */
    case 94:  /* *cfv4_extendhisi2 */
    case 91:  /* extendhidi2 */
    case 90:  /* extendqidi2 */
    case 89:  /* zero_extendqisi2 */
    case 88:  /* *zero_extendqisi2_cfv4 */
    case 87:  /* *zero_extendqihi2 */
    case 86:  /* zero_extendhisi2 */
    case 85:  /* *zero_extendhisi2_cf */
    case 84:  /* *zero_extendsidi2 */
    case 83:  /* zero_extendhidi2 */
    case 82:  /* zero_extendqidi2 */
    case 81:  /* *zero_extend_dec */
    case 80:  /* *zero_extend_inc */
    case 79:  /* truncsihi2 */
    case 78:  /* trunchiqi2 */
    case 77:  /* truncsiqi2 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 391:  /* *call */
    case 389:  /* *sibcall */
    case 64:  /* *movstrictqi_cf */
    case 63:  /* *m68k.md:1150 */
    case 60:  /* *m68k.md:1117 */
    case 59:  /* *m68k.md:1110 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (pat, 1));
      break;

    case 404:  /* load_got */
    case 52:  /* *movsi_const0 */
    case 51:  /* *movsi_const0_68040_60 */
    case 50:  /* *movsi_const0_68000_10 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      break;

    case 31:  /* cstore_bftstsi_insn */
    case 30:  /* cstore_bftstqi_insn */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (pat, 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XEXP (pat, 1), 0), 2));
      break;

    case 29:  /* cbranch_bftstsi_insn */
    case 28:  /* cbranch_bftstqi_insn */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 2));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      break;

    case 27:  /* cbranchsi4_btst_reg_insn_1 */
    case 26:  /* cbranchsi4_btst_mem_insn_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 2));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      break;

    case 25:  /* cbranchsi4_btst_reg_insn */
    case 24:  /* cbranchsi4_btst_mem_insn */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 2), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      break;

    case 380:  /* scc_di_5200 */
    case 379:  /* scc_di */
    case 48:  /* cstoredf4_insn_cf */
    case 47:  /* cstoresf4_insn_cf */
    case 46:  /* cstorexf4_insn_68881 */
    case 45:  /* cstoredf4_insn_68881 */
    case 44:  /* cstoresf4_insn_68881 */
    case 23:  /* cstoresi4_insn_cf */
    case 22:  /* cstorehi4_insn_cf */
    case 21:  /* cstoreqi4_insn_cf */
    case 20:  /* cstoresi4_insn */
    case 19:  /* cstorehi4_insn */
    case 18:  /* cstoreqi4_insn */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (pat, 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 43:  /* cbranchxf4_insn_rev_cf */
    case 42:  /* cbranchdf4_insn_rev_cf */
    case 41:  /* cbranchsf4_insn_rev_cf */
    case 40:  /* cbranchxf4_insn_rev_68881 */
    case 39:  /* cbranchdf4_insn_rev_68881 */
    case 38:  /* cbranchsf4_insn_rev_68881 */
    case 17:  /* cbranchsi4_insn_cf_rev */
    case 16:  /* cbranchhi4_insn_cf_rev */
    case 15:  /* cbranchqi4_insn_cf_rev */
    case 11:  /* cbranchsi4_insn_rev */
    case 10:  /* cbranchhi4_insn_rev */
    case 9:  /* cbranchqi4_insn_rev */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 2), 0));
      break;

    case 37:  /* cbranchxf4_insn_cf */
    case 36:  /* cbranchdf4_insn_cf */
    case 35:  /* cbranchsf4_insn_cf */
    case 34:  /* cbranchxf4_insn_68881 */
    case 33:  /* cbranchdf4_insn_68881 */
    case 32:  /* cbranchsf4_insn_68881 */
    case 14:  /* cbranchsi4_insn_cf */
    case 13:  /* cbranchhi4_insn_cf */
    case 12:  /* cbranchqi4_insn_cf */
    case 8:  /* cbranchsi4_insn */
    case 7:  /* cbranchhi4_insn */
    case 6:  /* cbranchqi4_insn */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      break;

    case 5:  /* cbranchdi4_insn */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      ro[5] = *(ro_loc[5] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 4:  /* bne0_di */
    case 3:  /* beq0_di */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 406:  /* *lea */
    case 76:  /* pushasi */
    case 75:  /* *m68k.md:1608 */
    case 74:  /* *m68k.md:1568 */
    case 73:  /* *m68k.md:1554 */
    case 72:  /* *m68k.md:1515 */
    case 71:  /* *m68k.md:1475 */
    case 70:  /* movdf_cf_hard */
    case 69:  /* movdf_cf_soft */
    case 68:  /* *m68k.md:1355 */
    case 67:  /* movsf_cf_hard */
    case 66:  /* movsf_cf_soft */
    case 65:  /* *m68k.md:1209 */
    case 62:  /* *m68k.md:1137 */
    case 61:  /* *m68k.md:1130 */
    case 58:  /* *m68k.md:1094 */
    case 57:  /* *m68k.md:1084 */
    case 56:  /* *m68k.md:1068 */
    case 55:  /* *movsi_cf */
    case 54:  /* *movsi_m68k2 */
    case 53:  /* *movsi_m68k */
    case 49:  /* pushexthisi_const */
    case 2:  /* pushdi */
    case 1:  /* *movdf_internal */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (pat, 1));
      break;

    }
}
