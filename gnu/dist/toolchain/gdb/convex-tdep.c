/* OBSOLETE /* Convex stuff for GDB. */
/* OBSOLETE    Copyright (C) 1990, 1991, 1996, 2000 Free Software Foundation, Inc. */
/* OBSOLETE  */
/* OBSOLETE This file is part of GDB. */
/* OBSOLETE  */
/* OBSOLETE This program is free software; you can redistribute it and/or modify */
/* OBSOLETE it under the terms of the GNU General Public License as published by */
/* OBSOLETE the Free Software Foundation; either version 2 of the License, or */
/* OBSOLETE (at your option) any later version. */
/* OBSOLETE  */
/* OBSOLETE This program is distributed in the hope that it will be useful, */
/* OBSOLETE but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE GNU General Public License for more details. */
/* OBSOLETE  */
/* OBSOLETE You should have received a copy of the GNU General Public License */
/* OBSOLETE along with this program; if not, write to the Free Software */
/* OBSOLETE Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #include "defs.h" */
/* OBSOLETE #include "command.h" */
/* OBSOLETE #include "symtab.h" */
/* OBSOLETE #include "value.h" */
/* OBSOLETE #include "frame.h" */
/* OBSOLETE #include "inferior.h" */
/* OBSOLETE #include "gdb_wait.h" */
/* OBSOLETE  */
/* OBSOLETE #include <signal.h> */
/* OBSOLETE #include <fcntl.h> */
/* OBSOLETE  */
/* OBSOLETE #include "gdbcore.h" */
/* OBSOLETE #include <sys/param.h> */
/* OBSOLETE #include <sys/dir.h> */
/* OBSOLETE #include <sys/user.h> */
/* OBSOLETE #include <sys/ioctl.h> */
/* OBSOLETE #include <sys/pcntl.h> */
/* OBSOLETE #include <sys/thread.h> */
/* OBSOLETE #include <sys/proc.h> */
/* OBSOLETE #include <sys/file.h> */
/* OBSOLETE #include "gdb_stat.h" */
/* OBSOLETE #include <sys/mman.h> */
/* OBSOLETE  */
/* OBSOLETE #include "gdbcmd.h" */
/* OBSOLETE  */
/* OBSOLETE CORE_ADDR */
/* OBSOLETE convex_skip_prologue (pc) */
/* OBSOLETE      CORE_ADDR pc; */
/* OBSOLETE { */
/* OBSOLETE   int op, ix; */
/* OBSOLETE   op = read_memory_integer (pc, 2); */
/* OBSOLETE   if ((op & 0xffc7) == 0x5ac0) */
/* OBSOLETE     pc += 2; */
/* OBSOLETE   else if (op == 0x1580) */
/* OBSOLETE     pc += 4; */
/* OBSOLETE   else if (op == 0x15c0) */
/* OBSOLETE     pc += 6; */
/* OBSOLETE   if ((read_memory_integer (pc, 2) & 0xfff8) == 0x7c40 */
/* OBSOLETE       && (read_memory_integer (pc + 2, 2) & 0xfff8) == 0x1240 */
/* OBSOLETE       && (read_memory_integer (pc + 8, 2) & 0xfff8) == 0x7c48) */
/* OBSOLETE     pc += 10; */
/* OBSOLETE   if (read_memory_integer (pc, 2) == 0x1240) */
/* OBSOLETE     pc += 6; */
/* OBSOLETE   for (;;) */
/* OBSOLETE     { */
/* OBSOLETE       op = read_memory_integer (pc, 2); */
/* OBSOLETE       ix = (op >> 3) & 7; */
/* OBSOLETE       if (ix != 6) */
/* OBSOLETE     break; */
/* OBSOLETE       if ((op & 0xfcc0) == 0x3000) */
/* OBSOLETE     pc += 4; */
/* OBSOLETE       else if ((op & 0xfcc0) == 0x3040) */
/* OBSOLETE     pc += 6; */
/* OBSOLETE       else if ((op & 0xfcc0) == 0x2800) */
/* OBSOLETE     pc += 4; */
/* OBSOLETE       else if ((op & 0xfcc0) == 0x2840) */
/* OBSOLETE     pc += 6; */
/* OBSOLETE       else */
/* OBSOLETE     break; */
/* OBSOLETE     } */
/* OBSOLETE   return pc; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE convex_frameless_function_invocation (fi) */
/* OBSOLETE      struct frame_info *fi; */
/* OBSOLETE { */
/* OBSOLETE   int frameless; */
/* OBSOLETE   extern CORE_ADDR text_start, text_end; */
/* OBSOLETE   CORE_ADDR call_addr = SAVED_PC_AFTER_CALL (FI); */
/* OBSOLETE   frameless = (call_addr >= text_start && call_addr < text_end */
/* OBSOLETE            && read_memory_integer (call_addr - 6, 1) == 0x22); */
/* OBSOLETE   return frameless; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE convex_frame_num_args (fi) */
/* OBSOLETE      struct frame_info *fi; */
/* OBSOLETE { */
/* OBSOLETE   int numargs = read_memory_integer (FRAME_ARGS_ADDRESS (fi) - 4, 4); */
/* OBSOLETE   if (numargs < 0 || numargs >= 256) */
/* OBSOLETE     numargs = -1; */
/* OBSOLETE   return numargs; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE exec_file_command (filename, from_tty) */
/* OBSOLETE      char *filename; */
/* OBSOLETE      int from_tty; */
/* OBSOLETE { */
/* OBSOLETE   int val; */
/* OBSOLETE   int n; */
/* OBSOLETE   struct stat st_exec; */
/* OBSOLETE  */
/* OBSOLETE   /* Eliminate all traces of old exec file. */
/* OBSOLETE      Mark text segment as empty.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   if (execfile) */
/* OBSOLETE     free (execfile); */
/* OBSOLETE   execfile = 0; */
/* OBSOLETE   data_start = 0; */
/* OBSOLETE   data_end = 0; */
/* OBSOLETE   text_start = 0; */
/* OBSOLETE   text_end = 0; */
/* OBSOLETE   exec_data_start = 0; */
/* OBSOLETE   exec_data_end = 0; */
/* OBSOLETE   if (execchan >= 0) */
/* OBSOLETE     close (execchan); */
/* OBSOLETE   execchan = -1; */
/* OBSOLETE  */
/* OBSOLETE   n_exec = 0; */
/* OBSOLETE  */
/* OBSOLETE   /* Now open and digest the file the user requested, if any.  *x/ */
/* OBSOLETE  */
/* OBSOLETE   if (filename) */
/* OBSOLETE     { */
/* OBSOLETE       filename = tilde_expand (filename); */
/* OBSOLETE       make_cleanup (free, filename); */
/* OBSOLETE        */
/* OBSOLETE       execchan = openp (getenv ("PATH"), 1, filename, O_RDONLY, 0, */
/* OBSOLETE                     &execfile); */
/* OBSOLETE       if (execchan < 0) */
/* OBSOLETE     perror_with_name (filename); */
/* OBSOLETE  */
/* OBSOLETE       if (myread (execchan, &filehdr, sizeof filehdr) < 0) */
/* OBSOLETE     perror_with_name (filename); */
/* OBSOLETE  */
/* OBSOLETE       if (! IS_SOFF_MAGIC (filehdr.h_magic)) */
/* OBSOLETE     error ("%s: not an executable file.", filename); */
/* OBSOLETE  */
/* OBSOLETE       if (myread (execchan, &opthdr, filehdr.h_opthdr) <= 0) */
/* OBSOLETE     perror_with_name (filename); */
/* OBSOLETE  */
/* OBSOLETE       /* Read through the section headers. */
/* OBSOLETE      For text, data, etc, record an entry in the exec file map. */
/* OBSOLETE      Record text_start and text_end.  *x/ */
/* OBSOLETE  */
/* OBSOLETE       lseek (execchan, (long) filehdr.h_scnptr, 0); */
/* OBSOLETE  */
/* OBSOLETE       for (n = 0; n < filehdr.h_nscns; n++) */
/* OBSOLETE     { */
/* OBSOLETE       if (myread (execchan, &scnhdr, sizeof scnhdr) < 0) */
/* OBSOLETE         perror_with_name (filename); */
/* OBSOLETE  */
/* OBSOLETE       if ((scnhdr.s_flags & S_TYPMASK) >= S_TEXT */
/* OBSOLETE           && (scnhdr.s_flags & S_TYPMASK) <= S_COMON) */
/* OBSOLETE         { */
/* OBSOLETE           exec_map[n_exec].mem_addr = scnhdr.s_vaddr; */
/* OBSOLETE           exec_map[n_exec].mem_end = scnhdr.s_vaddr + scnhdr.s_size; */
/* OBSOLETE           exec_map[n_exec].file_addr = scnhdr.s_scnptr; */
/* OBSOLETE           exec_map[n_exec].type = scnhdr.s_flags & S_TYPMASK; */
/* OBSOLETE           n_exec++; */
/* OBSOLETE  */
/* OBSOLETE           if ((scnhdr.s_flags & S_TYPMASK) == S_TEXT) */
/* OBSOLETE             { */
/* OBSOLETE               text_start = scnhdr.s_vaddr; */
/* OBSOLETE               text_end =  scnhdr.s_vaddr + scnhdr.s_size; */
/* OBSOLETE             } */
/* OBSOLETE         } */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE       fstat (execchan, &st_exec); */
/* OBSOLETE       exec_mtime = st_exec.st_mtime; */
/* OBSOLETE        */
/* OBSOLETE       validate_files (); */
/* OBSOLETE     } */
/* OBSOLETE   else if (from_tty) */
/* OBSOLETE     printf_filtered ("No executable file now.\n"); */
/* OBSOLETE  */
/* OBSOLETE   /* Tell display code (if any) about the changed file name.  *x/ */
/* OBSOLETE   if (exec_file_display_hook) */
/* OBSOLETE     (*exec_file_display_hook) (filename); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE #if 0 */
/* OBSOLETE /* Read data from SOFF exec or core file. */
/* OBSOLETE    Return 0 on success, EIO if address out of bounds. *x/ */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE xfer_core_file (memaddr, myaddr, len) */
/* OBSOLETE      CORE_ADDR memaddr; */
/* OBSOLETE      char *myaddr; */
/* OBSOLETE      int len; */
/* OBSOLETE { */
/* OBSOLETE   register int i; */
/* OBSOLETE   register int n; */
/* OBSOLETE   register int val; */
/* OBSOLETE   int xferchan; */
/* OBSOLETE   char **xferfile; */
/* OBSOLETE   int fileptr; */
/* OBSOLETE   int returnval = 0; */
/* OBSOLETE  */
/* OBSOLETE   while (len > 0) */
/* OBSOLETE     { */
/* OBSOLETE       xferfile = 0; */
/* OBSOLETE       xferchan = 0; */
/* OBSOLETE  */
/* OBSOLETE       /* Determine which file the next bunch of addresses reside in, */
/* OBSOLETE      and where in the file.  Set the file's read/write pointer */
/* OBSOLETE      to point at the proper place for the desired address */
/* OBSOLETE      and set xferfile and xferchan for the correct file. */
/* OBSOLETE      If desired address is nonexistent, leave them zero. */
/* OBSOLETE      i is set to the number of bytes that can be handled */
/* OBSOLETE      along with the next address.  *x/ */
/* OBSOLETE  */
/* OBSOLETE       i = len; */
/* OBSOLETE  */
/* OBSOLETE       for (n = 0; n < n_core; n++) */
/* OBSOLETE     { */
/* OBSOLETE       if (memaddr >= core_map[n].mem_addr && memaddr < core_map[n].mem_end */
/* OBSOLETE           && (core_map[n].thread == -1 */
/* OBSOLETE               || core_map[n].thread == inferior_thread)) */
/* OBSOLETE         { */
/* OBSOLETE           i = min (len, core_map[n].mem_end - memaddr); */
/* OBSOLETE           fileptr = core_map[n].file_addr + memaddr - core_map[n].mem_addr; */
/* OBSOLETE           if (core_map[n].file_addr) */
/* OBSOLETE             { */
/* OBSOLETE               xferfile = &corefile; */
/* OBSOLETE               xferchan = corechan; */
/* OBSOLETE             } */
/* OBSOLETE           break; */
/* OBSOLETE         } */
/* OBSOLETE       else if (core_map[n].mem_addr >= memaddr */
/* OBSOLETE                && core_map[n].mem_addr < memaddr + i) */
/* OBSOLETE         i = core_map[n].mem_addr - memaddr; */
/* OBSOLETE         } */
/* OBSOLETE  */
/* OBSOLETE       if (!xferfile)  */
/* OBSOLETE     for (n = 0; n < n_exec; n++) */
/* OBSOLETE       { */
/* OBSOLETE         if (memaddr >= exec_map[n].mem_addr */
/* OBSOLETE             && memaddr < exec_map[n].mem_end) */
/* OBSOLETE           { */
/* OBSOLETE             i = min (len, exec_map[n].mem_end - memaddr); */
/* OBSOLETE             fileptr = exec_map[n].file_addr + memaddr */
/* OBSOLETE               - exec_map[n].mem_addr; */
/* OBSOLETE             if (exec_map[n].file_addr) */
/* OBSOLETE               { */
/* OBSOLETE                 xferfile = &execfile; */
/* OBSOLETE                 xferchan = execchan; */
/* OBSOLETE               } */
/* OBSOLETE             break; */
/* OBSOLETE           } */
/* OBSOLETE         else if (exec_map[n].mem_addr >= memaddr */
/* OBSOLETE                  && exec_map[n].mem_addr < memaddr + i) */
/* OBSOLETE           i = exec_map[n].mem_addr - memaddr; */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE       /* Now we know which file to use. */
/* OBSOLETE      Set up its pointer and transfer the data.  *x/ */
/* OBSOLETE       if (xferfile) */
/* OBSOLETE     { */
/* OBSOLETE       if (*xferfile == 0) */
/* OBSOLETE         if (xferfile == &execfile) */
/* OBSOLETE           error ("No program file to examine."); */
/* OBSOLETE         else */
/* OBSOLETE           error ("No core dump file or running program to examine."); */
/* OBSOLETE       val = lseek (xferchan, fileptr, 0); */
/* OBSOLETE       if (val < 0) */
/* OBSOLETE         perror_with_name (*xferfile); */
/* OBSOLETE       val = myread (xferchan, myaddr, i); */
/* OBSOLETE       if (val < 0) */
/* OBSOLETE         perror_with_name (*xferfile); */
/* OBSOLETE     } */
/* OBSOLETE       /* If this address is for nonexistent memory, */
/* OBSOLETE      read zeros if reading, or do nothing if writing.  *x/ */
/* OBSOLETE       else */
/* OBSOLETE     { */
/* OBSOLETE       memset (myaddr, '\0', i); */
/* OBSOLETE       returnval = EIO; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE       memaddr += i; */
/* OBSOLETE       myaddr += i; */
/* OBSOLETE       len -= i; */
/* OBSOLETE     } */
/* OBSOLETE   return returnval; */
/* OBSOLETE } */
/* OBSOLETE #endif */
/* OBSOLETE  */
/* OBSOLETE /* Here from info files command to print an address map.  *x/ */
/* OBSOLETE  */
/* OBSOLETE print_maps () */
/* OBSOLETE { */
/* OBSOLETE   struct pmap ptrs[200]; */
/* OBSOLETE   int n; */
/* OBSOLETE  */
/* OBSOLETE   /* ID strings for core and executable file sections *x/ */
/* OBSOLETE  */
/* OBSOLETE   static char *idstr[] = */
/* OBSOLETE     { */
/* OBSOLETE       "0", "text", "data", "tdata", "bss", "tbss",  */
/* OBSOLETE       "common", "ttext", "ctx", "tctx", "10", "11", "12", */
/* OBSOLETE     }; */
/* OBSOLETE  */
/* OBSOLETE   for (n = 0; n < n_core; n++) */
/* OBSOLETE     { */
/* OBSOLETE       core_map[n].which = 0; */
/* OBSOLETE       ptrs[n] = core_map[n]; */
/* OBSOLETE     } */
/* OBSOLETE   for (n = 0; n < n_exec; n++) */
/* OBSOLETE     { */
/* OBSOLETE       exec_map[n].which = 1; */
/* OBSOLETE       ptrs[n_core+n] = exec_map[n]; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   qsort (ptrs, n_core + n_exec, sizeof *ptrs, ptr_cmp); */
/* OBSOLETE  */
/* OBSOLETE   for (n = 0; n < n_core + n_exec; n++) */
/* OBSOLETE     { */
/* OBSOLETE       struct pmap *p = &ptrs[n]; */
/* OBSOLETE       if (n > 0) */
/* OBSOLETE     { */
/* OBSOLETE       if (p->mem_addr < ptrs[n-1].mem_end) */
/* OBSOLETE         p->mem_addr = ptrs[n-1].mem_end; */
/* OBSOLETE       if (p->mem_addr >= p->mem_end) */
/* OBSOLETE         continue; */
/* OBSOLETE     } */
/* OBSOLETE       printf_filtered ("%08x .. %08x  %-6s  %s\n", */
/* OBSOLETE                    p->mem_addr, p->mem_end, idstr[p->type], */
/* OBSOLETE                    p->which ? execfile : corefile); */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Compare routine to put file sections in order. */
/* OBSOLETE    Sort into increasing order on address, and put core file sections */
/* OBSOLETE    before exec file sections if both files contain the same addresses.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static ptr_cmp (a, b) */
/* OBSOLETE      struct pmap *a, *b; */
/* OBSOLETE { */
/* OBSOLETE   if (a->mem_addr != b->mem_addr) return a->mem_addr - b->mem_addr; */
/* OBSOLETE   return a->which - b->which; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Trapped internal variables are used to handle special registers. */
/* OBSOLETE    A trapped i.v. calls a hook here every time it is dereferenced, */
/* OBSOLETE    to provide a new value for the variable, and it calls a hook here */
/* OBSOLETE    when a new value is assigned, to do something with the value. */
/* OBSOLETE     */
/* OBSOLETE    The vector registers are $vl, $vs, $vm, $vN, $VN (N in 0..7). */
/* OBSOLETE    The communication registers are $cN, $CN (N in 0..63). */
/* OBSOLETE    They not handled as regular registers because it's expensive to */
/* OBSOLETE    read them, and their size varies, and they have too many names.  *x/ */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Return 1 if NAME is a trapped internal variable, else 0. *x/ */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE is_trapped_internalvar (name) */
/* OBSOLETE      char *name; */
/* OBSOLETE { */
/* OBSOLETE     if ((name[0] == 'c' || name[0] == 'C') */
/* OBSOLETE     && name[1] >= '0' && name[1] <= '9' */
/* OBSOLETE     && (name[2] == '\0' */
/* OBSOLETE         || (name[2] >= '0' && name[2] <= '9' */
/* OBSOLETE             && name[3] == '\0' && name[1] != '0')) */
/* OBSOLETE     && atoi (&name[1]) < 64) return 1; */
/* OBSOLETE  */
/* OBSOLETE   if ((name[0] == 'v' || name[0] == 'V') */
/* OBSOLETE       && (((name[1] & -8) == '0' && name[2] == '\0') */
/* OBSOLETE       || STREQ (name, "vl") */
/* OBSOLETE       || STREQ (name, "vs")  */
/* OBSOLETE       || STREQ (name, "vm"))) */
/* OBSOLETE     return 1; */
/* OBSOLETE   else return 0; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Return the value of trapped internal variable VAR *x/ */
/* OBSOLETE  */
/* OBSOLETE value */
/* OBSOLETE value_of_trapped_internalvar (var) */
/* OBSOLETE      struct internalvar *var; */
/* OBSOLETE { */
/* OBSOLETE   char *name = var->name; */
/* OBSOLETE   value val; */
/* OBSOLETE   struct type *type; */
/* OBSOLETE   struct type *range_type; */
/* OBSOLETE   long len = *read_vector_register (VL_REGNUM); */
/* OBSOLETE   if (len <= 0 || len > 128) len = 128; */
/* OBSOLETE  */
/* OBSOLETE   if (STREQ (name, "vl")) */
/* OBSOLETE     { */
/* OBSOLETE       val = value_from_longest (builtin_type_int, */
/* OBSOLETE                          (LONGEST) *read_vector_register_1 (VL_REGNUM)); */
/* OBSOLETE     } */
/* OBSOLETE   else if (STREQ (name, "vs")) */
/* OBSOLETE     { */
/* OBSOLETE       val = value_from_longest (builtin_type_int, */
/* OBSOLETE                          (LONGEST) *read_vector_register_1 (VS_REGNUM)); */
/* OBSOLETE     } */
/* OBSOLETE   else if (STREQ (name, "vm")) */
/* OBSOLETE     { */
/* OBSOLETE       long vm[4]; */
/* OBSOLETE       long i, *p; */
/* OBSOLETE       memcpy (vm, read_vector_register_1 (VM_REGNUM), sizeof vm); */
/* OBSOLETE       range_type = */
/* OBSOLETE     create_range_type ((struct type *) NULL, builtin_type_int, 0, len - 1); */
/* OBSOLETE       type = */
/* OBSOLETE     create_array_type ((struct type *) NULL, builtin_type_int, range_type); */
/* OBSOLETE       val = allocate_value (type); */
/* OBSOLETE       p = (long *) VALUE_CONTENTS (val); */
/* OBSOLETE       for (i = 0; i < len; i++)  */
/* OBSOLETE     *p++ = !! (vm[3 - (i >> 5)] & (1 << (i & 037))); */
/* OBSOLETE     } */
/* OBSOLETE   else if (name[0] == 'V') */
/* OBSOLETE     { */
/* OBSOLETE       range_type = */
/* OBSOLETE     create_range_type ((struct type *) NULL, builtin_type_int 0, len - 1); */
/* OBSOLETE       type = */
/* OBSOLETE     create_array_type ((struct type *) NULL, builtin_type_long_long, */
/* OBSOLETE                        range_type); */
/* OBSOLETE       val = allocate_value (type); */
/* OBSOLETE       memcpy (VALUE_CONTENTS (val), */
/* OBSOLETE          read_vector_register_1 (name[1] - '0'), */
/* OBSOLETE          TYPE_LENGTH (type)); */
/* OBSOLETE     } */
/* OBSOLETE   else if (name[0] == 'v') */
/* OBSOLETE     { */
/* OBSOLETE       long *p1, *p2; */
/* OBSOLETE       range_type = */
/* OBSOLETE     create_range_type ((struct type *) NULL, builtin_type_int 0, len - 1); */
/* OBSOLETE       type = */
/* OBSOLETE     create_array_type ((struct type *) NULL, builtin_type_long, */
/* OBSOLETE                        range_type); */
/* OBSOLETE       val = allocate_value (type); */
/* OBSOLETE       p1 = read_vector_register_1 (name[1] - '0'); */
/* OBSOLETE       p2 = (long *) VALUE_CONTENTS (val); */
/* OBSOLETE       while (--len >= 0) {p1++; *p2++ = *p1++;} */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   else if (name[0] == 'c') */
/* OBSOLETE     val = value_from_longest (builtin_type_int, */
/* OBSOLETE                        read_comm_register (atoi (&name[1]))); */
/* OBSOLETE   else if (name[0] == 'C') */
/* OBSOLETE     val = value_from_longest (builtin_type_long_long, */
/* OBSOLETE                        read_comm_register (atoi (&name[1]))); */
/* OBSOLETE  */
/* OBSOLETE   VALUE_LVAL (val) = lval_internalvar; */
/* OBSOLETE   VALUE_INTERNALVAR (val) = var; */
/* OBSOLETE   return val; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Handle a new value assigned to a trapped internal variable *x/ */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE set_trapped_internalvar (var, val, bitpos, bitsize, offset) */
/* OBSOLETE      struct internalvar *var; */
/* OBSOLETE      value val; */
/* OBSOLETE      int bitpos, bitsize, offset; */
/* OBSOLETE {  */
/* OBSOLETE   char *name = var->name; */
/* OBSOLETE   long long newval = value_as_long (val); */
/* OBSOLETE  */
/* OBSOLETE   if (STREQ (name, "vl"))  */
/* OBSOLETE     write_vector_register (VL_REGNUM, 0, newval); */
/* OBSOLETE   else if (STREQ (name, "vs")) */
/* OBSOLETE     write_vector_register (VS_REGNUM, 0, newval); */
/* OBSOLETE   else if (name[0] == 'c' || name[0] == 'C') */
/* OBSOLETE     write_comm_register (atoi (&name[1]), newval); */
/* OBSOLETE   else if (STREQ (name, "vm")) */
/* OBSOLETE     error ("can't assign to $vm"); */
/* OBSOLETE   else */
/* OBSOLETE     { */
/* OBSOLETE       offset /= bitsize / 8; */
/* OBSOLETE       write_vector_register (name[1] - '0', offset, newval); */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Print an integer value when no format was specified.  gdb normally */
/* OBSOLETE    prints these values in decimal, but the the leading 0x80000000 of */
/* OBSOLETE    pointers produces intolerable 10-digit negative numbers. */
/* OBSOLETE    If it looks like an address, print it in hex instead.  *x/ */
/* OBSOLETE  */
/* OBSOLETE decout (stream, type, val) */
/* OBSOLETE      struct ui_file *stream; */
/* OBSOLETE      struct type *type; */
/* OBSOLETE      LONGEST val; */
/* OBSOLETE { */
/* OBSOLETE   long lv = val; */
/* OBSOLETE  */
/* OBSOLETE   switch (output_radix) */
/* OBSOLETE     { */
/* OBSOLETE     case 0: */
/* OBSOLETE       if ((lv == val || (unsigned) lv == val) */
/* OBSOLETE       && ((lv & 0xf0000000) == 0x80000000 */
/* OBSOLETE           || ((lv & 0xf0000000) == 0xf0000000 && lv < STACK_END_ADDR))) */
/* OBSOLETE     { */
/* OBSOLETE       print_longest (stream, "x", 0, val); */
/* OBSOLETE       return; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE     case 10: */
/* OBSOLETE       print_longest (stream, TYPE_UNSIGNED (type) ? "u" : "d", 0, val); */
/* OBSOLETE       return; */
/* OBSOLETE  */
/* OBSOLETE     case 8: */
/* OBSOLETE       print_longest (stream, "o", 0, val); */
/* OBSOLETE       return; */
/* OBSOLETE  */
/* OBSOLETE     case 16: */
/* OBSOLETE       print_longest (stream, "x", 0, val); */
/* OBSOLETE       return; */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Change the default output radix to 10 or 16, or set it to 0 (heuristic). */
/* OBSOLETE    This command is mostly obsolete now that the print command allows */
/* OBSOLETE    formats to apply to aggregates, but is still handy occasionally.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE set_base_command (arg) */
/* OBSOLETE     char *arg; */
/* OBSOLETE { */
/* OBSOLETE   int new_radix; */
/* OBSOLETE  */
/* OBSOLETE   if (!arg) */
/* OBSOLETE     output_radix = 0; */
/* OBSOLETE   else */
/* OBSOLETE     { */
/* OBSOLETE       new_radix = atoi (arg); */
/* OBSOLETE       if (new_radix != 10 && new_radix != 16 && new_radix != 8)  */
/* OBSOLETE     error ("base must be 8, 10 or 16, or null"); */
/* OBSOLETE       else output_radix = new_radix; */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Turn pipelining on or off in the inferior. *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE set_pipelining_command (arg) */
/* OBSOLETE     char *arg; */
/* OBSOLETE { */
/* OBSOLETE   if (!arg) */
/* OBSOLETE     { */
/* OBSOLETE       sequential = !sequential; */
/* OBSOLETE       printf_filtered ("%s\n", sequential ? "off" : "on"); */
/* OBSOLETE     } */
/* OBSOLETE   else if (STREQ (arg, "on")) */
/* OBSOLETE     sequential = 0; */
/* OBSOLETE   else if (STREQ (arg, "off")) */
/* OBSOLETE     sequential = 1; */
/* OBSOLETE   else error ("valid args are `on', to allow instructions to overlap, or\n\ */
/* OBSOLETE `off', to prevent it and thereby pinpoint exceptions."); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Enable, disable, or force parallel execution in the inferior.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE set_parallel_command (arg) */
/* OBSOLETE      char *arg; */
/* OBSOLETE { */
/* OBSOLETE   struct rlimit rl; */
/* OBSOLETE   int prevparallel = parallel; */
/* OBSOLETE  */
/* OBSOLETE   if (!strncmp (arg, "fixed", strlen (arg))) */
/* OBSOLETE     parallel = 2;   */
/* OBSOLETE   else if (STREQ (arg, "on")) */
/* OBSOLETE     parallel = 1; */
/* OBSOLETE   else if (STREQ (arg, "off")) */
/* OBSOLETE     parallel = 0; */
/* OBSOLETE   else error ("valid args are `on', to allow multiple threads, or\n\ */
/* OBSOLETE `fixed', to force multiple threads, or\n\ */
/* OBSOLETE `off', to run with one thread only."); */
/* OBSOLETE  */
/* OBSOLETE   if ((prevparallel == 0) != (parallel == 0) && inferior_pid) */
/* OBSOLETE     printf_filtered ("will take effect at next run.\n"); */
/* OBSOLETE  */
/* OBSOLETE   getrlimit (RLIMIT_CONCUR, &rl); */
/* OBSOLETE   rl.rlim_cur = parallel ? rl.rlim_max : 1; */
/* OBSOLETE   setrlimit (RLIMIT_CONCUR, &rl); */
/* OBSOLETE  */
/* OBSOLETE   if (inferior_pid) */
/* OBSOLETE     set_fixed_scheduling (inferior_pid, parallel == 2); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Add a new name for an existing command.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void  */
/* OBSOLETE alias_command (arg) */
/* OBSOLETE     char *arg; */
/* OBSOLETE { */
/* OBSOLETE     static char *aliaserr = "usage is `alias NEW OLD', no args allowed"; */
/* OBSOLETE     char *newname = arg; */
/* OBSOLETE     struct cmd_list_element *new, *old; */
/* OBSOLETE  */
/* OBSOLETE     if (!arg) */
/* OBSOLETE       error_no_arg ("newname oldname"); */
/* OBSOLETE      */
/* OBSOLETE     new = lookup_cmd (&arg, cmdlist, "", -1); */
/* OBSOLETE     if (new && !strncmp (newname, new->name, strlen (new->name))) */
/* OBSOLETE       { */
/* OBSOLETE     newname = new->name; */
/* OBSOLETE     if (!(*arg == '-'  */
/* OBSOLETE           || (*arg >= 'a' && *arg <= 'z') */
/* OBSOLETE           || (*arg >= 'A' && *arg <= 'Z') */
/* OBSOLETE           || (*arg >= '0' && *arg <= '9'))) */
/* OBSOLETE       error (aliaserr); */
/* OBSOLETE       } */
/* OBSOLETE     else */
/* OBSOLETE       { */
/* OBSOLETE     arg = newname; */
/* OBSOLETE     while (*arg == '-'  */
/* OBSOLETE            || (*arg >= 'a' && *arg <= 'z') */
/* OBSOLETE            || (*arg >= 'A' && *arg <= 'Z') */
/* OBSOLETE            || (*arg >= '0' && *arg <= '9')) */
/* OBSOLETE       arg++; */
/* OBSOLETE     if (*arg != ' ' && *arg != '\t') */
/* OBSOLETE       error (aliaserr); */
/* OBSOLETE     *arg = '\0'; */
/* OBSOLETE     arg++; */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE     old = lookup_cmd (&arg, cmdlist, "", 0); */
/* OBSOLETE  */
/* OBSOLETE     if (*arg != '\0') */
/* OBSOLETE       error (aliaserr); */
/* OBSOLETE  */
/* OBSOLETE     if (new && !strncmp (newname, new->name, strlen (new->name))) */
/* OBSOLETE       { */
/* OBSOLETE     char *tem; */
/* OBSOLETE     if (new->class == (int) class_user || new->class == (int) class_alias) */
/* OBSOLETE       tem = "Redefine command \"%s\"? "; */
/* OBSOLETE     else */
/* OBSOLETE       tem = "Really redefine built-in command \"%s\"? "; */
/* OBSOLETE     if (!query (tem, new->name)) */
/* OBSOLETE       error ("Command \"%s\" not redefined.", new->name); */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE     add_com (newname, class_alias, old->function, old->doc); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Print the current thread number, and any threads with signals in the */
/* OBSOLETE    queue.  *x/ */
/* OBSOLETE  */
/* OBSOLETE thread_info () */
/* OBSOLETE { */
/* OBSOLETE   struct threadpid *p; */
/* OBSOLETE  */
/* OBSOLETE   if (have_inferior_p ()) */
/* OBSOLETE     { */
/* OBSOLETE       ps.pi_buffer = (char *) &comm_registers; */
/* OBSOLETE       ps.pi_nbytes = sizeof comm_registers; */
/* OBSOLETE       ps.pi_offset = 0; */
/* OBSOLETE       ps.pi_thread = inferior_thread; */
/* OBSOLETE       ioctl (inferior_fd, PIXRDCREGS, &ps); */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   /* FIXME: stop_signal is from target.h but stop_sigcode is a */
/* OBSOLETE      convex-specific thing.  *x/ */
/* OBSOLETE   printf_filtered ("Current thread %d stopped with signal %d.%d (%s).\n", */
/* OBSOLETE                inferior_thread, stop_signal, stop_sigcode, */
/* OBSOLETE                subsig_name (stop_signal, stop_sigcode)); */
/* OBSOLETE    */
/* OBSOLETE   for (p = signal_stack; p->pid; p--) */
/* OBSOLETE     printf_filtered ("Thread %d stopped with signal %d.%d (%s).\n", */
/* OBSOLETE                  p->thread, p->signo, p->subsig, */
/* OBSOLETE                  subsig_name (p->signo, p->subsig)); */
/* OBSOLETE              */
/* OBSOLETE   if (iscrlbit (comm_registers.crctl.lbits.cc, 64+13)) */
/* OBSOLETE     printf_filtered ("New thread start pc %#x\n", */
/* OBSOLETE                  (long) (comm_registers.crreg.pcpsw >> 32)); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Return string describing a signal.subcode number *x/ */
/* OBSOLETE  */
/* OBSOLETE static char * */
/* OBSOLETE subsig_name (signo, subcode) */
/* OBSOLETE      int signo, subcode; */
/* OBSOLETE { */
/* OBSOLETE   static char *subsig4[] = { */
/* OBSOLETE     "error exit", "privileged instruction", "unknown", */
/* OBSOLETE     "unknown", "undefined opcode", */
/* OBSOLETE     0}; */
/* OBSOLETE   static char *subsig5[] = {0, */
/* OBSOLETE     "breakpoint", "single step", "fork trap", "exec trap", "pfork trap", */
/* OBSOLETE     "join trap", "idle trap", "last thread", "wfork trap", */
/* OBSOLETE     "process breakpoint", "trap instruction", */
/* OBSOLETE     0}; */
/* OBSOLETE   static char *subsig8[] = {0, */
/* OBSOLETE     "int overflow", "int divide check", "float overflow", */
/* OBSOLETE     "float divide check", "float underflow", "reserved operand", */
/* OBSOLETE     "sqrt error", "exp error", "ln error", "sin error", "cos error", */
/* OBSOLETE     0}; */
/* OBSOLETE   static char *subsig10[] = {0, */
/* OBSOLETE     "invalid inward ring address", "invalid outward ring call", */
/* OBSOLETE     "invalid inward ring return", "invalid syscall gate", */
/* OBSOLETE     "invalid rtn frame length", "invalid comm reg address", */
/* OBSOLETE     "invalid trap gate", */
/* OBSOLETE     0}; */
/* OBSOLETE   static char *subsig11[] = {0, */
/* OBSOLETE     "read access denied", "write access denied", "execute access denied", */
/* OBSOLETE     "segment descriptor fault", "page table fault", "data reference fault", */
/* OBSOLETE     "i/o access denied", "levt pte invalid", */
/* OBSOLETE     0}; */
/* OBSOLETE  */
/* OBSOLETE   static char **subsig_list[] =  */
/* OBSOLETE     {0, 0, 0, 0, subsig4, subsig5, 0, 0, subsig8, 0, subsig10, subsig11, 0}; */
/* OBSOLETE  */
/* OBSOLETE   int i; */
/* OBSOLETE   char *p; */
/* OBSOLETE  */
/* OBSOLETE   if ((p = strsignal (signo)) == NULL) */
/* OBSOLETE     p = "unknown"; */
/* OBSOLETE   if (signo >= (sizeof subsig_list / sizeof *subsig_list) */
/* OBSOLETE       || !subsig_list[signo]) */
/* OBSOLETE     return p; */
/* OBSOLETE   for (i = 1; subsig_list[signo][i]; i++) */
/* OBSOLETE     if (i == subcode) */
/* OBSOLETE       return subsig_list[signo][subcode]; */
/* OBSOLETE   return p; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* Print a compact display of thread status, essentially x/i $pc */
/* OBSOLETE    for all active threads.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE threadstat () */
/* OBSOLETE { */
/* OBSOLETE   int t; */
/* OBSOLETE  */
/* OBSOLETE   for (t = 0; t < n_threads; t++) */
/* OBSOLETE     if (thread_state[t] == PI_TALIVE) */
/* OBSOLETE       { */
/* OBSOLETE     printf_filtered ("%d%c %08x%c %d.%d ", t, */
/* OBSOLETE                      (t == inferior_thread ? '*' : ' '), thread_pc[t], */
/* OBSOLETE                      (thread_is_in_kernel[t] ? '#' : ' '), */
/* OBSOLETE                      thread_signal[t], thread_sigcode[t]); */
/* OBSOLETE     print_insn (thread_pc[t], stdout); */
/* OBSOLETE     printf_filtered ("\n"); */
/* OBSOLETE       } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Change the current thread to ARG.  *x/ */
/* OBSOLETE  */
/* OBSOLETE set_thread_command (arg) */
/* OBSOLETE      char *arg; */
/* OBSOLETE { */
/* OBSOLETE     int thread; */
/* OBSOLETE  */
/* OBSOLETE     if (!arg) */
/* OBSOLETE       { */
/* OBSOLETE     threadstat (); */
/* OBSOLETE     return; */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE     thread = parse_and_eval_address (arg); */
/* OBSOLETE  */
/* OBSOLETE     if (thread < 0 || thread > n_threads || thread_state[thread] != PI_TALIVE) */
/* OBSOLETE       error ("no such thread."); */
/* OBSOLETE  */
/* OBSOLETE     select_thread (thread); */
/* OBSOLETE  */
/* OBSOLETE     stop_pc = read_pc (); */
/* OBSOLETE     flush_cached_frames (); */
/* OBSOLETE     select_frame (get_current_frame (), 0); */
/* OBSOLETE     print_stack_frame (selected_frame, selected_frame_level, -1); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Here on CONT command; gdb's dispatch address is changed to come here. */
/* OBSOLETE    Set global variable ALL_CONTINUE to tell resume() that it should */
/* OBSOLETE    start up all threads, and that a thread switch will not blow gdb's */
/* OBSOLETE    mind.  *x/ */
/* OBSOLETE  */
/* OBSOLETE static void */
/* OBSOLETE convex_cont_command (proc_count_exp, from_tty) */
/* OBSOLETE      char *proc_count_exp; */
/* OBSOLETE      int from_tty; */
/* OBSOLETE { */
/* OBSOLETE   all_continue = 1; */
/* OBSOLETE   cont_command (proc_count_exp, from_tty); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Here on 1CONT command.  Resume only the current thread.  *x/ */
/* OBSOLETE  */
/* OBSOLETE one_cont_command (proc_count_exp, from_tty) */
/* OBSOLETE      char *proc_count_exp; */
/* OBSOLETE      int from_tty; */
/* OBSOLETE { */
/* OBSOLETE   cont_command (proc_count_exp, from_tty); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Print the contents and lock bits of all communication registers, */
/* OBSOLETE    or just register ARG if ARG is a communication register, */
/* OBSOLETE    or the 3-word resource structure in memory at address ARG.  *x/ */
/* OBSOLETE  */
/* OBSOLETE comm_registers_info (arg) */
/* OBSOLETE     char *arg; */
/* OBSOLETE { */
/* OBSOLETE   int i, regnum; */
/* OBSOLETE  */
/* OBSOLETE   if (arg) */
/* OBSOLETE     { */
/* OBSOLETE              if (sscanf (arg, "$c%d", &regnum) == 1) { */
/* OBSOLETE     ; */
/* OBSOLETE       } else if (sscanf (arg, "$C%d", &regnum) == 1) { */
/* OBSOLETE     ; */
/* OBSOLETE       } else { */
/* OBSOLETE     regnum = parse_and_eval_address (arg); */
/* OBSOLETE     if (regnum > 0) */
/* OBSOLETE       regnum &= ~0x8000; */
/* OBSOLETE       } */
/* OBSOLETE  */
/* OBSOLETE       if (regnum >= 64) */
/* OBSOLETE     error ("%s: invalid register name.", arg); */
/* OBSOLETE  */
/* OBSOLETE       /* if we got a (user) address, examine the resource struct there *x/ */
/* OBSOLETE  */
/* OBSOLETE       if (regnum < 0) */
/* OBSOLETE     { */
/* OBSOLETE       static int buf[3]; */
/* OBSOLETE       read_memory (regnum, buf, sizeof buf); */
/* OBSOLETE       printf_filtered ("%08x  %08x%08x%s\n", regnum, buf[1], buf[2], */
/* OBSOLETE                        buf[0] & 0xff ? " locked" : ""); */
/* OBSOLETE       return; */
/* OBSOLETE     } */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   ps.pi_buffer = (char *) &comm_registers; */
/* OBSOLETE   ps.pi_nbytes = sizeof comm_registers; */
/* OBSOLETE   ps.pi_offset = 0; */
/* OBSOLETE   ps.pi_thread = inferior_thread; */
/* OBSOLETE   ioctl (inferior_fd, PIXRDCREGS, &ps); */
/* OBSOLETE  */
/* OBSOLETE   for (i = 0; i < 64; i++) */
/* OBSOLETE     if (!arg || i == regnum) */
/* OBSOLETE       printf_filtered ("%2d 0x8%03x %016llx%s\n", i, i, */
/* OBSOLETE                    comm_registers.crreg.r4[i], */
/* OBSOLETE                    (iscrlbit (comm_registers.crctl.lbits.cc, i) */
/* OBSOLETE                     ? " locked" : "")); */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE /* Print the psw *x/ */
/* OBSOLETE  */
/* OBSOLETE static void  */
/* OBSOLETE psw_info (arg) */
/* OBSOLETE     char *arg; */
/* OBSOLETE { */
/* OBSOLETE   struct pswbit */
/* OBSOLETE     { */
/* OBSOLETE       int bit; */
/* OBSOLETE       int pos; */
/* OBSOLETE       char *text; */
/* OBSOLETE     }; */
/* OBSOLETE  */
/* OBSOLETE   static struct pswbit pswbit[] = */
/* OBSOLETE     { */
/* OBSOLETE       { 0x80000000, -1, "A carry" },  */
/* OBSOLETE       { 0x40000000, -1, "A integer overflow" },  */
/* OBSOLETE       { 0x20000000, -1, "A zero divide" },  */
/* OBSOLETE       { 0x10000000, -1, "Integer overflow enable" },  */
/* OBSOLETE       { 0x08000000, -1, "Trace" },  */
/* OBSOLETE       { 0x06000000, 25, "Frame length" },  */
/* OBSOLETE       { 0x01000000, -1, "Sequential" },  */
/* OBSOLETE       { 0x00800000, -1, "S carry" },  */
/* OBSOLETE       { 0x00400000, -1, "S integer overflow" },  */
/* OBSOLETE       { 0x00200000, -1, "S zero divide" },  */
/* OBSOLETE       { 0x00100000, -1, "Zero divide enable" },  */
/* OBSOLETE       { 0x00080000, -1, "Floating underflow" },  */
/* OBSOLETE       { 0x00040000, -1, "Floating overflow" },  */
/* OBSOLETE       { 0x00020000, -1, "Floating reserved operand" },  */
/* OBSOLETE       { 0x00010000, -1, "Floating zero divide" },  */
/* OBSOLETE       { 0x00008000, -1, "Floating error enable" },  */
/* OBSOLETE       { 0x00004000, -1, "Floating underflow enable" },  */
/* OBSOLETE       { 0x00002000, -1, "IEEE" },  */
/* OBSOLETE       { 0x00001000, -1, "Sequential stores" },  */
/* OBSOLETE       { 0x00000800, -1, "Intrinsic error" },  */
/* OBSOLETE       { 0x00000400, -1, "Intrinsic error enable" },  */
/* OBSOLETE       { 0x00000200, -1, "Trace thread creates" },  */
/* OBSOLETE       { 0x00000100, -1, "Thread init trap" },  */
/* OBSOLETE       { 0x000000e0,  5, "Reserved" }, */
/* OBSOLETE       { 0x0000001f,  0, "Intrinsic error code" }, */
/* OBSOLETE       {0, 0, 0}, */
/* OBSOLETE     }; */
/* OBSOLETE  */
/* OBSOLETE   long psw; */
/* OBSOLETE   struct pswbit *p; */
/* OBSOLETE  */
/* OBSOLETE   if (arg) */
/* OBSOLETE     psw = parse_and_eval_address (arg); */
/* OBSOLETE   else */
/* OBSOLETE     psw = read_register (PS_REGNUM); */
/* OBSOLETE  */
/* OBSOLETE   for (p = pswbit; p->bit; p++) */
/* OBSOLETE     { */
/* OBSOLETE       if (p->pos < 0) */
/* OBSOLETE     printf_filtered ("%08x  %s  %s\n", p->bit, */
/* OBSOLETE                      (psw & p->bit) ? "yes" : "no ", p->text); */
/* OBSOLETE       else */
/* OBSOLETE     printf_filtered ("%08x %3d   %s\n", p->bit, */
/* OBSOLETE                      (psw & p->bit) >> p->pos, p->text); */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE #include "symtab.h" */
/* OBSOLETE  */
/* OBSOLETE /* reg (fmt_field, inst_field) -- */
/* OBSOLETE    the {first,second,third} operand of instruction as fmt_field = [ijk] */
/* OBSOLETE    gets the value of the field from the [ijk] position of the instruction *x/ */
/* OBSOLETE  */
/* OBSOLETE #define reg(a,b) ((char (*)[3])(op[fmt->a]))[inst.f0.b] */
/* OBSOLETE  */
/* OBSOLETE /* lit (fmt_field) -- field [ijk] is a literal (PSW, VL, eg) *x/ */
/* OBSOLETE  */
/* OBSOLETE #define lit(i) op[fmt->i] */
/* OBSOLETE  */
/* OBSOLETE /* aj[j] -- name for A register j *x/ */
/* OBSOLETE  */
/* OBSOLETE #define aj ((char (*)[3])(op[A])) */
/* OBSOLETE  */
/* OBSOLETE union inst { */
/* OBSOLETE     struct { */
/* OBSOLETE     unsigned   : 7; */
/* OBSOLETE     unsigned i : 3; */
/* OBSOLETE     unsigned j : 3; */
/* OBSOLETE     unsigned k : 3; */
/* OBSOLETE     unsigned   : 16; */
/* OBSOLETE     unsigned   : 32; */
/* OBSOLETE     } f0; */
/* OBSOLETE     struct { */
/* OBSOLETE     unsigned   : 8; */
/* OBSOLETE     unsigned indir : 1; */
/* OBSOLETE     unsigned len : 1; */
/* OBSOLETE     unsigned j : 3; */
/* OBSOLETE     unsigned k : 3; */
/* OBSOLETE     unsigned   : 16; */
/* OBSOLETE     unsigned   : 32; */
/* OBSOLETE     } f1; */
/* OBSOLETE     unsigned char byte[8]; */
/* OBSOLETE     unsigned short half[4]; */
/* OBSOLETE     char signed_byte[8]; */
/* OBSOLETE     short signed_half[4]; */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE struct opform { */
/* OBSOLETE     int mask;                       /* opcode mask *x/ */
/* OBSOLETE     int shift;                      /* opcode align *x/ */
/* OBSOLETE     struct formstr *formstr[3];     /* ST, E0, E1 *x/ */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE struct formstr { */
/* OBSOLETE     unsigned lop:8, rop:5;  /* opcode *x/ */
/* OBSOLETE     unsigned fmt:5;         /* inst format *x/ */
/* OBSOLETE     unsigned i:5, j:5, k:2; /* operand formats *x/ */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE #include "opcode/convex.h" */
/* OBSOLETE  */
/* OBSOLETE CONST unsigned char formdecode [] = { */
/* OBSOLETE     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, */
/* OBSOLETE     9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, */
/* OBSOLETE     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, */
/* OBSOLETE     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, */
/* OBSOLETE     2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, */
/* OBSOLETE     2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, */
/* OBSOLETE     3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, */
/* OBSOLETE     4,4,4,4,4,4,4,4,5,5,5,5,6,6,7,8, */
/* OBSOLETE     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, */
/* OBSOLETE     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, */
/* OBSOLETE     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, */
/* OBSOLETE     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, */
/* OBSOLETE     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, */
/* OBSOLETE     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, */
/* OBSOLETE     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, */
/* OBSOLETE     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE CONST struct opform opdecode[] = { */
/* OBSOLETE     0x7e00, 9, format0, e0_format0, e1_format0, */
/* OBSOLETE     0x3f00, 8, format1, e0_format1, e1_format1, */
/* OBSOLETE     0x1fc0, 6, format2, e0_format2, e1_format2, */
/* OBSOLETE     0x0fc0, 6, format3, e0_format3, e1_format3, */
/* OBSOLETE     0x0700, 8, format4, e0_format4, e1_format4, */
/* OBSOLETE     0x03c0, 6, format5, e0_format5, e1_format5, */
/* OBSOLETE     0x01f8, 3, format6, e0_format6, e1_format6, */
/* OBSOLETE     0x00f8, 3, format7, e0_format7, e1_format7, */
/* OBSOLETE     0x0000, 0, formatx, formatx, formatx, */
/* OBSOLETE     0x0f80, 7, formatx, formatx, formatx, */
/* OBSOLETE     0x0f80, 7, formatx, formatx, formatx, */
/* OBSOLETE }; */
/* OBSOLETE  */
/* OBSOLETE /* Print the instruction at address MEMADDR in debugged memory, */
/* OBSOLETE    on STREAM.  Returns length of the instruction, in bytes.  *x/ */
/* OBSOLETE  */
/* OBSOLETE int */
/* OBSOLETE convex_print_insn (memaddr, stream) */
/* OBSOLETE      CORE_ADDR memaddr; */
/* OBSOLETE      FILE *stream; */
/* OBSOLETE { */
/* OBSOLETE   union inst inst; */
/* OBSOLETE   struct formstr *fmt; */
/* OBSOLETE   register int format, op1, pfx; */
/* OBSOLETE   int l; */
/* OBSOLETE  */
/* OBSOLETE   read_memory (memaddr, &inst, sizeof inst); */
/* OBSOLETE  */
/* OBSOLETE   /* Remove and note prefix, if present *x/ */
/* OBSOLETE      */
/* OBSOLETE   pfx = inst.half[0]; */
/* OBSOLETE   if ((pfx & 0xfff0) == 0x7ef0) */
/* OBSOLETE     { */
/* OBSOLETE       pfx = ((pfx >> 3) & 1) + 1; */
/* OBSOLETE       *(long long *) &inst = *(long long *) &inst.half[1]; */
/* OBSOLETE     } */
/* OBSOLETE   else pfx = 0; */
/* OBSOLETE  */
/* OBSOLETE   /* Split opcode into format.op1 and look up in appropriate table *x/ */
/* OBSOLETE  */
/* OBSOLETE   format = formdecode[inst.byte[0]]; */
/* OBSOLETE   op1 = (inst.half[0] & opdecode[format].mask) >> opdecode[format].shift; */
/* OBSOLETE   if (format == 9) */
/* OBSOLETE     { */
/* OBSOLETE       if (pfx) */
/* OBSOLETE     fmt = formatx; */
/* OBSOLETE       else if (inst.f1.j == 0) */
/* OBSOLETE     fmt = &format1a[op1]; */
/* OBSOLETE       else if (inst.f1.j == 1) */
/* OBSOLETE     fmt = &format1b[op1]; */
/* OBSOLETE       else */
/* OBSOLETE     fmt = formatx; */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     fmt = &opdecode[format].formstr[pfx][op1]; */
/* OBSOLETE  */
/* OBSOLETE   /* Print it *x/ */
/* OBSOLETE  */
/* OBSOLETE   if (fmt->fmt == xxx) */
/* OBSOLETE     { */
/* OBSOLETE       /* noninstruction *x/ */
/* OBSOLETE       fprintf (stream, "0x%04x", pfx ? pfx : inst.half[0]); */
/* OBSOLETE       return 2; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   if (pfx) */
/* OBSOLETE     pfx = 2; */
/* OBSOLETE  */
/* OBSOLETE   fprintf (stream, "%s%s%s", lop[fmt->lop], rop[fmt->rop], */
/* OBSOLETE        &"        "[strlen(lop[fmt->lop]) + strlen(rop[fmt->rop])]); */
/* OBSOLETE  */
/* OBSOLETE   switch (fmt->fmt) */
/* OBSOLETE     { */
/* OBSOLETE     case rrr:                       /* three register *x/ */
/* OBSOLETE       fprintf (stream, "%s,%s,%s", reg(i,i), reg(j,j), reg(k,k)); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case rr:                        /* two register *x/ */
/* OBSOLETE       fprintf (stream, "%s,%s", reg(i,j), reg(j,k)); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case rxr:                       /* two register, reversed i and j fields *x/ */
/* OBSOLETE       fprintf (stream, "%s,%s", reg(i,k), reg(j,j)); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case r:                 /* one register *x/ */
/* OBSOLETE       fprintf (stream, "%s", reg(i,k)); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case nops:                      /* no operands *x/ */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case nr:                        /* short immediate, one register *x/ */
/* OBSOLETE       fprintf (stream, "#%d,%s", inst.f0.j, reg(i,k)); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case pcrel:                     /* pc relative *x/ */
/* OBSOLETE       print_address (memaddr + 2 * inst.signed_byte[1], stream); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case lr:                        /* literal, one register *x/ */
/* OBSOLETE       fprintf (stream, "%s,%s", lit(i), reg(j,k)); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case rxl:                       /* one register, literal *x/ */
/* OBSOLETE       fprintf (stream, "%s,%s", reg(i,k), lit(j)); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case rlr:                       /* register, literal, register *x/ */
/* OBSOLETE       fprintf (stream, "%s,%s,%s", reg(i,j), lit(j), reg(k,k)); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case rrl:                       /* register, register, literal *x/ */
/* OBSOLETE       fprintf (stream, "%s,%s,%s", reg(i,j), reg(j,k), lit(k)); */
/* OBSOLETE       return pfx + 2; */
/* OBSOLETE  */
/* OBSOLETE     case iml:                       /* immediate, literal *x/ */
/* OBSOLETE       if (inst.f1.len) */
/* OBSOLETE     { */
/* OBSOLETE       fprintf (stream, "#%#x,%s", */
/* OBSOLETE                (inst.signed_half[1] << 16) + inst.half[2], lit(i)); */
/* OBSOLETE       return pfx + 6; */
/* OBSOLETE     } */
/* OBSOLETE       else */
/* OBSOLETE     { */
/* OBSOLETE       fprintf (stream, "#%d,%s", inst.signed_half[1], lit(i)); */
/* OBSOLETE       return pfx + 4; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE     case imr:                       /* immediate, register *x/ */
/* OBSOLETE       if (inst.f1.len) */
/* OBSOLETE     { */
/* OBSOLETE       fprintf (stream, "#%#x,%s", */
/* OBSOLETE                (inst.signed_half[1] << 16) + inst.half[2], reg(i,k)); */
/* OBSOLETE       return pfx + 6; */
/* OBSOLETE     } */
/* OBSOLETE       else */
/* OBSOLETE     { */
/* OBSOLETE       fprintf (stream, "#%d,%s", inst.signed_half[1], reg(i,k)); */
/* OBSOLETE       return pfx + 4; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE     case a1r:                       /* memory, register *x/ */
/* OBSOLETE       l = print_effa (inst, stream); */
/* OBSOLETE       fprintf (stream, ",%s", reg(i,k)); */
/* OBSOLETE       return pfx + l; */
/* OBSOLETE  */
/* OBSOLETE     case a1l:                       /* memory, literal  *x/ */
/* OBSOLETE       l = print_effa (inst, stream); */
/* OBSOLETE       fprintf (stream, ",%s", lit(i)); */
/* OBSOLETE       return pfx + l; */
/* OBSOLETE  */
/* OBSOLETE     case a2r:                       /* register, memory *x/ */
/* OBSOLETE       fprintf (stream, "%s,", reg(i,k)); */
/* OBSOLETE       return pfx + print_effa (inst, stream); */
/* OBSOLETE  */
/* OBSOLETE     case a2l:                       /* literal, memory *x/ */
/* OBSOLETE       fprintf (stream, "%s,", lit(i)); */
/* OBSOLETE       return pfx + print_effa (inst, stream); */
/* OBSOLETE  */
/* OBSOLETE     case a3:                        /* memory *x/ */
/* OBSOLETE       return pfx + print_effa (inst, stream); */
/* OBSOLETE  */
/* OBSOLETE     case a4:                        /* system call *x/ */
/* OBSOLETE       l = 29; goto a4a5; */
/* OBSOLETE     case a5:                        /* trap *x/ */
/* OBSOLETE       l = 27; */
/* OBSOLETE     a4a5: */
/* OBSOLETE       if (inst.f1.len) */
/* OBSOLETE     { */
/* OBSOLETE       unsigned int m = (inst.signed_half[1] << 16) + inst.half[2]; */
/* OBSOLETE       fprintf (stream, "#%d,#%d", m >> l, m & (-1 >> (32-l))); */
/* OBSOLETE       return pfx + 6; */
/* OBSOLETE     } */
/* OBSOLETE       else */
/* OBSOLETE     { */
/* OBSOLETE       unsigned int m = inst.signed_half[1]; */
/* OBSOLETE       fprintf (stream, "#%d,#%d", m >> l, m & (-1 >> (32-l))); */
/* OBSOLETE       return pfx + 4; */
/* OBSOLETE     } */
/* OBSOLETE     } */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE /* print effective address @nnn(aj), return instruction length *x/ */
/* OBSOLETE  */
/* OBSOLETE int print_effa (inst, stream) */
/* OBSOLETE      union inst inst; */
/* OBSOLETE      FILE *stream; */
/* OBSOLETE { */
/* OBSOLETE   int n, l; */
/* OBSOLETE  */
/* OBSOLETE   if (inst.f1.len) */
/* OBSOLETE     { */
/* OBSOLETE       n = (inst.signed_half[1] << 16) + inst.half[2]; */
/* OBSOLETE       l = 6; */
/* OBSOLETE     } */
/* OBSOLETE   else */
/* OBSOLETE     { */
/* OBSOLETE       n = inst.signed_half[1]; */
/* OBSOLETE       l = 4; */
/* OBSOLETE     } */
/* OBSOLETE      */
/* OBSOLETE   if (inst.f1.indir) */
/* OBSOLETE     printf ("@"); */
/* OBSOLETE  */
/* OBSOLETE   if (!inst.f1.j) */
/* OBSOLETE     { */
/* OBSOLETE       print_address (n, stream); */
/* OBSOLETE       return l; */
/* OBSOLETE     } */
/* OBSOLETE  */
/* OBSOLETE   fprintf (stream, (n & 0xf0000000) == 0x80000000 ? "%#x(%s)" : "%d(%s)", */
/* OBSOLETE        n, aj[inst.f1.j]); */
/* OBSOLETE  */
/* OBSOLETE   return l; */
/* OBSOLETE } */
/* OBSOLETE  */
/* OBSOLETE  */
/* OBSOLETE void */
/* OBSOLETE _initialize_convex_dep () */
/* OBSOLETE { */
/* OBSOLETE   add_com ("alias", class_support, alias_command, */
/* OBSOLETE        "Add a new name for an existing command."); */
/* OBSOLETE  */
/* OBSOLETE   add_cmd ("base", class_vars, set_base_command, */
/* OBSOLETE        "Change the integer output radix to 8, 10 or 16\n\ */
/* OBSOLETE or use just `set base' with no args to return to the ad-hoc default,\n\ */
/* OBSOLETE which is 16 for integers that look like addresses, 10 otherwise.", */
/* OBSOLETE        &setlist); */
/* OBSOLETE  */
/* OBSOLETE   add_cmd ("pipeline", class_run, set_pipelining_command, */
/* OBSOLETE        "Enable or disable overlapped execution of instructions.\n\ */
/* OBSOLETE With `set pipe off', exceptions are reported with\n\ */
/* OBSOLETE $pc pointing at the instruction after the faulting one.\n\ */
/* OBSOLETE The default is `set pipe on', which runs faster.", */
/* OBSOLETE        &setlist); */
/* OBSOLETE  */
/* OBSOLETE   add_cmd ("parallel", class_run, set_parallel_command, */
/* OBSOLETE        "Enable or disable multi-threaded execution of parallel code.\n\ */
/* OBSOLETE `set parallel off' means run the program on a single CPU.\n\ */
/* OBSOLETE `set parallel fixed' means run the program with all CPUs assigned to it.\n\ */
/* OBSOLETE `set parallel on' means run the program on any CPUs that are available.", */
/* OBSOLETE        &setlist); */
/* OBSOLETE  */
/* OBSOLETE   add_com ("1cont", class_run, one_cont_command, */
/* OBSOLETE        "Continue the program, activating only the current thread.\n\ */
/* OBSOLETE Args are the same as the `cont' command."); */
/* OBSOLETE  */
/* OBSOLETE   add_com ("thread", class_run, set_thread_command, */
/* OBSOLETE        "Change the current thread, the one under scrutiny and control.\n\ */
/* OBSOLETE With no arg, show the active threads, the current one marked with *."); */
/* OBSOLETE  */
/* OBSOLETE   add_info ("threads", thread_info, */
/* OBSOLETE         "List status of active threads."); */
/* OBSOLETE  */
/* OBSOLETE   add_info ("comm-registers", comm_registers_info, */
/* OBSOLETE         "List communication registers and their contents.\n\ */
/* OBSOLETE A communication register name as argument means describe only that register.\n\ */
/* OBSOLETE An address as argument means describe the resource structure at that address.\n\ */
/* OBSOLETE `Locked' means that the register has been sent to but not yet received from."); */
/* OBSOLETE  */
/* OBSOLETE   add_info ("psw", psw_info,  */
/* OBSOLETE         "Display $ps, the processor status word, bit by bit.\n\ */
/* OBSOLETE An argument means display that value's interpretation as a psw."); */
/* OBSOLETE  */
/* OBSOLETE   add_cmd ("convex", no_class, 0, "Convex-specific commands.\n\ */
/* OBSOLETE 32-bit registers  $pc $ps $sp $ap $fp $a1-5 $s0-7 $v0-7 $vl $vs $vm $c0-63\n\ */
/* OBSOLETE 64-bit registers  $S0-7 $V0-7 $C0-63\n\ */
/* OBSOLETE \n\ */
/* OBSOLETE info threads            display info on stopped threads waiting to signal\n\ */
/* OBSOLETE thread                  display list of active threads\n\ */
/* OBSOLETE thread N        select thread N (its registers, stack, memory, etc.)\n\ */
/* OBSOLETE step, next, etc     step selected thread only\n\ */
/* OBSOLETE 1cont                   continue selected thread only\n\ */
/* OBSOLETE cont                    continue all threads\n\ */
/* OBSOLETE info comm-registers display contents of comm register(s) or a resource struct\n\ */
/* OBSOLETE info psw        display processor status word $ps\n\ */
/* OBSOLETE set base N      change integer radix used by `print' without a format\n\ */
/* OBSOLETE set pipeline off    exceptions are precise, $pc points after the faulting insn\n\ */
/* OBSOLETE set pipeline on     normal mode, $pc is somewhere ahead of faulting insn\n\ */
/* OBSOLETE set parallel off    program runs on a single CPU\n\ */
/* OBSOLETE set parallel fixed  all CPUs are assigned to the program\n\ */
/* OBSOLETE set parallel on     normal mode, parallel execution on random available CPUs\n\ */
/* OBSOLETE ", */
/* OBSOLETE        &cmdlist); */
/* OBSOLETE  */
/* OBSOLETE } */
