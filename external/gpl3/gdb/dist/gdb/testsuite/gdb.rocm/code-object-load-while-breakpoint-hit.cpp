/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifdef DEVICE

#include <hip/hip_runtime.h>

constexpr unsigned int NUM_BREAKPOINT_HITS = 5;

static __device__ void
break_here ()
{
}

extern "C" __global__ void
kernel ()
{
  for (int n = 0; n < NUM_BREAKPOINT_HITS; ++n)
    break_here ();
}

#else

#include <hip/hip_runtime.h>
#include <unistd.h>

constexpr unsigned int NUM_ITEMS_PER_BLOCK = 256;
constexpr unsigned int NUM_BLOCKS = 128;
constexpr unsigned int NUM_ITEMS = NUM_ITEMS_PER_BLOCK * NUM_BLOCKS;
constexpr unsigned int NUM_LOAD_UNLOADS = 5;

#define CHECK(cmd)                                                            \
  {                                                                           \
    hipError_t error = cmd;                                                   \
    if (error != hipSuccess)                                                  \
      {                                                                       \
	fprintf (stderr, "error: '%s'(%d) at %s:%d\n",                        \
		 hipGetErrorString (error), error, __FILE__, __LINE__);       \
	exit (EXIT_FAILURE);                                                  \
      }                                                                       \
  }

int
main (int argc, const char **argv)
{
  if (argc != 2)
    {
      fprintf (stderr, "Usage: %s <hip_module_path>\n", argv[0]);
      return 1;
    }

  const auto module_path = argv[1];
  hipModule_t module;
  CHECK (hipModuleLoad (&module, module_path));

  /* Launch the kernel.  */
  hipFunction_t function;
  CHECK (hipModuleGetFunction (&function, module, "kernel"));
  CHECK (hipModuleLaunchKernel (function, NUM_BLOCKS, 1, 1,
				NUM_ITEMS_PER_BLOCK, 1, 1, 0, nullptr, nullptr,
				nullptr));

  /* Load and unload the module many times.  */
  for (int i = 0; i < NUM_LOAD_UNLOADS; ++i)
    {
      hipModule_t dummy_module;
      CHECK (hipModuleLoad (&dummy_module, module_path));
      CHECK (hipModuleUnload (dummy_module));
    }
}

#endif
