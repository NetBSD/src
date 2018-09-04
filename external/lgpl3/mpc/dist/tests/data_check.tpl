/* data_check.tpl -- template file for checking result against data
   file.

   Usage: Before including this template file in your source file,
   #define the prototype of the function under test in the
   CALL_MPC_FUNCTION symbol, see tadd_tmpl.c for an example.

   To test the reuse of the first parameter, #define the
   MPC_FUNCTION_CALL_REUSE_OP1 and MPC_FUNCTION_CALL_REUSE_OP2 symbols
   with the first and second input parameter reused as the output, see
   tadd_tmpl.c for an example. It is not possible to test parameter
   reuse in functions with two output (like mpc_sin_cos) with this
   system.

Copyright (C) 2012, 2013 INRIA

This file is part of GNU MPC.

GNU MPC is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

GNU MPC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see http://www.gnu.org/licenses/ .
*/

#ifndef MPC_FUNCTION_CALL
#error Define MPC_FUNCTION_CALL before including 'data_check.tpl'. 
#endif

int
data_check_template (const char* descr_file, const char * data_file)
{
  static int rand_counter = 0;

  mpc_datafile_context_t datafile_context;
  mpc_datafile_context_t *dc = &datafile_context;

  mpc_fun_param_t params;
  mpc_operand_t *P = params.P; /* developer-friendly alias */

  read_description (&params, descr_file);
  init_parameters (&params);

  open_datafile (dc, data_file);
  while (datafile_context.nextchar != EOF) {
    read_line (dc, &params);
    set_mpfr_flags (rand_counter);
    MPC_FUNCTION_CALL;
    check_mpfr_flags (rand_counter++);
    check_data (dc, &params, 0);

#ifdef MPC_FUNCTION_CALL_SYMMETRIC
      MPC_FUNCTION_CALL_SYMMETRIC;
      check_data (dc, &params, 0);
#endif

#ifdef MPC_FUNCTION_CALL_REUSE_OP1
    if (copy_parameter (&params, 1, 2) == 0)
      {
        MPC_FUNCTION_CALL_REUSE_OP1;
        check_data (dc, &params, 2);
      }
#endif

#ifdef MPC_FUNCTION_CALL_REUSE_OP2
    if (copy_parameter (&params, 1, 3) == 0)
      {
        MPC_FUNCTION_CALL_REUSE_OP2;
        check_data (dc, &params, 3);
      }
#endif

#ifdef MPC_FUNCTION_CALL_REUSE_OP3
    if (copy_parameter (&params, 1, 4) == 0)
      {
        MPC_FUNCTION_CALL_REUSE_OP3;
        check_data (dc, &params, 4);
      }
#endif
  }
  close_datafile (dc);

  clear_parameters (&params);

  return 0;
}
