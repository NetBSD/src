/* Python interface to blocks.

   Copyright (C) 2008-2013 Free Software Foundation, Inc.

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
#include "block.h"
#include "dictionary.h"
#include "symtab.h"
#include "python-internal.h"
#include "objfiles.h"
#include "symtab.h"

typedef struct blpy_block_object {
  PyObject_HEAD
  /* The GDB block structure that represents a frame's code block.  */
  const struct block *block;
  /* The backing object file.  There is no direct relationship in GDB
     between a block and an object file.  When a block is created also
     store a pointer to the object file for later use.  */
  struct objfile *objfile;
  /* Keep track of all blocks with a doubly-linked list.  Needed for
     block invalidation if the source object file has been freed.  */
  struct blpy_block_object *prev;
  struct blpy_block_object *next;
} block_object;

typedef struct {
  PyObject_HEAD
  /* The block.  */
  const struct block *block;
  /* The iterator for that block.  */
  struct block_iterator iter;
  /* Has the iterator been initialized flag.  */
  int initialized_p;
  /* Pointer back to the original source block object.  Needed to
     check if the block is still valid, and has not been invalidated
     when an object file has been freed.  */
  struct blpy_block_object *source;
} block_syms_iterator_object;

/* Require a valid block.  All access to block_object->block should be
   gated by this call.  */
#define BLPY_REQUIRE_VALID(block_obj, block)		\
  do {							\
    block = block_object_to_block (block_obj);		\
    if (block == NULL)					\
      {							\
	PyErr_SetString (PyExc_RuntimeError,		\
			 _("Block is invalid."));	\
	return NULL;					\
      }							\
  } while (0)

/* Require a valid block.  This macro is called during block iterator
   creation, and at each next call.  */
#define BLPY_ITER_REQUIRE_VALID(block_obj)				\
  do {									\
    if (block_obj->block == NULL)					\
      {									\
	PyErr_SetString (PyExc_RuntimeError,				\
			 _("Source block for iterator is invalid."));	\
	return NULL;							\
      }									\
  } while (0)

static PyTypeObject block_syms_iterator_object_type;
static const struct objfile_data *blpy_objfile_data_key;

static PyObject *
blpy_iter (PyObject *self)
{
  block_syms_iterator_object *block_iter_obj;
  const struct block *block = NULL;

  BLPY_REQUIRE_VALID (self, block);

  block_iter_obj = PyObject_New (block_syms_iterator_object,
				 &block_syms_iterator_object_type);
  if (block_iter_obj == NULL)
      return NULL;

  block_iter_obj->block = block;
  block_iter_obj->initialized_p = 0;
  Py_INCREF (self);
  block_iter_obj->source = (block_object *) self;

  return (PyObject *) block_iter_obj;
}

static PyObject *
blpy_get_start (PyObject *self, void *closure)
{
  const struct block *block = NULL;

  BLPY_REQUIRE_VALID (self, block);

  return gdb_py_object_from_ulongest (BLOCK_START (block));
}

static PyObject *
blpy_get_end (PyObject *self, void *closure)
{
  const struct block *block = NULL;

  BLPY_REQUIRE_VALID (self, block);

  return gdb_py_object_from_ulongest (BLOCK_END (block));
}

static PyObject *
blpy_get_function (PyObject *self, void *closure)
{
  struct symbol *sym;
  const struct block *block;

  BLPY_REQUIRE_VALID (self, block);

  sym = BLOCK_FUNCTION (block);
  if (sym)
    return symbol_to_symbol_object (sym);

  Py_RETURN_NONE;
}

static PyObject *
blpy_get_superblock (PyObject *self, void *closure)
{
  const struct block *block;
  const struct block *super_block;
  block_object *self_obj  = (block_object *) self;

  BLPY_REQUIRE_VALID (self, block);

  super_block = BLOCK_SUPERBLOCK (block);
  if (super_block)
    return block_to_block_object (super_block, self_obj->objfile);

  Py_RETURN_NONE;
}

/* Return the global block associated to this block.  */

static PyObject *
blpy_get_global_block (PyObject *self, void *closure)
{
  const struct block *block;
  const struct block *global_block;
  block_object *self_obj  = (block_object *) self;

  BLPY_REQUIRE_VALID (self, block);

  global_block = block_global_block (block);

  return block_to_block_object (global_block,
				self_obj->objfile);

}

/* Return the static block associated to this block.  Return None
   if we cannot get the static block (this is the global block).  */

static PyObject *
blpy_get_static_block (PyObject *self, void *closure)
{
  const struct block *block;
  const struct block *static_block;
  block_object *self_obj  = (block_object *) self;

  BLPY_REQUIRE_VALID (self, block);

  if (BLOCK_SUPERBLOCK (block) == NULL)
    Py_RETURN_NONE;

  static_block = block_static_block (block);

  return block_to_block_object (static_block, self_obj->objfile);
}

/* Implementation of gdb.Block.is_global (self) -> Boolean.
   Returns True if this block object is a global block.  */

static PyObject *
blpy_is_global (PyObject *self, void *closure)
{
  const struct block *block;

  BLPY_REQUIRE_VALID (self, block);

  if (BLOCK_SUPERBLOCK (block))
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.Block.is_static (self) -> Boolean.
   Returns True if this block object is a static block.  */

static PyObject *
blpy_is_static (PyObject *self, void *closure)
{
  const struct block *block;

  BLPY_REQUIRE_VALID (self, block);

  if (BLOCK_SUPERBLOCK (block) != NULL
     && BLOCK_SUPERBLOCK (BLOCK_SUPERBLOCK (block)) == NULL)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

static void
blpy_dealloc (PyObject *obj)
{
  block_object *block = (block_object *) obj;

  if (block->prev)
    block->prev->next = block->next;
  else if (block->objfile)
    {
      set_objfile_data (block->objfile, blpy_objfile_data_key,
			block->next);
    }
  if (block->next)
    block->next->prev = block->prev;
  block->block = NULL;
}

/* Given a block, and a block_object that has previously been
   allocated and initialized, populate the block_object with the
   struct block data.  Also, register the block_object life-cycle
   with the life-cycle of the object file associated with this
   block, if needed.  */
static void
set_block (block_object *obj, const struct block *block,
	   struct objfile *objfile)
{
  obj->block = block;
  obj->prev = NULL;
  if (objfile)
    {
      obj->objfile = objfile;
      obj->next = objfile_data (objfile, blpy_objfile_data_key);
      if (obj->next)
	obj->next->prev = obj;
      set_objfile_data (objfile, blpy_objfile_data_key, obj);
    }
  else
    obj->next = NULL;
}

/* Create a new block object (gdb.Block) that encapsulates the struct
   block object from GDB.  */
PyObject *
block_to_block_object (const struct block *block, struct objfile *objfile)
{
  block_object *block_obj;

  block_obj = PyObject_New (block_object, &block_object_type);
  if (block_obj)
    set_block (block_obj, block, objfile);

  return (PyObject *) block_obj;
}

/* Return struct block reference that is wrapped by this object.  */
const struct block *
block_object_to_block (PyObject *obj)
{
  if (! PyObject_TypeCheck (obj, &block_object_type))
    return NULL;
  return ((block_object *) obj)->block;
}

/* Return a reference to the block iterator.  */
static PyObject *
blpy_block_syms_iter (PyObject *self)
{
  block_syms_iterator_object *iter_obj = (block_syms_iterator_object *) self;

  BLPY_ITER_REQUIRE_VALID (iter_obj->source);

  Py_INCREF (self);
  return self;
}

/* Return the next symbol in the iteration through the block's
   dictionary.  */
static PyObject *
blpy_block_syms_iternext (PyObject *self)
{
  block_syms_iterator_object *iter_obj = (block_syms_iterator_object *) self;
  struct symbol *sym;

  BLPY_ITER_REQUIRE_VALID (iter_obj->source);

  if (!iter_obj->initialized_p)
    {
      sym = block_iterator_first (iter_obj->block,  &(iter_obj->iter));
      iter_obj->initialized_p = 1;
    }
  else
    sym = block_iterator_next (&(iter_obj->iter));

  if (sym == NULL)
    {
      PyErr_SetString (PyExc_StopIteration, _("Symbol is null."));
      return NULL;
    }

  return symbol_to_symbol_object (sym);
}

static void
blpy_block_syms_dealloc (PyObject *obj)
{
  block_syms_iterator_object *iter_obj = (block_syms_iterator_object *) obj;

  Py_XDECREF (iter_obj->source);
}

/* Implementation of gdb.Block.is_valid (self) -> Boolean.
   Returns True if this block object still exists in GDB.  */

static PyObject *
blpy_is_valid (PyObject *self, PyObject *args)
{
  const struct block *block;

  block = block_object_to_block (self);
  if (block == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Implementation of gdb.BlockIterator.is_valid (self) -> Boolean.
   Returns True if this block iterator object still exists in GDB  */

static PyObject *
blpy_iter_is_valid (PyObject *self, PyObject *args)
{
  block_syms_iterator_object *iter_obj =
    (block_syms_iterator_object *) self;

  if (iter_obj->source->block == NULL)
    Py_RETURN_FALSE;

  Py_RETURN_TRUE;
}

/* Return the innermost lexical block containing the specified pc value,
   or 0 if there is none.  */
PyObject *
gdbpy_block_for_pc (PyObject *self, PyObject *args)
{
  gdb_py_ulongest pc;
  struct block *block = NULL;
  struct obj_section *section = NULL;
  struct symtab *symtab = NULL;
  volatile struct gdb_exception except;

  if (!PyArg_ParseTuple (args, GDB_PY_LLU_ARG, &pc))
    return NULL;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      section = find_pc_mapped_section (pc);
      symtab = find_pc_sect_symtab (pc, section);

      if (symtab != NULL && symtab->objfile != NULL)
	block = block_for_pc (pc);
    }
  GDB_PY_HANDLE_EXCEPTION (except);

  if (!symtab || symtab->objfile == NULL)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Cannot locate object file for block."));
      return NULL;
    }

  if (block)
    return block_to_block_object (block, symtab->objfile);

  Py_RETURN_NONE;
}

/* This function is called when an objfile is about to be freed.
   Invalidate the block as further actions on the block would result
   in bad data.  All access to obj->symbol should be gated by
   BLPY_REQUIRE_VALID which will raise an exception on invalid
   blocks.  */
static void
del_objfile_blocks (struct objfile *objfile, void *datum)
{
  block_object *obj = datum;

  while (obj)
    {
      block_object *next = obj->next;

      obj->block = NULL;
      obj->objfile = NULL;
      obj->next = NULL;
      obj->prev = NULL;

      obj = next;
    }
}

void
gdbpy_initialize_blocks (void)
{
  block_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&block_object_type) < 0)
    return;

  block_syms_iterator_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&block_syms_iterator_object_type) < 0)
    return;

  /* Register an objfile "free" callback so we can properly
     invalidate blocks when an object file is about to be
     deleted.  */
  blpy_objfile_data_key
    = register_objfile_data_with_cleanup (NULL, del_objfile_blocks);

  Py_INCREF (&block_object_type);
  PyModule_AddObject (gdb_module, "Block", (PyObject *) &block_object_type);

  Py_INCREF (&block_syms_iterator_object_type);
  PyModule_AddObject (gdb_module, "BlockIterator",
		      (PyObject *) &block_syms_iterator_object_type);
}



static PyMethodDef block_object_methods[] = {
  { "is_valid", blpy_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this block is valid, false if not." },
  {NULL}  /* Sentinel */
};

static PyGetSetDef block_object_getset[] = {
  { "start", blpy_get_start, NULL, "Start address of the block.", NULL },
  { "end", blpy_get_end, NULL, "End address of the block.", NULL },
  { "function", blpy_get_function, NULL,
    "Symbol that names the block, or None.", NULL },
  { "superblock", blpy_get_superblock, NULL,
    "Block containing the block, or None.", NULL },
  { "global_block", blpy_get_global_block, NULL,
    "Block containing the global block.", NULL },
  { "static_block", blpy_get_static_block, NULL,
    "Block containing the static block.", NULL },
  { "is_static", blpy_is_static, NULL,
    "Whether this block is a static block.", NULL },
  { "is_global", blpy_is_global, NULL,
    "Whether this block is a global block.", NULL },
  { NULL }  /* Sentinel */
};

PyTypeObject block_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Block",			  /*tp_name*/
  sizeof (block_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  blpy_dealloc,                   /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER,  /*tp_flags*/
  "GDB block object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  0,				  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  blpy_iter,			  /* tp_iter */
  0,				  /* tp_iternext */
  block_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  block_object_getset		  /* tp_getset */
};

static PyMethodDef block_iterator_object_methods[] = {
  { "is_valid", blpy_iter_is_valid, METH_NOARGS,
    "is_valid () -> Boolean.\n\
Return true if this block iterator is valid, false if not." },
  {NULL}  /* Sentinel */
};

static PyTypeObject block_syms_iterator_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.BlockIterator",		  /*tp_name*/
  sizeof (block_syms_iterator_object),	      /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  blpy_block_syms_dealloc,	  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  0,				  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER,  /*tp_flags*/
  "GDB block syms iterator object",	      /*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  blpy_block_syms_iter,           /*tp_iter */
  blpy_block_syms_iternext,	  /*tp_iternext */
  block_iterator_object_methods   /*tp_methods */
};
