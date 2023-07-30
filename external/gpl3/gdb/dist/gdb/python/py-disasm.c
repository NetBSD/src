/* Python interface to instruction disassembly.

   Copyright (C) 2021-2023 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include "defs.h"
#include "python-internal.h"
#include "dis-asm.h"
#include "arch-utils.h"
#include "charset.h"
#include "disasm.h"
#include "progspace.h"

/* Implement gdb.disassembler.DisassembleInfo type.  An object of this type
   represents a single disassembler request from GDB.  */

struct disasm_info_object
{
  PyObject_HEAD

  /* The architecture in which we are disassembling.  */
  struct gdbarch *gdbarch;

  /* The program_space in which we are disassembling.  */
  struct program_space *program_space;

  /* Address of the instruction to disassemble.  */
  bfd_vma address;

  /* The disassemble_info passed from core GDB, this contains the
     callbacks necessary to read the instruction from core GDB, and to
     print the disassembled instruction.  */
  disassemble_info *gdb_info;

  /* If copies of this object are created then they are chained together
     via this NEXT pointer, this allows all the copies to be invalidated at
     the same time as the parent object.  */
  struct disasm_info_object *next;
};

extern PyTypeObject disasm_info_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("disasm_info_object");

/* Implement gdb.disassembler.DisassemblerResult type, an object that holds
   the result of calling the disassembler.  This is mostly the length of
   the disassembled instruction (in bytes), and the string representing the
   disassembled instruction.  */

struct disasm_result_object
{
  PyObject_HEAD

  /* The length of the disassembled instruction in bytes.  */
  int length;

  /* A buffer which, when allocated, holds the disassembled content of an
     instruction.  */
  string_file *content;
};

extern PyTypeObject disasm_result_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("disasm_result_object");

/* When this is false we fast path out of gdbpy_print_insn, which should
   keep the performance impact of the Python disassembler down.  This is
   set to true from Python by calling gdb.disassembler._set_enabled() when
   the user registers a disassembler.  */

static bool python_print_insn_enabled = false;

/* A sub-class of gdb_disassembler that holds a pointer to a Python
   DisassembleInfo object.  A pointer to an instance of this class is
   placed in the application_data field of the disassemble_info that is
   used when we call gdbarch_print_insn.  */

struct gdbpy_disassembler : public gdb_printing_disassembler
{
  /* Constructor.  */
  gdbpy_disassembler (disasm_info_object *obj, PyObject *memory_source);

  /* Get the DisassembleInfo object pointer.  */
  disasm_info_object *
  py_disasm_info () const
  {
    return m_disasm_info_object;
  }

  /* Callbacks used by disassemble_info.  */
  static void memory_error_func (int status, bfd_vma memaddr,
				 struct disassemble_info *info) noexcept;
  static void print_address_func (bfd_vma addr,
				  struct disassemble_info *info) noexcept;
  static int read_memory_func (bfd_vma memaddr, gdb_byte *buff,
			       unsigned int len,
			       struct disassemble_info *info) noexcept;

  /* Return a reference to an optional that contains the address at which a
     memory error occurred.  The optional will only have a value if a
     memory error actually occurred.  */
  const gdb::optional<CORE_ADDR> &memory_error_address () const
  { return m_memory_error_address; }

  /* Return the content of the disassembler as a string.  The contents are
     moved out of the disassembler, so after this call the disassembler
     contents have been reset back to empty.  */
  std::string release ()
  {
    return m_string_file.release ();
  }

  /* If there is a Python exception stored in this disassembler then
     restore it (i.e. set the PyErr_* state), clear the exception within
     this disassembler, and return true.  There must be no current
     exception set (i.e. !PyErr_Occurred()) when this function is called,
     as any such exception might get lost.

     Otherwise, there is no exception stored in this disassembler, return
     false.  */
  bool restore_exception ()
  {
    gdb_assert (!PyErr_Occurred ());
    if (m_stored_exception.has_value ())
      {
	gdbpy_err_fetch ex = std::move (*m_stored_exception);
	m_stored_exception.reset ();
	ex.restore ();
	return true;
      }

    return false;
  }

private:

  /* Where the disassembler result is written.  */
  string_file m_string_file;

  /* The DisassembleInfo object we are disassembling for.  */
  disasm_info_object *m_disasm_info_object;

  /* When the user indicates that a memory error has occurred then the
     address of the memory error is stored in here.  */
  gdb::optional<CORE_ADDR> m_memory_error_address;

  /* When the user calls the builtin_disassemble function, if they pass a
     memory source object then a pointer to the object is placed in here,
     otherwise, this field is nullptr.  */
  PyObject *m_memory_source;

  /* Move the exception EX into this disassembler object.  */
  void store_exception (gdbpy_err_fetch &&ex)
  {
    /* The only calls to store_exception are from read_memory_func, which
       will return early if there's already an exception stored.  */
    gdb_assert (!m_stored_exception.has_value ());
    m_stored_exception.emplace (std::move (ex));
  }

  /* Return true if there is an exception stored in this disassembler.  */
  bool has_stored_exception () const
  {
    return m_stored_exception.has_value ();
  }

  /* Store a single exception.  This is used to pass Python exceptions back
     from ::memory_read to disasmpy_builtin_disassemble.  */
  gdb::optional<gdbpy_err_fetch> m_stored_exception;
};

/* Return true if OBJ is still valid, otherwise, return false.  A valid OBJ
   will have a non-nullptr gdb_info field.  */

static bool
disasm_info_object_is_valid (disasm_info_object *obj)
{
  return obj->gdb_info != nullptr;
}

/* Fill in OBJ with all the other arguments.  */

static void
disasm_info_fill (disasm_info_object *obj, struct gdbarch *gdbarch,
		  program_space *progspace, bfd_vma address,
		  disassemble_info *di, disasm_info_object *next)
{
  obj->gdbarch = gdbarch;
  obj->program_space = progspace;
  obj->address = address;
  obj->gdb_info = di;
  obj->next = next;
}

/* Implement DisassembleInfo.__init__.  Takes a single argument that must
   be another DisassembleInfo object and copies the contents from the
   argument into this new object.  */

static int
disasm_info_init (PyObject *self, PyObject *args, PyObject *kwargs)
{
  static const char *keywords[] = { "info", NULL };
  PyObject *info_obj;
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kwargs, "O!", keywords,
					&disasm_info_object_type,
					&info_obj))
    return -1;

  disasm_info_object *other = (disasm_info_object *) info_obj;
  disasm_info_object *info = (disasm_info_object *) self;
  disasm_info_fill (info, other->gdbarch, other->program_space,
		    other->address, other->gdb_info, other->next);
  other->next = info;

  /* As the OTHER object now holds a pointer to INFO we inc the ref count
     on INFO.  This stops INFO being deleted until OTHER has gone away.  */
  Py_INCREF ((PyObject *) info);
  return 0;
}

/* The tp_dealloc callback for the DisassembleInfo type.  */

static void
disasm_info_dealloc (PyObject *self)
{
  disasm_info_object *obj = (disasm_info_object *) self;

  /* We no longer care about the object our NEXT pointer points at, so we
     can decrement its reference count.  This macro handles the case when
     NEXT is nullptr.  */
  Py_XDECREF ((PyObject *) obj->next);

  /* Now core deallocation behaviour.  */
  Py_TYPE (self)->tp_free (self);
}

/* Implement DisassembleInfo.is_valid(), really just a wrapper around the
   disasm_info_object_is_valid function above.  */

static PyObject *
disasmpy_info_is_valid (PyObject *self, PyObject *args)
{
  disasm_info_object *disasm_obj = (disasm_info_object *) self;

  if (disasm_info_object_is_valid (disasm_obj))
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Set the Python exception to be a gdb.MemoryError object, with ADDRESS
   as its payload.  */

static void
disasmpy_set_memory_error_for_address (CORE_ADDR address)
{
  PyObject *address_obj = gdb_py_object_from_longest (address).release ();
  PyErr_SetObject (gdbpy_gdb_memory_error, address_obj);
}

/* Ensure that a gdb.disassembler.DisassembleInfo is valid.  */

#define DISASMPY_DISASM_INFO_REQUIRE_VALID(Info)			\
  do {									\
    if (!disasm_info_object_is_valid (Info))				\
      {									\
	PyErr_SetString (PyExc_RuntimeError,				\
			 _("DisassembleInfo is no longer valid."));	\
	return nullptr;							\
      }									\
  } while (0)

/* Initialise OBJ, a DisassemblerResult object with LENGTH and CONTENT.
   OBJ might already have been initialised, in which case any existing
   content should be discarded before the new CONTENT is moved in.  */

static void
disasmpy_init_disassembler_result (disasm_result_object *obj, int length,
				   std::string content)
{
  if (obj->content == nullptr)
    obj->content = new string_file;
  else
    obj->content->clear ();

  obj->length = length;
  *(obj->content) = std::move (content);
}

/* Implement gdb.disassembler.builtin_disassemble().  Calls back into GDB's
   builtin disassembler.  The first argument is a DisassembleInfo object
   describing what to disassemble.  The second argument is optional and
   provides a mechanism to modify the memory contents that the builtin
   disassembler will actually disassemble.

   Returns an instance of gdb.disassembler.DisassemblerResult, an object
   that wraps a disassembled instruction, or it raises a
   gdb.MemoryError.  */

static PyObject *
disasmpy_builtin_disassemble (PyObject *self, PyObject *args, PyObject *kw)
{
  PyObject *info_obj, *memory_source_obj = nullptr;
  static const char *keywords[] = { "info", "memory_source", nullptr };
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "O!|O", keywords,
					&disasm_info_object_type, &info_obj,
					&memory_source_obj))
    return nullptr;

  disasm_info_object *disasm_info = (disasm_info_object *) info_obj;
  DISASMPY_DISASM_INFO_REQUIRE_VALID (disasm_info);

  /* Where the result will be written.  */
  gdbpy_disassembler disassembler (disasm_info, memory_source_obj);

  /* Now actually perform the disassembly.  LENGTH is set to the length of
     the disassembled instruction, or -1 if there was a memory-error
     encountered while disassembling.  See below more more details on
     handling of -1 return value.  */
  int length = gdbarch_print_insn (disasm_info->gdbarch, disasm_info->address,
				   disassembler.disasm_info ());

  /* It is possible that, while calling a user overridden memory read
     function, a Python exception was raised that couldn't be
     translated into a standard memory-error.  In this case the first such
     exception is stored in the disassembler and restored here.  */
  if (disassembler.restore_exception ())
    return nullptr;

  if (length == -1)
    {

      /* In an ideal world, every disassembler should always call the
	 memory error function before returning a status of -1 as the only
	 error a disassembler should encounter is a failure to read
	 memory.  Unfortunately, there are some disassemblers who don't
	 follow this rule, and will return -1 without calling the memory
	 error function.

	 To make the Python API simpler, we just classify everything as a
	 memory error, but the message has to be modified for the case
	 where the disassembler didn't call the memory error function.  */
      if (disassembler.memory_error_address ().has_value ())
	{
	  CORE_ADDR addr = *disassembler.memory_error_address ();
	  disasmpy_set_memory_error_for_address (addr);
	}
      else
	{
	  std::string content = disassembler.release ();
	  if (!content.empty ())
	    PyErr_SetString (gdbpy_gdberror_exc, content.c_str ());
	  else
	    PyErr_SetString (gdbpy_gdberror_exc,
			     _("Unknown disassembly error."));
	}
      return nullptr;
    }

  /* Instructions are either non-zero in length, or we got an error,
     indicated by a length of -1, which we handled above.  */
  gdb_assert (length > 0);

  /* We should not have seen a memory error in this case.  */
  gdb_assert (!disassembler.memory_error_address ().has_value ());

  /* Create a DisassemblerResult containing the results.  */
  std::string content = disassembler.release ();
  PyTypeObject *type = &disasm_result_object_type;
  gdbpy_ref<disasm_result_object> res
    ((disasm_result_object *) type->tp_alloc (type, 0));
  disasmpy_init_disassembler_result (res.get (), length, std::move (content));
  return reinterpret_cast<PyObject *> (res.release ());
}

/* Implement gdb._set_enabled function.  Takes a boolean parameter, and
   sets whether GDB should enter the Python disassembler code or not.

   This is called from within the Python code when a new disassembler is
   registered.  When no disassemblers are registered the global C++ flag
   is set to false, and GDB never even enters the Python environment to
   check for a disassembler.

   When the user registers a new Python disassembler, the global C++ flag
   is set to true, and now GDB will enter the Python environment to check
   if there's a disassembler registered for the current architecture.  */

static PyObject *
disasmpy_set_enabled (PyObject *self, PyObject *args, PyObject *kw)
{
  PyObject *newstate;
  static const char *keywords[] = { "state", nullptr };
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "O", keywords,
					&newstate))
    return nullptr;

  if (!PyBool_Check (newstate))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("The value passed to `_set_enabled' must be a boolean."));
      return nullptr;
    }

  python_print_insn_enabled = PyObject_IsTrue (newstate);
  Py_RETURN_NONE;
}

/* Implement DisassembleInfo.read_memory(LENGTH, OFFSET).  Read LENGTH
   bytes at OFFSET from the start of the instruction currently being
   disassembled, and return a memory buffer containing the bytes.

   OFFSET defaults to zero if it is not provided.  LENGTH is required.  If
   the read fails then this will raise a gdb.MemoryError exception.  */

static PyObject *
disasmpy_info_read_memory (PyObject *self, PyObject *args, PyObject *kw)
{
  disasm_info_object *obj = (disasm_info_object *) self;
  DISASMPY_DISASM_INFO_REQUIRE_VALID (obj);

  LONGEST length, offset = 0;
  gdb::unique_xmalloc_ptr<gdb_byte> buffer;
  static const char *keywords[] = { "length", "offset", nullptr };

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "L|L", keywords,
					&length, &offset))
    return nullptr;

  /* The apparent address from which we are reading memory.  Note that in
     some cases GDB actually disassembles instructions from a buffer, so
     we might not actually be reading this information directly from the
     inferior memory.  This is all hidden behind the read_memory_func API
     within the disassemble_info structure.  */
  CORE_ADDR address = obj->address + offset;

  /* Setup a buffer to hold the result.  */
  buffer.reset ((gdb_byte *) xmalloc (length));

  /* Read content into BUFFER.  If the read fails then raise a memory
     error, otherwise, convert BUFFER to a Python memory buffer, and return
     it to the user.  */
  disassemble_info *info = obj->gdb_info;
  if (info->read_memory_func ((bfd_vma) address, buffer.get (),
			      (unsigned int) length, info) != 0)
    {
      disasmpy_set_memory_error_for_address (address);
      return nullptr;
    }
  return gdbpy_buffer_to_membuf (std::move (buffer), address, length);
}

/* Implement DisassembleInfo.address attribute, return the address at which
   GDB would like an instruction disassembled.  */

static PyObject *
disasmpy_info_address (PyObject *self, void *closure)
{
  disasm_info_object *obj = (disasm_info_object *) self;
  DISASMPY_DISASM_INFO_REQUIRE_VALID (obj);
  return gdb_py_object_from_longest (obj->address).release ();
}

/* Implement DisassembleInfo.architecture attribute.  Return the
   gdb.Architecture in which we are disassembling.  */

static PyObject *
disasmpy_info_architecture (PyObject *self, void *closure)
{
  disasm_info_object *obj = (disasm_info_object *) self;
  DISASMPY_DISASM_INFO_REQUIRE_VALID (obj);
  return gdbarch_to_arch_object (obj->gdbarch);
}

/* Implement DisassembleInfo.progspace attribute.  Return the
   gdb.Progspace in which we are disassembling.  */

static PyObject *
disasmpy_info_progspace (PyObject *self, void *closure)
{
  disasm_info_object *obj = (disasm_info_object *) self;
  DISASMPY_DISASM_INFO_REQUIRE_VALID (obj);
  return pspace_to_pspace_object (obj->program_space).release ();
}

/* This implements the disassemble_info read_memory_func callback and is
   called from the libopcodes disassembler when the disassembler wants to
   read memory.

   From the INFO argument we can find the gdbpy_disassembler object for
   which we are disassembling, and from that object we can find the
   DisassembleInfo for the current disassembly call.

   This function reads the instruction bytes by calling the read_memory
   method on the DisassembleInfo object.  This method might have been
   overridden by user code.

   Read LEN bytes from MEMADDR and place them into BUFF.  Return 0 on
   success (in which case BUFF has been filled), or -1 on error, in which
   case the contents of BUFF are undefined.  */

int
gdbpy_disassembler::read_memory_func (bfd_vma memaddr, gdb_byte *buff,
				      unsigned int len,
				      struct disassemble_info *info) noexcept
{
  gdbpy_disassembler *dis
    = static_cast<gdbpy_disassembler *> (info->application_data);
  disasm_info_object *obj = dis->py_disasm_info ();

  /* If a previous read attempt resulted in an exception, then we don't
     allow any further reads to succeed.  We only do this check for the
     read_memory_func as this is the only one the user can hook into,
     thus, this check prevents us calling back into user code if a
     previous call has already thrown an error.  */
  if (dis->has_stored_exception ())
    return -1;

  /* The DisassembleInfo.read_memory method expects an offset from the
     address stored within the DisassembleInfo object; calculate that
     offset here.  */
  LONGEST offset = (LONGEST) memaddr - (LONGEST) obj->address;

  /* Now call the DisassembleInfo.read_memory method.  This might have been
     overridden by the user.  */
  gdbpy_ref<> result_obj (PyObject_CallMethod ((PyObject *) obj,
					       "read_memory",
					       "KL", len, offset));

  /* Handle any exceptions.  */
  if (result_obj == nullptr)
    {
      /* If we got a gdb.MemoryError then we ignore this and just report
	 that the read failed to the caller.  The caller is then
	 responsible for calling the memory_error_func if it wants to.
	 Remember, the disassembler might just be probing to see if these
	 bytes can be read, if we automatically call the memory error
	 function, we can end up registering an error prematurely.  */
      if (PyErr_ExceptionMatches (gdbpy_gdb_memory_error))
	{
	  PyErr_Clear ();
	  return -1;
	}

      /* For any other exception type we capture the value of the Python
	 exception and throw it, this will then be caught in
	 disasmpy_builtin_disassemble, at which point the exception will be
	 restored.  */
      dis->store_exception (gdbpy_err_fetch ());
      return -1;
    }

  /* Convert the result to a buffer.  */
  Py_buffer py_buff;
  if (!PyObject_CheckBuffer (result_obj.get ())
      || PyObject_GetBuffer (result_obj.get(), &py_buff, PyBUF_CONTIG_RO) < 0)
    {
      PyErr_Format (PyExc_TypeError,
		    _("Result from read_memory is not a buffer"));
      dis->store_exception (gdbpy_err_fetch ());
      return -1;
    }

  /* Wrap PY_BUFF so that it is cleaned up correctly at the end of this
     scope.  */
  Py_buffer_up buffer_up (&py_buff);

  /* Validate that the buffer is the correct length.  */
  if (py_buff.len != len)
    {
      PyErr_Format (PyExc_ValueError,
		    _("Buffer returned from read_memory is sized %d instead of the expected %d"),
		    py_buff.len, len);
      dis->store_exception (gdbpy_err_fetch ());
      return -1;
    }

  /* Copy the data out of the Python buffer and return success.  */
  const gdb_byte *buffer = (const gdb_byte *) py_buff.buf;
  memcpy (buff, buffer, len);
  return 0;
}

/* Implement DisassemblerResult.length attribute, return the length of the
   disassembled instruction.  */

static PyObject *
disasmpy_result_length (PyObject *self, void *closure)
{
  disasm_result_object *obj = (disasm_result_object *) self;
  return gdb_py_object_from_longest (obj->length).release ();
}

/* Implement DisassemblerResult.string attribute, return the content string
   of the disassembled instruction.  */

static PyObject *
disasmpy_result_string (PyObject *self, void *closure)
{
  disasm_result_object *obj = (disasm_result_object *) self;

  gdb_assert (obj->content != nullptr);
  gdb_assert (strlen (obj->content->c_str ()) > 0);
  gdb_assert (obj->length > 0);
  return PyUnicode_Decode (obj->content->c_str (),
			   obj->content->size (),
			   host_charset (), nullptr);
}

/* Implement DisassemblerResult.__init__.  Takes two arguments, an
   integer, the length in bytes of the disassembled instruction, and a
   string, the disassembled content of the instruction.  */

static int
disasmpy_result_init (PyObject *self, PyObject *args, PyObject *kwargs)
{
  static const char *keywords[] = { "length", "string", NULL };
  int length;
  const char *string;
  if (!gdb_PyArg_ParseTupleAndKeywords (args, kwargs, "is", keywords,
					&length, &string))
    return -1;

  if (length <= 0)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("Length must be greater than 0."));
      return -1;
    }

  if (strlen (string) == 0)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("String must not be empty."));
      return -1;
    }

  disasm_result_object *obj = (disasm_result_object *) self;
  disasmpy_init_disassembler_result (obj, length, std::string (string));

  return 0;
}

/* Implement memory_error_func callback for disassemble_info.  Extract the
   underlying DisassembleInfo Python object, and set a memory error on
   it.  */

void
gdbpy_disassembler::memory_error_func (int status, bfd_vma memaddr,
				       struct disassemble_info *info) noexcept
{
  gdbpy_disassembler *dis
    = static_cast<gdbpy_disassembler *> (info->application_data);
  dis->m_memory_error_address.emplace (memaddr);
}

/* Wrapper of print_address.  */

void
gdbpy_disassembler::print_address_func (bfd_vma addr,
					struct disassemble_info *info) noexcept
{
  gdbpy_disassembler *dis
    = static_cast<gdbpy_disassembler *> (info->application_data);
  print_address (dis->arch (), addr, dis->stream ());
}

/* constructor.  */

gdbpy_disassembler::gdbpy_disassembler (disasm_info_object *obj,
					PyObject *memory_source)
  : gdb_printing_disassembler (obj->gdbarch, &m_string_file,
			       read_memory_func, memory_error_func,
			       print_address_func),
    m_disasm_info_object (obj),
    m_memory_source (memory_source)
{ /* Nothing.  */ }

/* A wrapper around a reference to a Python DisassembleInfo object, which
   ensures that the object is marked as invalid when we leave the enclosing
   scope.

   Each DisassembleInfo is created in gdbpy_print_insn, and is done with by
   the time that function returns.  However, there's nothing to stop a user
   caching a reference to the DisassembleInfo, and thus keeping the object
   around.

   We therefore have the notion of a DisassembleInfo becoming invalid, this
   happens when gdbpy_print_insn returns.  This class is responsible for
   marking the DisassembleInfo as invalid in its destructor.  */

struct scoped_disasm_info_object
{
  /* Constructor.  */
  scoped_disasm_info_object (struct gdbarch *gdbarch, CORE_ADDR memaddr,
			     disassemble_info *info)
    : m_disasm_info (allocate_disasm_info_object ())
  {
    disasm_info_fill (m_disasm_info.get (), gdbarch, current_program_space,
		      memaddr, info, nullptr);
  }

  /* Upon destruction mark m_diasm_info as invalid.  */
  ~scoped_disasm_info_object ()
  {
    /* Invalidate the original DisassembleInfo object as well as any copies
       that the user might have made.  */
    for (disasm_info_object *obj = m_disasm_info.get ();
	 obj != nullptr;
	 obj = obj->next)
      obj->gdb_info = nullptr;
  }

  /* Return a pointer to the underlying disasm_info_object instance.  */
  disasm_info_object *
  get () const
  {
    return m_disasm_info.get ();
  }

private:

  /* Wrapper around the call to PyObject_New, this wrapper function can be
     called from the constructor initialization list, while PyObject_New, a
     macro, can't.  */
  static disasm_info_object *
  allocate_disasm_info_object ()
  {
    return (disasm_info_object *) PyObject_New (disasm_info_object,
						&disasm_info_object_type);
  }

  /* A reference to a gdb.disassembler.DisassembleInfo object.  When this
     containing instance goes out of scope this reference is released,
     however, the user might be holding other references to the
     DisassembleInfo object in Python code, so the underlying object might
     not be deleted.  */
  gdbpy_ref<disasm_info_object> m_disasm_info;
};

/* See python-internal.h.  */

gdb::optional<int>
gdbpy_print_insn (struct gdbarch *gdbarch, CORE_ADDR memaddr,
		  disassemble_info *info)
{
  /* Early exit case.  This must be done as early as possible, and
     definitely before we enter Python environment.  The
     python_print_insn_enabled flag is set (from Python) only when the user
     has installed one (or more) Python disassemblers.  So in the common
     case (no custom disassembler installed) this flag will be false,
     allowing for a quick return.  */
  if (!gdb_python_initialized || !python_print_insn_enabled)
    return {};

  gdbpy_enter enter_py (get_current_arch (), current_language);

  /* Import the gdb.disassembler module.  */
  gdbpy_ref<> gdb_python_disassembler_module
    (PyImport_ImportModule ("gdb.disassembler"));
  if (gdb_python_disassembler_module == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  /* Get the _print_insn attribute from the module, this should be the
     function we are going to call to actually perform the disassembly.  */
  gdbpy_ref<> hook
    (PyObject_GetAttrString (gdb_python_disassembler_module.get (),
			     "_print_insn"));
  if (hook == nullptr)
    {
      gdbpy_print_stack ();
      return {};
    }

  /* Create the new DisassembleInfo object we will pass into Python.  This
     object will be marked as invalid when we leave this scope.  */
  scoped_disasm_info_object scoped_disasm_info (gdbarch, memaddr, info);
  disasm_info_object *disasm_info = scoped_disasm_info.get ();

  /* Call into the registered disassembler to (possibly) perform the
     disassembly.  */
  PyObject *insn_disas_obj = (PyObject *) disasm_info;
  gdbpy_ref<> result (PyObject_CallFunctionObjArgs (hook.get (),
						    insn_disas_obj,
						    nullptr));

  if (result == nullptr)
    {
      /* The call into Python code resulted in an exception.  If this was a
	 gdb.MemoryError, then we can figure out an address and call the
	 disassemble_info::memory_error_func to report the error back to
	 core GDB.  Any other exception type we report back to core GDB as
	 an unknown error (return -1 without first calling the
	 memory_error_func callback).  */

      if (PyErr_ExceptionMatches (gdbpy_gdb_memory_error))
	{
	  /* A gdb.MemoryError might have an address attribute which
	     contains the address at which the memory error occurred.  If
	     this is the case then use this address, otherwise, fallback to
	     just using the address of the instruction we were asked to
	     disassemble.  */
	  gdbpy_err_fetch err;
	  PyErr_Clear ();

	  CORE_ADDR addr;
	  if (err.value () != nullptr
	      && PyObject_HasAttrString (err.value ().get (), "address"))
	    {
	      PyObject *addr_obj
		= PyObject_GetAttrString (err.value ().get (), "address");
	      if (get_addr_from_python (addr_obj, &addr) < 0)
		addr = disasm_info->address;
	    }
	  else
	    addr = disasm_info->address;

	  info->memory_error_func (-1, addr, info);
	  return gdb::optional<int> (-1);
	}
      else if (PyErr_ExceptionMatches (gdbpy_gdberror_exc))
	{
	  gdbpy_err_fetch err;
	  gdb::unique_xmalloc_ptr<char> msg = err.to_string ();

	  info->fprintf_func (info->stream, "%s", msg.get ());
	  return gdb::optional<int> (-1);
	}
      else
	{
	  gdbpy_print_stack ();
	  return gdb::optional<int> (-1);
	}

    }
  else if (result == Py_None)
    {
      /* A return value of None indicates that the Python code could not,
	 or doesn't want to, disassemble this instruction.  Just return an
	 empty result and core GDB will try to disassemble this for us.  */
      return {};
    }

  /* Check the result is a DisassemblerResult (or a sub-class).  */
  if (!PyObject_IsInstance (result.get (),
			    (PyObject *) &disasm_result_object_type))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Result is not a DisassemblerResult."));
      gdbpy_print_stack ();
      return gdb::optional<int> (-1);
    }

  /* The call into Python neither raised an exception, or returned None.
     Check to see if the result looks valid.  */
  gdbpy_ref<> length_obj (PyObject_GetAttrString (result.get (), "length"));
  if (length_obj == nullptr)
    {
      gdbpy_print_stack ();
      return gdb::optional<int> (-1);
    }

  gdbpy_ref<> string_obj (PyObject_GetAttrString (result.get (), "string"));
  if (string_obj == nullptr)
    {
      gdbpy_print_stack ();
      return gdb::optional<int> (-1);
    }
  if (!gdbpy_is_string (string_obj.get ()))
    {
      PyErr_SetString (PyExc_TypeError, _("String attribute is not a string."));
      gdbpy_print_stack ();
      return gdb::optional<int> (-1);
    }

  gdb::unique_xmalloc_ptr<char> string
    = gdbpy_obj_to_string (string_obj.get ());
  if (string == nullptr)
    {
      gdbpy_print_stack ();
      return gdb::optional<int> (-1);
    }

  long length;
  if (!gdb_py_int_as_long (length_obj.get (), &length))
    {
      gdbpy_print_stack ();
      return gdb::optional<int> (-1);
    }

  long max_insn_length = (gdbarch_max_insn_length_p (gdbarch) ?
			  gdbarch_max_insn_length (gdbarch) : INT_MAX);
  if (length <= 0)
    {
      PyErr_SetString
	(PyExc_ValueError,
	 _("Invalid length attribute: length must be greater than 0."));
      gdbpy_print_stack ();
      return gdb::optional<int> (-1);
    }
  if (length > max_insn_length)
    {
      PyErr_Format
	(PyExc_ValueError,
	 _("Invalid length attribute: length %d greater than architecture maximum of %d"),
	 length, max_insn_length);
      gdbpy_print_stack ();
      return gdb::optional<int> (-1);
    }

  if (strlen (string.get ()) == 0)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("String attribute must not be empty."));
      gdbpy_print_stack ();
      return gdb::optional<int> (-1);
    }

  /* Print the disassembled instruction back to core GDB, and return the
     length of the disassembled instruction.  */
  info->fprintf_func (info->stream, "%s", string.get ());
  return gdb::optional<int> (length);
}

/* The tp_dealloc callback for the DisassemblerResult type.  Takes care of
   deallocating the content buffer.  */

static void
disasmpy_dealloc_result (PyObject *self)
{
  disasm_result_object *obj = (disasm_result_object *) self;
  delete obj->content;
  Py_TYPE (self)->tp_free (self);
}

/* The get/set attributes of the gdb.disassembler.DisassembleInfo type.  */

static gdb_PyGetSetDef disasm_info_object_getset[] = {
  { "address", disasmpy_info_address, nullptr,
    "Start address of the instruction to disassemble.", nullptr },
  { "architecture", disasmpy_info_architecture, nullptr,
    "Architecture to disassemble in", nullptr },
  { "progspace", disasmpy_info_progspace, nullptr,
    "Program space to disassemble in", nullptr },
  { nullptr }   /* Sentinel */
};

/* The methods of the gdb.disassembler.DisassembleInfo type.  */

static PyMethodDef disasm_info_object_methods[] = {
  { "read_memory", (PyCFunction) disasmpy_info_read_memory,
    METH_VARARGS | METH_KEYWORDS,
    "read_memory (LEN, OFFSET = 0) -> Octets[]\n\
Read LEN octets for the instruction to disassemble." },
  { "is_valid", disasmpy_info_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this DisassembleInfo is valid, false if not." },
  {nullptr}  /* Sentinel */
};

/* The get/set attributes of the gdb.disassembler.DisassemblerResult type.  */

static gdb_PyGetSetDef disasm_result_object_getset[] = {
  { "length", disasmpy_result_length, nullptr,
    "Length of the disassembled instruction.", nullptr },
  { "string", disasmpy_result_string, nullptr,
    "String representing the disassembled instruction.", nullptr },
  { nullptr }   /* Sentinel */
};

/* These are the methods we add into the _gdb.disassembler module, which
   are then imported into the gdb.disassembler module.  These are global
   functions that support performing disassembly.  */

PyMethodDef python_disassembler_methods[] =
{
  { "builtin_disassemble", (PyCFunction) disasmpy_builtin_disassemble,
    METH_VARARGS | METH_KEYWORDS,
    "builtin_disassemble (INFO, MEMORY_SOURCE = None) -> None\n\
Disassemble using GDB's builtin disassembler.  INFO is an instance of\n\
gdb.disassembler.DisassembleInfo.  The MEMORY_SOURCE, if not None, should\n\
be an object with the read_memory method." },
  { "_set_enabled", (PyCFunction) disasmpy_set_enabled,
    METH_VARARGS | METH_KEYWORDS,
    "_set_enabled (STATE) -> None\n\
Set whether GDB should call into the Python _print_insn code or not." },
  {nullptr, nullptr, 0, nullptr}
};

/* Structure to define the _gdb.disassembler module.  */

static struct PyModuleDef python_disassembler_module_def =
{
  PyModuleDef_HEAD_INIT,
  "_gdb.disassembler",
  nullptr,
  -1,
  python_disassembler_methods,
  nullptr,
  nullptr,
  nullptr,
  nullptr
};

/* Called to initialize the Python structures in this file.  */

int
gdbpy_initialize_disasm ()
{
  /* Create the _gdb.disassembler module, and add it to the _gdb module.  */

  PyObject *gdb_disassembler_module;
  gdb_disassembler_module = PyModule_Create (&python_disassembler_module_def);
  if (gdb_disassembler_module == nullptr)
    return -1;
  PyModule_AddObject(gdb_module, "disassembler", gdb_disassembler_module);

  /* This is needed so that 'import _gdb.disassembler' will work.  */
  PyObject *dict = PyImport_GetModuleDict ();
  PyDict_SetItemString (dict, "_gdb.disassembler", gdb_disassembler_module);

  disasm_info_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&disasm_info_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_disassembler_module, "DisassembleInfo",
			      (PyObject *) &disasm_info_object_type) < 0)
    return -1;

  disasm_result_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&disasm_result_object_type) < 0)
    return -1;

  if (gdb_pymodule_addobject (gdb_disassembler_module, "DisassemblerResult",
			      (PyObject *) &disasm_result_object_type) < 0)
    return -1;

  return 0;
}

/* Describe the gdb.disassembler.DisassembleInfo type.  */

PyTypeObject disasm_info_object_type = {
  PyVarObject_HEAD_INIT (nullptr, 0)
  "gdb.disassembler.DisassembleInfo",		/*tp_name*/
  sizeof (disasm_info_object),			/*tp_basicsize*/
  0,						/*tp_itemsize*/
  disasm_info_dealloc,				/*tp_dealloc*/
  0,						/*tp_print*/
  0,						/*tp_getattr*/
  0,						/*tp_setattr*/
  0,						/*tp_compare*/
  0,						/*tp_repr*/
  0,						/*tp_as_number*/
  0,						/*tp_as_sequence*/
  0,						/*tp_as_mapping*/
  0,						/*tp_hash */
  0,						/*tp_call*/
  0,						/*tp_str*/
  0,						/*tp_getattro*/
  0,						/*tp_setattro*/
  0,						/*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/*tp_flags*/
  "GDB instruction disassembler object",	/* tp_doc */
  0,						/* tp_traverse */
  0,						/* tp_clear */
  0,						/* tp_richcompare */
  0,						/* tp_weaklistoffset */
  0,						/* tp_iter */
  0,						/* tp_iternext */
  disasm_info_object_methods,			/* tp_methods */
  0,						/* tp_members */
  disasm_info_object_getset,			/* tp_getset */
  0,						/* tp_base */
  0,						/* tp_dict */
  0,						/* tp_descr_get */
  0,						/* tp_descr_set */
  0,						/* tp_dictoffset */
  disasm_info_init,				/* tp_init */
  0,						/* tp_alloc */
};

/* Describe the gdb.disassembler.DisassemblerResult type.  */

PyTypeObject disasm_result_object_type = {
  PyVarObject_HEAD_INIT (nullptr, 0)
  "gdb.disassembler.DisassemblerResult",	/*tp_name*/
  sizeof (disasm_result_object),		/*tp_basicsize*/
  0,						/*tp_itemsize*/
  disasmpy_dealloc_result,			/*tp_dealloc*/
  0,						/*tp_print*/
  0,						/*tp_getattr*/
  0,						/*tp_setattr*/
  0,						/*tp_compare*/
  0,						/*tp_repr*/
  0,						/*tp_as_number*/
  0,						/*tp_as_sequence*/
  0,						/*tp_as_mapping*/
  0,						/*tp_hash */
  0,						/*tp_call*/
  0,						/*tp_str*/
  0,						/*tp_getattro*/
  0,						/*tp_setattro*/
  0,						/*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/*tp_flags*/
  "GDB object, representing a disassembler result",	/* tp_doc */
  0,						/* tp_traverse */
  0,						/* tp_clear */
  0,						/* tp_richcompare */
  0,						/* tp_weaklistoffset */
  0,						/* tp_iter */
  0,						/* tp_iternext */
  0,						/* tp_methods */
  0,						/* tp_members */
  disasm_result_object_getset,			/* tp_getset */
  0,						/* tp_base */
  0,						/* tp_dict */
  0,						/* tp_descr_get */
  0,						/* tp_descr_set */
  0,						/* tp_dictoffset */
  disasmpy_result_init,				/* tp_init */
  0,						/* tp_alloc */
};
