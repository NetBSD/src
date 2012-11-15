/*
 *  smart_ptr.h
 *  elftosb
 *
 *  Created by Chris Reed on 4/18/06.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */
#if !defined(_smart_ptr_h_)
#define _smart_ptr_h_

/*!
 * \brief Simple, standard smart pointer class.
 *
 * This class only supports the single-owner paradigm.
 */
template <typename T>
class smart_ptr
{
public:
	typedef T data_type;
	typedef T * ptr_type;
	typedef const T * const_ptr_type;
	typedef T & ref_type;
	typedef const T & const_ref_type;
	
	//! Default constuctor. Initialises with no pointer set.
	smart_ptr() : _p(0) {}
	
	//! This constructor takes a pointer to the object to be deleted.
	smart_ptr(ptr_type p) : _p(p) {}
	
	//! Destructor. If an object (pointer) has been set, it will be deleted.
	//! Deletes the object using safe_delete().
	virtual ~smart_ptr() { safe_delete(); }
	
	//! Return the current pointer value.
	ptr_type get() { return _p; }
	
	//! Return the const form of the current pointer value.
	const_ptr_type get() const { return _p; }
	
	//! Change the pointer value, or set if if the default constructor was used.
	//! If a pointer had previously been associated with the object, and \a p is
	//! different than that previous pointer, it will be deleted before taking
	//! ownership of \a p. If this is not desired, call reset() beforehand.
	void set(ptr_type p)
	{
		if (_p && p != _p)
		{
			safe_delete();
		}
		_p = p;
	}
	
	//! Dissociates any previously set pointer value without deleting it.
	void reset() { _p = 0; }
	
	//! Dissociates a previously set pointer value, deleting it at the same time.
	void clear() { safe_delete(); }
	
	//! Forces immediate deletion of the object. If you are planning on using
	//! this method, think about just using a normal pointer. It probably makes
	//! more sense.
	virtual void safe_delete()
	{
		if (_p)
		{
			delete _p;
			_p = 0;
		} 
	}
	
	//! \name Operators
	//@{
	
	//! Makes the object transparent as the template type.
	operator ptr_type () { return _p; }
	
	//! Const version of the pointer operator.
	operator const_ptr_type () const { return _p; }
	
	//! Makes the object transparent as a reference of the template type.
	operator ref_type () { return *_p; }
	
	//! Const version of the reference operator.
	operator const_ref_type () const { return *_p; }
	
	//! Returns a boolean indicating whether the object has a pointer set or not.
	operator bool () const { return _p != 0; }
	
	//! To allow setting the pointer directly. Equivalent to a call to set().
	smart_ptr<T> & operator = (const_ptr_type p)
	{
		set(const_cast<ptr_type>(p));
		return *this;
	}
	
	//! Another operator to allow you to treat the object just like a pointer.
	ptr_type operator ->() { return _p; }
	
	//! Another operator to allow you to treat the object just like a pointer.
	const_ptr_type operator ->() const { return _p; }
	
//	//! Pointer dereferencing operator.
//	ref_type operator * () const { return *_p; }
//	
//	//! Const version of the pointer dereference operator.
//	const_ref_type operator * () const { return *_p; }
	
	//@}

protected:
	ptr_type _p;	//!< The wrapped pointer.
};

/*!
 * \brief Simple, standard smart pointer class that uses the array delete operator.
 *
 * This class only supports the single-owner paradigm.
 *
 * This is almost entirely a copy of smart_ptr since the final C++ specification
 * does not allow template subclass members to access members  of the parent that
 * do not depend on the template parameter.
 */
template <typename T>
class smart_array_ptr
{
public:
	typedef T data_type;
	typedef T * ptr_type;
	typedef const T * const_ptr_type;
	typedef T & ref_type;
	typedef const T & const_ref_type;
	
	//! Default constuctor. Initialises with no pointer set.
	smart_array_ptr() : _p(0) {}
	
	//! This constructor takes a pointer to the object to be deleted.
	smart_array_ptr(ptr_type p) : _p(p) {}
	
	//! Destructor. If an array has been set, it will be deleted.
	//! Deletes the array using safe_delete().
	virtual ~smart_array_ptr() { safe_delete(); }
	
	//! Return the current pointer value.
	ptr_type get() { return _p; }
	
	//! Return the const form of the current pointer value.
	const_ptr_type get() const { return _p; }
	
	//! Change the pointer value, or set if if the default constructor was used.
	//! If a pointer had previously been associated with the object, and \a p is
	//! different than that previous pointer, it will be deleted before taking
	//! ownership of \a p. If this is not desired, call reset() beforehand.
	void set(ptr_type p)
	{
		if (_p && p != _p)
		{
			safe_delete();
		}
		_p = p;
	}
	
	//! Dissociates any previously set pointer value without deleting it.
	void reset() { _p = 0; }
	
	//! Dissociates a previously set pointer value, deleting it at the same time.
	void clear() { safe_delete(); }
	
	//! Forces immediate deletion of the object. If you are planning on using
	//! this method, think about just using a normal pointer. It probably makes
	//! more sense.
	virtual void safe_delete()
	{
		if (_p)
		{
			delete [] _p;
			_p = 0;
		} 
	}
	
	//! \name Operators
	//@{
	
	//! Makes the object transparent as the template type.
	operator ptr_type () { return _p; }
	
	//! Const version of the pointer operator.
	operator const_ptr_type () const { return _p; }
	
	//! Makes the object transparent as a reference of the template type.
	operator ref_type () { return *_p; }
	
	//! Const version of the reference operator.
	operator const_ref_type () const { return *_p; }
	
	//! Returns a boolean indicating whether the object has a pointer set or not.
	operator bool () const { return _p != 0; }
	
	//! To allow setting the pointer directly. Equivalent to a call to set().
	smart_array_ptr<T> & operator = (const_ptr_type p)
	{
		set(const_cast<ptr_type>(p));
		return *this;
	}
	
	//! Another operator to allow you to treat the object just like a pointer.
	ptr_type operator ->() { return _p; }
	
	//! Another operator to allow you to treat the object just like a pointer.
	const_ptr_type operator ->() const { return _p; }
	
	//! Indexing operator.
	ref_type operator [] (unsigned index) { return _p[index]; }
	
	//! Indexing operator.
	const_ref_type operator [] (unsigned index) const { return _p[index]; }
	
//	//! Pointer dereferencing operator.
//	ref_type operator * () const { return *_p; }
//	
//	//! Const version of the pointer dereference operator.
//	const_ref_type operator * () const { return *_p; }
	
	//@}

protected:
	ptr_type _p;	//!< The wrapped pointer.
};

#endif // _smart_ptr_h_
