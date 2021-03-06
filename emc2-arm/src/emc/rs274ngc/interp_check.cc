/********************************************************************
* Description: interp_check.cc
*
*   Derived from a work by Thomas Kramer
*
* Author:
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
* Last change:
* $Revision: 1.19 $
* $Author: alex_joni $
* $Date: 2007/08/19 10:51:54 $
********************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rs274ngc.hh"
#include "rs274ngc_return.hh"
#include "interp_internal.hh"

/****************************************************************************/

/*! check_g_codes

Returned Value: int
   If any of the following errors occur, this returns the error shown.
   Otherwise, it returns INTERP_OK.
   1. NCE_DWELL_TIME_MISSING_WITH_G4
   2. NCE_MUST_USE_G0_OR_G1_WITH_G53
   3. NCE_CANNOT_USE_G53_INCREMENTAL
   4. NCE_LINE_WITH_G10_DOES_NOT_HAVE_L2
   5. NCE_P_VALUE_NOT_AN_INTEGER_WITH_G10_L2
   6. NCE_P_VALUE_OUT_OF_RANGE_WITH_G10_L2
   7. NCE_BUG_BAD_G_CODE_MODAL_GROUP_0

Side effects: none

Called by: check_items

This runs checks on g_codes from a block of RS274/NGC instructions.
Currently, all checks are on g_codes in modal group 0.

The read_g function checks for errors which would foul up the reading.
The enhance_block function checks for logical errors in the use of
axis values by g-codes in modal groups 0 and 1.
This function checks for additional logical errors in g_codes.

[Fanuc, page 45, note 4] says there is no maximum for how many g_codes
may be put on the same line, [NCMS] says nothing one way or the other,
so the test for that is not used.

We are suspending any implicit motion g_code when a g_code from our
group 0 is used.  The implicit motion g_code takes effect again
automatically after the line on which the group 0 g_code occurs.  It
is not clear what the intent of [Fanuc] is in this regard. The
alternative is to require that any implicit motion be explicitly
cancelled.

Not all checks on g_codes are included here. Those checks that are
sensitive to whether other g_codes on the same line have been executed
yet are made by the functions called by convert_g.

Our reference sources differ regarding what codes may be used for
dwell time.  [Fanuc, page 58] says use "p" or "x". [NCMS, page 23] says
use "p", "x", or "u". We are allowing "p" only, since it is consistent
with both sources and "x" would be confusing. However, "p" is also used
with G10, where it must be an integer, so reading "p" values is a bit
more trouble than would be nice.

*/

int Interp::check_g_codes(block_pointer block,   //!< pointer to a block to be checked
                         setup_pointer settings)        //!< pointer to machine settings
{
  static char name[] = "check_g_codes";
  int mode0;
  int p_int;

  mode0 = block->g_modes[0];

  if (mode0 == -1) {
  } else if (mode0 == G_4) {
    CHK((block->p_number == -1.0), NCE_DWELL_TIME_MISSING_WITH_G4);
  } else if (mode0 == G_10) {
    p_int = (int) (block->p_number + 0.0001);
    CHK((block->l_number != 2), NCE_LINE_WITH_G10_DOES_NOT_HAVE_L2);
    CHK((((block->p_number + 0.0001) - p_int) > 0.0002),
        NCE_P_VALUE_NOT_AN_INTEGER_WITH_G10_L2);
    CHK(((p_int < 1) || (p_int > 9)), NCE_P_VALUE_OUT_OF_RANGE_WITH_G10_L2);
  } else if (mode0 == G_28) {
  } else if (mode0 == G_30) {
  } else if (mode0 == G_53) {
    CHK(((block->motion_to_be != G_0) && (block->motion_to_be != G_1)),
        NCE_MUST_USE_G0_OR_G1_WITH_G53);
    CHK(((block->g_modes[3] == G_91) ||
         ((block->g_modes[3] != G_90) &&
          (settings->distance_mode == MODE_INCREMENTAL))),
        NCE_CANNOT_USE_G53_INCREMENTAL);
  } else if (mode0 == G_92) {
  } else if ((mode0 == G_92_1) || (mode0 == G_92_2) || (mode0 == G_92_3)) {
  } else
    ERM(NCE_BUG_BAD_G_CODE_MODAL_GROUP_0);
  return INTERP_OK;
}

/****************************************************************************/

/*! check_items

Returned Value: int
   If any one of check_g_codes, check_m_codes, and check_other_codes
   returns an error code, this returns that code.
   Otherwise, it returns INTERP_OK.

Side effects: none

Called by: parse_line

This runs checks on a block of RS274 code.

The functions named read_XXXX check for errors which would foul up the
reading. This function checks for additional logical errors.

A block has an array of g_codes, which are initialized to -1
(meaning no code). This calls check_g_codes to check the g_codes.

A block has an array of m_codes, which are initialized to -1
(meaning no code). This calls check_m_codes to check the m_codes.

Items in the block which are not m or g codes are checked by
check_other_codes.

*/

int Interp::check_items(block_pointer block,     //!< pointer to a block to be checked
                       setup_pointer settings)  //!< pointer to machine settings
{
  static char name[] = "check_items";
  int status;

  CHP(check_g_codes(block, settings));
  CHP(check_m_codes(block));
  CHP(check_other_codes(block));
  return INTERP_OK;
}

/****************************************************************************/

/*! check_m_codes

Returned Value: int
   If any of the following errors occur, this returns the error code shown.
   Otherwise, it returns INTERP_OK.
   1. There are too many m codes in the block: NCE_TOO_MANY_M_CODES_ON_LINE

Side effects: none

Called by: check_items

This runs checks on m_codes from a block of RS274/NGC instructions.

The read_m function checks for errors which would foul up the
reading. This function checks for additional errors in m_codes.

*/

int Interp::check_m_codes(block_pointer block)   //!< pointer to a block to be checked
{
  static char name[] = "check_m_codes";

  CHK((block->m_count > MAX_EMS), NCE_TOO_MANY_M_CODES_ON_LINE);
  return INTERP_OK;
}

/****************************************************************************/

/*! check_other_codes

Returned Value: int
   If any of the following errors occur, this returns the error code shown.
   Otherwise, it returns INTERP_OK.
   1. An A-axis value is given with a canned cycle (g80 to g89):
      NCE_CANNOT_PUT_AN_A_IN_CANNED_CYCLE
   2. A B-axis value is given with a canned cycle (g80 to g89):
      NCE_CANNOT_PUT_A_B_IN_CANNED_CYCLE
   3. A C-axis value is given with a canned cycle (g80 to g89):
      NCE_CANNOT_PUT_A_C_IN_CANNED_CYCLE
   4. A d word is in a block with no cutter_radius_compensation_on command:
      NCE_D_WORD_WITH_NO_G41_OR_G42
   5. An h_number is in a block with no tool length offset setting:
      NCE_H_WORD_WITH_NO_G43
   6. An i_number is in a block with no G code that uses it:
      NCE_I_WORD_WITH_NO_G2_OR_G3_OR_G87_TO_USE_IT
   7. A j_number is in a block with no G code that uses it:
      NCE_J_WORD_WITH_NO_G2_OR_G3_OR_G87_TO_USE_IT
   8. A k_number is in a block with no G code that uses it:
      NCE_K_WORD_WITH_NO_G2_OR_G3_OR_G87_TO_USE_IT
   9. A l_number is in a block with no G code that uses it:
      NCE_L_WORD_WITH_NO_CANNED_CYCLE_OR_G10
  10. A p_number is in a block with no G code that uses it:
      NCE_P_WORD_WITH_NO_G4_G10_G64_G82_G86_G88_G89
  11. A q_number is in a block with no G code that uses it:
      NCE_Q_WORD_WITH_NO_G83_OR_M66
  12. An r_number is in a block with no G code that uses it:
      NCE_R_WORD_WITH_NO_G_CODE_THAT_USES_IT
  13. A k word is missing from a G33 block:
      NCE_K_WORD_MISSING_WITH_G33
  14. An e word is in a block with no G76 or M66 to use it:
      NCE_E_WORD_WITH_NO_G76_OR_M66_TO_USE_IT

Side effects: none

Called by: check_items

This runs checks on codes from a block of RS274/NGC code which are
not m or g codes.

The functions named read_XXXX check for errors which would foul up the
reading. This function checks for additional logical errors in codes.

*/

int Interp::check_other_codes(block_pointer block)       //!< pointer to a block of RS274/NGC instructions
{
  static char name[] = "check_other_codes";
  int motion;

  motion = block->motion_to_be;
#ifndef LATHE
  if (block->a_flag != OFF) {
    CHK(((block->g_modes[1] > G_80) && (block->g_modes[1] < G_90)),
        NCE_CANNOT_PUT_AN_A_IN_CANNED_CYCLE);
  }
  if (block->b_flag != OFF) {
    CHK(((block->g_modes[1] > G_80) && (block->g_modes[1] < G_90)),
        NCE_CANNOT_PUT_A_B_IN_CANNED_CYCLE);
  }
  if (block->c_flag != OFF) {
    CHK(((block->g_modes[1] > G_80) && (block->g_modes[1] < G_90)),
        NCE_CANNOT_PUT_A_C_IN_CANNED_CYCLE);
  }
#else
  CHK(((block->g_modes[1] > G_80) && (block->g_modes[1] < G_90)),
	NCE_CANNED_CYCLES_NOT_SUPPORTED);
#endif
  if (block->d_flag == ON) {
    CHKS(((block->g_modes[7] != G_41) && (block->g_modes[7] != G_42) &&
        (block->g_modes[7] != G_41_1) && (block->g_modes[7] != G_42_1) &&
	(block->g_modes[14] != G_96) && (block->m_modes[5] != 66)),
        "D word with no G41, G41.1, G42, G42.2, or G96 to use it");
  }
  if (block->s_number < 0) {
    CHK((block->g_modes[14] == G_96), NCE_S_WORD_MISSING_WITH_G96);
  }
   
  if (block->h_flag == ON) {
    CHK((block->g_modes[8] != G_43 && motion != G_76), NCE_H_WORD_WITH_NO_G43);
  }

  if (block->i_flag == ON) {    /* could still be useless if yz_plane arc */
    CHKS(((motion != G_2) && (motion != G_3) && (motion != G_76) && (motion != G_87) && (block->g_modes[8] != G_43_1)),
        "I word with no G2, G3, G76, G87, or G43.1 to use it");
  }

  if (block->j_flag == ON) {    /* could still be useless if xz_plane arc */
    CHK(((motion != G_2) && (motion != G_3) && (motion != G_76) && (motion != G_87)),
        NCE_J_WORD_WITH_NO_G2_OR_G3_G76_OR_G87_TO_USE_IT);
  }

  if (block->k_flag == ON) {    /* could still be useless if xy_plane arc */
    CHKS(((motion != G_2) && (motion != G_3) && (motion != G_33) && (motion != G_33_1) && (motion != G_76) && (motion != G_87) && (block->g_modes[8] != G_43_1)),
        "K word with no G2, G3, G76, G87, or G43.1 to use it");
  }

  if (motion == G_33 || motion == G_33_1) {
    CHK((block->k_flag == OFF),	NCE_K_WORD_MISSING_WITH_G33);
    CHK((block->f_number != -1), NCE_F_WORD_USED_WITH_G33);
  }

  if (motion == G_76) {
    // pitch
    CHK((block->p_number == -1), NCE_P_WORD_MISSING_WITH_G76);

    CHK((block->i_flag == OFF || block->j_flag == OFF || block->k_flag == OFF),
            NCE_I_J_OR_K_WORDS_MISSING_WITH_G76);
  }

  if (block->l_number != -1) {
    CHKS((((motion < G_81) || (motion > G_89)) &&
         (block->g_modes[0] != G_10) &&
         (motion != G_76) &&
         (block->g_modes[7] != G_41) &&
         (block->g_modes[7] != G_41_1) &&
         (block->g_modes[7] != G_42) &&
         (block->g_modes[7] != G_42_1) &&
	 (block->m_modes[5] != 66)),
        "L word with no G10, cutter compensation, or canned cycle");
  }

  if (block->p_number != -1.0) {
    CHK(((block->g_modes[0] != G_10) &&
         (block->g_modes[0] != G_4) &&
	 (block->g_modes[13] != G_64) &&
         (block->m_modes[5] != 62) &&
         (block->m_modes[5] != 63) &&
         (block->m_modes[5] != 64) &&
         (block->m_modes[5] != 65) &&
         (block->m_modes[5] != 66) &&
         (block->m_modes[5] != 67) &&
         (block->m_modes[5] != 68) &&
         (block->m_modes[9] != 50) &&
         (block->m_modes[9] != 51) &&
         (block->m_modes[9] != 52) &&
         (block->m_modes[9] != 53) &&
         (block->user_m != 1) &&
         (motion != G_82) && (motion != G_86) &&
         (motion != G_88) && (motion != G_89) && (motion != G_76)),
        NCE_P_WORD_WITH_NO_G4_G10_G64_G76_G82_G86_G88_G89);
  }

  if (block->q_number != -1.0) {
    CHK((motion != G_83) && (block->user_m != 1) && (motion != G_76) &&
	    (block->m_modes[5] != 66), 
            NCE_Q_WORD_WITH_NO_G83_OR_M66);
  }

  if (block->e_flag == ON) {
    CHK(((motion != G_76) && (block->m_modes[5] != 66)), NCE_E_WORD_WITH_NO_G76_OR_M66_TO_USE_IT);
  }

  if (block->r_flag == ON) {
    CHK(((motion != G_2) && (motion != G_3) && (motion != G_76) &&
         ((motion < G_81) || (motion > G_89)) && (block->g_modes[7] != G_41_1) &&
         (block->g_modes[7] != G_42_1)),
        NCE_R_WORD_WITH_NO_G_CODE_THAT_USES_IT);
  }

  return INTERP_OK;
}
