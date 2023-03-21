/* Python interface to register, and register group information.

   Copyright (C) 2020 Free Software Foundation, Inc.

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
#include "gdbarch.h"
#include "arch-utils.h"
#include "disasm.h"
#include "reggroups.h"
#include "python-internal.h"
#include "user-regs.h"
#include <unordered_map>

/* Token to access per-gdbarch data related to register descriptors.  */
static struct gdbarch_data *gdbpy_register_object_data = NULL;

/* Structure for iterator over register descriptors.  */
typedef struct {
  PyObject_HEAD

  /* The register group that the user is iterating over.  This will never
     be NULL.  */
  struct reggroup *reggroup;

  /* The next register number to lookup.  Starts at 0 and counts up.  */
  int regnum;

  /* Pointer back to the architecture we're finding registers for.  */
  struct gdbarch *gdbarch;
} register_descriptor_iterator_object;

extern PyTypeObject register_descriptor_iterator_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("register_descriptor_iterator_object");

/* A register descriptor.  */
typedef struct {
  PyObject_HEAD

  /* The register this is a descriptor for.  */
  int regnum;

  /* The architecture this is a register for.  */
  struct gdbarch *gdbarch;
} register_descriptor_object;

extern PyTypeObject register_descriptor_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("register_descriptor_object");

/* Structure for iterator over register groups.  */
typedef struct {
  PyObject_HEAD

  /* The last register group returned.  Initially this will be NULL.  */
  struct reggroup *reggroup;

  /* Pointer back to the architecture we're finding registers for.  */
  struct gdbarch *gdbarch;
} reggroup_iterator_object;

extern PyTypeObject reggroup_iterator_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("reggroup_iterator_object");

/* A register group object.  */
typedef struct {
  PyObject_HEAD

  /* The register group being described.  */
  struct reggroup *reggroup;
} reggroup_object;

extern PyTypeObject reggroup_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("reggroup_object");

/* Associates a vector of gdb.RegisterDescriptor objects with GDBARCH as
   gdbarch_data via the gdbarch post init registration mechanism
   (gdbarch_data_register_post_init).  */

static void *
gdbpy_register_object_data_init (struct gdbarch *gdbarch)
{
  return new std::vector<gdbpy_ref<>>;
}

/* Return a gdb.RegisterGroup object wrapping REGGROUP.  The register
   group objects are cached, and the same Python object will always be
   returned for the same REGGROUP pointer.  */

static gdbpy_ref<>
gdbpy_get_reggroup (struct reggroup *reggroup)
{
  /* Map from GDB's internal reggroup objects to the Python representation.
     GDB's reggroups are global, and are never deleted, so using a map like
     this is safe.  */
  static std::unordered_map<struct reggroup *,gdbpy_ref<>>
    gdbpy_reggroup_object_map;

  /* If there is not already a suitable Python object in the map then
     create a new one, and add it to the map.  */
  if (gdbpy_reggroup_object_map[reggroup] == nullptr)
    {
      /* Create a new object and fill in its details.  */
      gdbpy_ref<reggroup_object> group
	(PyObject_New (reggroup_object, &reggroup_object_type));
      if (group == NULL)
	return NULL;
      group->reggroup = reggroup;
      gdbpy_reggroup_object_map[reggroup]
	= gdbpy_ref<> ((PyObject *) group.release ());
    }

  /* Fetch the Python object wrapping REGGROUP from the map, increasing
     the reference count is handled by the gdbpy_ref class.  */
  return gdbpy_reggroup_object_map[reggroup];
}

/* Convert a gdb.RegisterGroup to a string, it just returns the name of
   the register group.  */

static PyObject *
gdbpy_reggroup_to_string (PyObject *self)
{
  reggroup_object *group = (reggroup_object *) self;
  struct reggroup *reggroup = group->reggroup;

  const char *name = reggroup_name (reggroup);
  return PyString_FromString (name);
}

/* Implement gdb.RegisterGroup.name (self) -> String.
   Return a string that is the name of this register group.  */

static PyObject *
gdbpy_reggroup_name (PyObject *self, void *closure)
{
  return gdbpy_reggroup_to_string (self);
}

/* Return a gdb.RegisterDescriptor object for REGNUM from GDBARCH.  For
   each REGNUM (in GDBARCH) only one descriptor is ever created, which is
   then cached on the GDBARCH.  */

static gdbpy_ref<>
gdbpy_get_register_descriptor (struct gdbarch *gdbarch,
			       int regnum)
{
  auto &vec
    = *(std::vector<gdbpy_ref<>> *) gdbarch_data (gdbarch,
						  gdbpy_register_object_data);

  /* Ensure that we have enough entries in the vector.  */
  if (vec.size () <= regnum)
    vec.resize ((regnum + 1), nullptr);

  /* If we don't already have a descriptor for REGNUM in GDBARCH then
     create one now.  */
  if (vec[regnum] == nullptr)
    {
      gdbpy_ref <register_descriptor_object> reg
	(PyObject_New (register_descriptor_object,
		       &register_descriptor_object_type));
      if (reg == NULL)
	return NULL;
      reg->regnum = regnum;
      reg->gdbarch = gdbarch;
      vec[regnum] = gdbpy_ref<> ((PyObject *) reg.release ());
    }

  /* Grab the register descriptor from the vector, the reference count is
     automatically incremented thanks to gdbpy_ref.  */
  return vec[regnum];
}

/* Convert the register descriptor to a string.  */

static PyObject *
gdbpy_register_descriptor_to_string (PyObject *self)
{
  register_descriptor_object *reg
    = (register_descriptor_object *) self;
  struct gdbarch *gdbarch = reg->gdbarch;
  int regnum = reg->regnum;

  const char *name = gdbarch_register_name (gdbarch, regnum);
  return PyString_FromString (name);
}

/* Implement gdb.RegisterDescriptor.name attribute get function.  Return a
   string that is the name of this register.  Due to checking when register
   descriptors are created the name will never by the empty string.  */

static PyObject *
gdbpy_register_descriptor_name (PyObject *self, void *closure)
{
  return gdbpy_register_descriptor_to_string (self);
}

/* Return a reference to the gdb.RegisterGroupsIterator object.  */

static PyObject *
gdbpy_reggroup_iter (PyObject *self)
{
  Py_INCREF (self);
  return self;
}

/* Return the next gdb.RegisterGroup object from the iterator.  */

static PyObject *
gdbpy_reggroup_iter_next (PyObject *self)
{
  reggroup_iterator_object *iter_obj
    = (reggroup_iterator_object *) self;
  struct gdbarch *gdbarch = iter_obj->gdbarch;

  struct reggroup *next_group = reggroup_next (gdbarch, iter_obj->reggroup);
  if (next_group == NULL)
    {
      PyErr_SetString (PyExc_StopIteration, _("No more groups"));
      return NULL;
    }

  iter_obj->reggroup = next_group;
  return gdbpy_get_reggroup (iter_obj->reggroup).release ();
}

/* Return a new gdb.RegisterGroupsIterator over all the register groups in
   GDBARCH.  */

PyObject *
gdbpy_new_reggroup_iterator (struct gdbarch *gdbarch)
{
  gdb_assert (gdbarch != nullptr);

  /* Create a new object and fill in its internal state.  */
  reggroup_iterator_object *iter
    = PyObject_New (reggroup_iterator_object,
		    &reggroup_iterator_object_type);
  if (iter == NULL)
    return NULL;
  iter->reggroup = NULL;
  iter->gdbarch = gdbarch;
  return (PyObject *) iter;
}

/* Create and return a new gdb.RegisterDescriptorIterator object which
   will iterate over all registers in GROUP_NAME for GDBARCH.  If
   GROUP_NAME is either NULL or the empty string then the ALL_REGGROUP is
   used, otherwise lookup the register group matching GROUP_NAME and use
   that.

   This function can return NULL if GROUP_NAME isn't found.  */

PyObject *
gdbpy_new_register_descriptor_iterator (struct gdbarch *gdbarch,
					const char *group_name)
{
  struct reggroup *grp = NULL;

  /* Lookup the requested register group, or find the default.  */
  if (group_name == NULL || *group_name == '\0')
    grp = all_reggroup;
  else
    {
      grp = reggroup_find (gdbarch, group_name);
      if (grp == NULL)
	{
	  PyErr_SetString (PyExc_ValueError,
			   _("Unknown register group name."));
	  return NULL;
	}
    }
  /* Create a new iterator object initialised for this architecture and
     fill in all of the details.  */
  register_descriptor_iterator_object *iter
    = PyObject_New (register_descriptor_iterator_object,
		    &register_descriptor_iterator_object_type);
  if (iter == NULL)
    return NULL;
  iter->regnum = 0;
  iter->gdbarch = gdbarch;
  gdb_assert (grp != NULL);
  iter->reggroup = grp;

  return (PyObject *) iter;
}

/* Return a reference to the gdb.RegisterDescriptorIterator object.  */

static PyObject *
gdbpy_register_descriptor_iter (PyObject *self)
{
  Py_INCREF (self);
  return self;
}

/* Return the next register name.  */

static PyObject *
gdbpy_register_descriptor_iter_next (PyObject *self)
{
  register_descriptor_iterator_object *iter_obj
    = (register_descriptor_iterator_object *) self;
  struct gdbarch *gdbarch = iter_obj->gdbarch;

  do
    {
      if (iter_obj->regnum >= gdbarch_num_cooked_regs (gdbarch))
	{
	  PyErr_SetString (PyExc_StopIteration, _("No more registers"));
	  return NULL;
	}

      const char *name = nullptr;
      int regnum = iter_obj->regnum;
      if (gdbarch_register_reggroup_p (gdbarch, regnum,
				       iter_obj->reggroup))
	name = gdbarch_register_name (gdbarch, regnum);
      iter_obj->regnum++;

      if (name != nullptr && *name != '\0')
	return gdbpy_get_register_descriptor (gdbarch, regnum).release ();
    }
  while (true);
}

/* Implement:

   gdb.RegisterDescriptorIterator.find (self, name) -> gdb.RegisterDescriptor

   Look up a descriptor for register with NAME.  If no matching register is
   found then return None.  */

static PyObject *
register_descriptor_iter_find (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] = { "name", NULL };
  const char *register_name = NULL;

  register_descriptor_iterator_object *iter_obj
    = (register_descriptor_iterator_object *) self;
  struct gdbarch *gdbarch = iter_obj->gdbarch;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "s", keywords,
					&register_name))
    return NULL;

  if (register_name != NULL && *register_name != '\0')
    {
      int regnum = user_reg_map_name_to_regnum (gdbarch, register_name,
						strlen (register_name));
      if (regnum >= 0)
	return gdbpy_get_register_descriptor (gdbarch, regnum).release ();
    }

  Py_RETURN_NONE;
}

/* See python-internal.h.  */

bool
gdbpy_parse_register_id (struct gdbarch *gdbarch, PyObject *pyo_reg_id,
			 int *reg_num)
{
  gdb_assert (pyo_reg_id != NULL);

  /* The register could be a string, its name.  */
  if (gdbpy_is_string (pyo_reg_id))
    {
      gdb::unique_xmalloc_ptr<char> reg_name (gdbpy_obj_to_string (pyo_reg_id));

      if (reg_name != NULL)
	{
	  *reg_num = user_reg_map_name_to_regnum (gdbarch, reg_name.get (),
						  strlen (reg_name.get ()));
	  return *reg_num >= 0;
	}
    }
  /* The register could be its internal GDB register number.  */
  else if (PyInt_Check (pyo_reg_id))
    {
      long value;
      if (gdb_py_int_as_long (pyo_reg_id, &value) && (int) value == value)
        {
	  if (user_reg_map_regnum_to_name (gdbarch, value) != NULL)
	    {
	      *reg_num = (int) value;
	      return true;
	    }
        }
    }
  /* The register could be a gdb.RegisterDescriptor object.  */
  else if (PyObject_IsInstance (pyo_reg_id,
			   (PyObject *) &register_descriptor_object_type))
    {
      register_descriptor_object *reg
	= (register_descriptor_object *) pyo_reg_id;
      if (reg->gdbarch == gdbarch)
	{
	  *reg_num = reg->regnum;
	  return true;
	}
      else
	PyErr_SetString (PyExc_ValueError,
			 _("Invalid Architecture in RegisterDescriptor"));
    }

  gdb_assert (PyErr_Occurred ());
  return false;
}

/* Initializes the new Python classes from this file in the gdb module.  */

int
gdbpy_initialize_registers ()
{
  gdbpy_register_object_data
    = gdbarch_data_register_post_init (gdbpy_register_object_data_init);

  register_descriptor_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&register_descriptor_object_type) < 0)
    return -1;
  if (gdb_pymodule_addobject
      (gdb_module, "RegisterDescriptor",
       (PyObject *) &register_descriptor_object_type) < 0)
    return -1;

  reggroup_iterator_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&reggroup_iterator_object_type) < 0)
    return -1;
  if (gdb_pymodule_addobject
      (gdb_module, "RegisterGroupsIterator",
       (PyObject *) &reggroup_iterator_object_type) < 0)
    return -1;

  reggroup_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&reggroup_object_type) < 0)
    return -1;
  if (gdb_pymodule_addobject
      (gdb_module, "RegisterGroup",
       (PyObject *) &reggroup_object_type) < 0)
    return -1;

  register_descriptor_iterator_object_type.tp_new = PyType_GenericNew;
  if (PyType_Ready (&register_descriptor_iterator_object_type) < 0)
    return -1;
  return (gdb_pymodule_addobject
	  (gdb_module, "RegisterDescriptorIterator",
	   (PyObject *) &register_descriptor_iterator_object_type));
}

static PyMethodDef register_descriptor_iterator_object_methods [] = {
  { "find", (PyCFunction) register_descriptor_iter_find,
    METH_VARARGS | METH_KEYWORDS,
    "registers (name) -> gdb.RegisterDescriptor.\n\
Return a register descriptor for the register NAME, or None if no register\n\
with that name exists in this iterator." },
  {NULL}  /* Sentinel */
};

PyTypeObject register_descriptor_iterator_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.RegisterDescriptorIterator",	  	/*tp_name*/
  sizeof (register_descriptor_iterator_object),	/*tp_basicsize*/
  0,				  /*tp_itemsize*/
  0,				  /*tp_dealloc*/
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
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER,			/*tp_flags*/
  "GDB architecture register descriptor iterator object",	/*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  gdbpy_register_descriptor_iter,	  /*tp_iter */
  gdbpy_register_descriptor_iter_next,  /*tp_iternext */
  register_descriptor_iterator_object_methods		/*tp_methods */
};

static gdb_PyGetSetDef gdbpy_register_descriptor_getset[] = {
  { "name", gdbpy_register_descriptor_name, NULL,
    "The name of this register.", NULL },
  { NULL }  /* Sentinel */
};

PyTypeObject register_descriptor_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.RegisterDescriptor",	  /*tp_name*/
  sizeof (register_descriptor_object),	/*tp_basicsize*/
  0,				  /*tp_itemsize*/
  0,				  /*tp_dealloc*/
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
  gdbpy_register_descriptor_to_string,			/*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB architecture register descriptor object",	/*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  0,				  /*tp_iter */
  0,				  /*tp_iternext */
  0,				  /*tp_methods */
  0,				  /*tp_members */
  gdbpy_register_descriptor_getset			/*tp_getset */
};

PyTypeObject reggroup_iterator_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.RegisterGroupsIterator",	  /*tp_name*/
  sizeof (reggroup_iterator_object),		/*tp_basicsize*/
  0,				  /*tp_itemsize*/
  0,				  /*tp_dealloc*/
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
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER,	/*tp_flags*/
  "GDB register groups iterator object",	/*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  gdbpy_reggroup_iter,		  /*tp_iter */
  gdbpy_reggroup_iter_next,	  /*tp_iternext */
  0				  /*tp_methods */
};

static gdb_PyGetSetDef gdbpy_reggroup_getset[] = {
  { "name", gdbpy_reggroup_name, NULL,
    "The name of this register group.", NULL },
  { NULL }  /* Sentinel */
};

PyTypeObject reggroup_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.RegisterGroup",		  /*tp_name*/
  sizeof (reggroup_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  0,				  /*tp_dealloc*/
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
  gdbpy_reggroup_to_string,	  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB register group object",	  /*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  0,				  /*tp_iter */
  0,				  /*tp_iternext */
  0,				  /*tp_methods */
  0,				  /*tp_members */
  gdbpy_reggroup_getset		  /*tp_getset */
};
