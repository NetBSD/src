// Safe iterator implementation  -*- C++ -*-

// Copyright (C) 2003-2015 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file debug/safe_iterator.h
 *  This file is a GNU debug extension to the Standard C++ Library.
 */

#ifndef _GLIBCXX_DEBUG_SAFE_ITERATOR_H
#define _GLIBCXX_DEBUG_SAFE_ITERATOR_H 1

#include <debug/debug.h>
#include <debug/macros.h>
#include <debug/functions.h>
#include <debug/safe_base.h>
#include <bits/stl_pair.h>
#include <ext/type_traits.h>

namespace __gnu_debug
{
  /** Helper struct to deal with sequence offering a before_begin
   *  iterator.
   **/
  template <typename _Sequence>
    struct _BeforeBeginHelper
    {
      template<typename _Iterator>
	static bool
	_S_Is(const _Safe_iterator<_Iterator, _Sequence>&)
	{ return false; }

      template<typename _Iterator>
	static bool
	_S_Is_Beginnest(const _Safe_iterator<_Iterator, _Sequence>& __it)
	{ return __it.base() == __it._M_get_sequence()->_M_base().begin(); }
    };

  /** Iterators that derive from _Safe_iterator_base can be determined singular
   *  or non-singular.
   **/
  inline bool
  __check_singular_aux(const _Safe_iterator_base* __x)
  { return __x->_M_singular(); }

  /** The precision to which we can calculate the distance between
   *  two iterators.
   */
  enum _Distance_precision
    {
      __dp_equality, //< Can compare iterator equality, only
      __dp_sign,     //< Can determine equality and ordering
      __dp_exact     //< Can determine distance precisely
    };

  /** Determine the distance between two iterators with some known
   *	precision.
  */
  template<typename _Iterator>
    inline std::pair<typename std::iterator_traits<_Iterator>::difference_type,
		     _Distance_precision>
    __get_distance(const _Iterator& __lhs, const _Iterator& __rhs,
		   std::random_access_iterator_tag)
    { return std::make_pair(__rhs - __lhs, __dp_exact); }

  template<typename _Iterator>
    inline std::pair<typename std::iterator_traits<_Iterator>::difference_type,
		     _Distance_precision>
    __get_distance(const _Iterator& __lhs, const _Iterator& __rhs,
		   std::forward_iterator_tag)
    { return std::make_pair(__lhs == __rhs? 0 : 1, __dp_equality); }

  template<typename _Iterator>
    inline std::pair<typename std::iterator_traits<_Iterator>::difference_type,
		     _Distance_precision>
    __get_distance(const _Iterator& __lhs, const _Iterator& __rhs)
    {
      typedef typename std::iterator_traits<_Iterator>::iterator_category
	  _Category;
      return __get_distance(__lhs, __rhs, _Category());
    }

  /** \brief Safe iterator wrapper.
   *
   *  The class template %_Safe_iterator is a wrapper around an
   *  iterator that tracks the iterator's movement among sequences and
   *  checks that operations performed on the "safe" iterator are
   *  legal. In additional to the basic iterator operations (which are
   *  validated, and then passed to the underlying iterator),
   *  %_Safe_iterator has member functions for iterator invalidation,
   *  attaching/detaching the iterator from sequences, and querying
   *  the iterator's state.
   *
   *  Note that _Iterator must be the first base class so that it gets
   *  initialized before the iterator is being attached to the container's list
   *  of iterators and it is being detached before _Iterator get
   *  destroyed. Otherwise it would result in a data race.
   */
  template<typename _Iterator, typename _Sequence>
    class _Safe_iterator
    : private _Iterator,
      public _Safe_iterator_base
    {
      typedef _Iterator _Iter_base;
      typedef _Safe_iterator_base _Safe_base;
      typedef typename _Sequence::const_iterator _Const_iterator;

      /// Determine if this is a constant iterator.
      bool
      _M_constant() const
      { return std::__are_same<_Const_iterator, _Safe_iterator>::__value; }

      typedef std::iterator_traits<_Iterator> _Traits;

      struct _Attach_single
      { };

      _Safe_iterator(const _Iterator& __i, _Safe_sequence_base* __seq,
		     _Attach_single)
      _GLIBCXX_NOEXCEPT
      : _Iter_base(__i)
      { _M_attach_single(__seq); }

    public:
      typedef _Iterator					iterator_type;
      typedef typename _Traits::iterator_category	iterator_category;
      typedef typename _Traits::value_type		value_type;
      typedef typename _Traits::difference_type		difference_type;
      typedef typename _Traits::reference		reference;
      typedef typename _Traits::pointer			pointer;

      /// @post the iterator is singular and unattached
      _Safe_iterator() _GLIBCXX_NOEXCEPT : _Iter_base() { }

      /**
       * @brief Safe iterator construction from an unsafe iterator and
       * its sequence.
       *
       * @pre @p seq is not NULL
       * @post this is not singular
       */
      _Safe_iterator(const _Iterator& __i, const _Safe_sequence_base* __seq)
      _GLIBCXX_NOEXCEPT
      : _Iter_base(__i), _Safe_base(__seq, _M_constant())
      {
	_GLIBCXX_DEBUG_VERIFY(!this->_M_singular(),
			      _M_message(__msg_init_singular)
			      ._M_iterator(*this, "this"));
      }

      /**
       * @brief Copy construction.
       */
      _Safe_iterator(const _Safe_iterator& __x) _GLIBCXX_NOEXCEPT
      : _Iter_base(__x.base())
      {
	// _GLIBCXX_RESOLVE_LIB_DEFECTS
	// DR 408. Is vector<reverse_iterator<char*> > forbidden?
	_GLIBCXX_DEBUG_VERIFY(!__x._M_singular()
			      || __x.base() == _Iterator(),
			      _M_message(__msg_init_copy_singular)
			      ._M_iterator(*this, "this")
			      ._M_iterator(__x, "other"));
	_M_attach(__x._M_sequence);
      }

#if __cplusplus >= 201103L
      /**
       * @brief Move construction.
       * @post __x is singular and unattached
       */
      _Safe_iterator(_Safe_iterator&& __x) noexcept
      : _Iter_base()
      {
	_GLIBCXX_DEBUG_VERIFY(!__x._M_singular()
			      || __x.base() == _Iterator(),
			      _M_message(__msg_init_copy_singular)
			      ._M_iterator(*this, "this")
			      ._M_iterator(__x, "other"));
	_Safe_sequence_base* __seq = __x._M_sequence;
	__x._M_detach();
	std::swap(base(), __x.base());
	_M_attach(__seq);
      }
#endif

      /**
       *  @brief Converting constructor from a mutable iterator to a
       *  constant iterator.
      */
      template<typename _MutableIterator>
	_Safe_iterator(
	  const _Safe_iterator<_MutableIterator,
	  typename __gnu_cxx::__enable_if<(std::__are_same<_MutableIterator,
		      typename _Sequence::iterator::iterator_type>::__value),
		   _Sequence>::__type>& __x) _GLIBCXX_NOEXCEPT
	: _Iter_base(__x.base())
	{
	  // _GLIBCXX_RESOLVE_LIB_DEFECTS
	  // DR 408. Is vector<reverse_iterator<char*> > forbidden?
	  _GLIBCXX_DEBUG_VERIFY(!__x._M_singular()
				|| __x.base() == _Iterator(),
				_M_message(__msg_init_const_singular)
				._M_iterator(*this, "this")
				._M_iterator(__x, "other"));
	  _M_attach(__x._M_sequence);
	}

      /**
       * @brief Copy assignment.
       */
      _Safe_iterator&
      operator=(const _Safe_iterator& __x) _GLIBCXX_NOEXCEPT
      {
	// _GLIBCXX_RESOLVE_LIB_DEFECTS
	// DR 408. Is vector<reverse_iterator<char*> > forbidden?
	_GLIBCXX_DEBUG_VERIFY(!__x._M_singular()
			      || __x.base() == _Iterator(),
			      _M_message(__msg_copy_singular)
			      ._M_iterator(*this, "this")
			      ._M_iterator(__x, "other"));

	if (this->_M_sequence && this->_M_sequence == __x._M_sequence)
	  {
	    __gnu_cxx::__scoped_lock __l(this->_M_get_mutex());
	    base() = __x.base();
	    _M_version = __x._M_sequence->_M_version;
	  }
	else
	  {
	    _M_detach();
	    base() = __x.base();
	    _M_attach(__x._M_sequence);
	  }

	return *this;
      }

#if __cplusplus >= 201103L
      /**
       * @brief Move assignment.
       * @post __x is singular and unattached
       */
      _Safe_iterator&
      operator=(_Safe_iterator&& __x) noexcept
      {
	_GLIBCXX_DEBUG_VERIFY(this != &__x,
			      _M_message(__msg_self_move_assign)
			      ._M_iterator(*this, "this"));
	_GLIBCXX_DEBUG_VERIFY(!__x._M_singular()
			      || __x.base() == _Iterator(),
			      _M_message(__msg_copy_singular)
			      ._M_iterator(*this, "this")
			      ._M_iterator(__x, "other"));

	if (this->_M_sequence && this->_M_sequence == __x._M_sequence)
	  {
	    __gnu_cxx::__scoped_lock __l(this->_M_get_mutex());
	    base() = __x.base();
	    _M_version = __x._M_sequence->_M_version;
	  }
	else
	  {
	    _M_detach();
	    base() = __x.base();
	    _M_attach(__x._M_sequence);
	  }

	__x._M_detach();
	__x.base() = _Iterator();
	return *this;
      }
#endif

      /**
       *  @brief Iterator dereference.
       *  @pre iterator is dereferenceable
       */
      reference
      operator*() const _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_dereferenceable(),
			      _M_message(__msg_bad_deref)
			      ._M_iterator(*this, "this"));
	return *base();
      }

      /**
       *  @brief Iterator dereference.
       *  @pre iterator is dereferenceable
       *  @todo Make this correct w.r.t. iterators that return proxies
       */
      pointer
      operator->() const _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_dereferenceable(),
			      _M_message(__msg_bad_deref)
			      ._M_iterator(*this, "this"));
	return std::__addressof(*base());
      }

      // ------ Input iterator requirements ------
      /**
       *  @brief Iterator preincrement
       *  @pre iterator is incrementable
       */
      _Safe_iterator&
      operator++() _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_incrementable(),
			      _M_message(__msg_bad_inc)
			      ._M_iterator(*this, "this"));
	__gnu_cxx::__scoped_lock(this->_M_get_mutex());
	++base();
	return *this;
      }

      /**
       *  @brief Iterator postincrement
       *  @pre iterator is incrementable
       */
      _Safe_iterator
      operator++(int) _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_incrementable(),
			      _M_message(__msg_bad_inc)
			      ._M_iterator(*this, "this"));
	__gnu_cxx::__scoped_lock __l(this->_M_get_mutex());
	return _Safe_iterator(base()++, this->_M_sequence, _Attach_single());
      }

      // ------ Bidirectional iterator requirements ------
      /**
       *  @brief Iterator predecrement
       *  @pre iterator is decrementable
       */
      _Safe_iterator&
      operator--() _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_decrementable(),
			      _M_message(__msg_bad_dec)
			      ._M_iterator(*this, "this"));
	__gnu_cxx::__scoped_lock __l(this->_M_get_mutex());
	--base();
	return *this;
      }

      /**
       *  @brief Iterator postdecrement
       *  @pre iterator is decrementable
       */
      _Safe_iterator
      operator--(int) _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_decrementable(),
			      _M_message(__msg_bad_dec)
			      ._M_iterator(*this, "this"));
	__gnu_cxx::__scoped_lock __l(this->_M_get_mutex());
	return _Safe_iterator(base()--, this->_M_sequence, _Attach_single());
      }

      // ------ Random access iterator requirements ------
      reference
      operator[](const difference_type& __n) const _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_can_advance(__n)
			      && this->_M_can_advance(__n+1),
			      _M_message(__msg_iter_subscript_oob)
			      ._M_iterator(*this)._M_integer(__n));
	return base()[__n];
      }

      _Safe_iterator&
      operator+=(const difference_type& __n) _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_can_advance(__n),
			      _M_message(__msg_advance_oob)
			      ._M_iterator(*this)._M_integer(__n));
	__gnu_cxx::__scoped_lock __l(this->_M_get_mutex());
	base() += __n;
	return *this;
      }

      _Safe_iterator
      operator+(const difference_type& __n) const _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_can_advance(__n),
			      _M_message(__msg_advance_oob)
			      ._M_iterator(*this)._M_integer(__n));
	return _Safe_iterator(base() + __n, this->_M_sequence);
      }

      _Safe_iterator&
      operator-=(const difference_type& __n) _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_can_advance(-__n),
			      _M_message(__msg_retreat_oob)
			      ._M_iterator(*this)._M_integer(__n));
	__gnu_cxx::__scoped_lock __l(this->_M_get_mutex());
	base() -= __n;
	return *this;
      }

      _Safe_iterator
      operator-(const difference_type& __n) const _GLIBCXX_NOEXCEPT
      {
	_GLIBCXX_DEBUG_VERIFY(this->_M_can_advance(-__n),
			      _M_message(__msg_retreat_oob)
			      ._M_iterator(*this)._M_integer(__n));
	return _Safe_iterator(base() - __n, this->_M_sequence);
      }

      // ------ Utilities ------
      /**
       * @brief Return the underlying iterator
       */
      _Iterator&
      base() _GLIBCXX_NOEXCEPT { return *this; }

      const _Iterator&
      base() const _GLIBCXX_NOEXCEPT { return *this; }

      /**
       * @brief Conversion to underlying non-debug iterator to allow
       * better interaction with non-debug containers.
       */
      operator _Iterator() const _GLIBCXX_NOEXCEPT { return *this; }

      /** Attach iterator to the given sequence. */
      void
      _M_attach(_Safe_sequence_base* __seq)
      { _Safe_base::_M_attach(__seq, _M_constant()); }

      /** Likewise, but not thread-safe. */
      void
      _M_attach_single(_Safe_sequence_base* __seq)
      { _Safe_base::_M_attach_single(__seq, _M_constant()); }

      /// Is the iterator dereferenceable?
      bool
      _M_dereferenceable() const
      { return !this->_M_singular() && !_M_is_end() && !_M_is_before_begin(); }

      /// Is the iterator before a dereferenceable one?
      bool
      _M_before_dereferenceable() const
      {
	if (this->_M_incrementable())
	{
	  _Iterator __base = base();
	  return ++__base != _M_get_sequence()->_M_base().end();
	}
	return false;
      }

      /// Is the iterator incrementable?
      bool
      _M_incrementable() const
      { return !this->_M_singular() && !_M_is_end(); }

      // Is the iterator decrementable?
      bool
      _M_decrementable() const { return !_M_singular() && !_M_is_begin(); }

      // Can we advance the iterator @p __n steps (@p __n may be negative)
      bool
      _M_can_advance(const difference_type& __n) const;

      // Is the iterator range [*this, __rhs) valid?
      bool
      _M_valid_range(const _Safe_iterator& __rhs) const;

      // The sequence this iterator references.
      typename
      __gnu_cxx::__conditional_type<std::__are_same<_Const_iterator,
						    _Safe_iterator>::__value,
				    const _Sequence*,
				    _Sequence*>::__type
      _M_get_sequence() const
      { return static_cast<_Sequence*>(_M_sequence); }

      /// Is this iterator equal to the sequence's begin() iterator?
      bool
      _M_is_begin() const
      { return base() == _M_get_sequence()->_M_base().begin(); }

      /// Is this iterator equal to the sequence's end() iterator?
      bool
      _M_is_end() const
      { return base() == _M_get_sequence()->_M_base().end(); }

      /// Is this iterator equal to the sequence's before_begin() iterator if
      /// any?
      bool
      _M_is_before_begin() const
      { return _BeforeBeginHelper<_Sequence>::_S_Is(*this); }

      /// Is this iterator equal to the sequence's before_begin() iterator if
      /// any or begin() otherwise?
      bool
      _M_is_beginnest() const
      { return _BeforeBeginHelper<_Sequence>::_S_Is_Beginnest(*this); }
    };

  template<typename _IteratorL, typename _IteratorR, typename _Sequence>
    inline bool
    operator==(const _Safe_iterator<_IteratorL, _Sequence>& __lhs,
	       const _Safe_iterator<_IteratorR, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_compare_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_compare_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() == __rhs.base();
    }

  template<typename _Iterator, typename _Sequence>
    inline bool
    operator==(const _Safe_iterator<_Iterator, _Sequence>& __lhs,
	       const _Safe_iterator<_Iterator, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_compare_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_compare_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() == __rhs.base();
    }

  template<typename _IteratorL, typename _IteratorR, typename _Sequence>
    inline bool
    operator!=(const _Safe_iterator<_IteratorL, _Sequence>& __lhs,
	       const _Safe_iterator<_IteratorR, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_compare_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_compare_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() != __rhs.base();
    }

  template<typename _Iterator, typename _Sequence>
    inline bool
    operator!=(const _Safe_iterator<_Iterator, _Sequence>& __lhs,
	       const _Safe_iterator<_Iterator, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_compare_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_compare_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() != __rhs.base();
    }

  template<typename _IteratorL, typename _IteratorR, typename _Sequence>
    inline bool
    operator<(const _Safe_iterator<_IteratorL, _Sequence>& __lhs,
	      const _Safe_iterator<_IteratorR, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_order_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_order_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() < __rhs.base();
    }

  template<typename _Iterator, typename _Sequence>
    inline bool
    operator<(const _Safe_iterator<_Iterator, _Sequence>& __lhs,
	      const _Safe_iterator<_Iterator, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_order_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_order_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() < __rhs.base();
    }

  template<typename _IteratorL, typename _IteratorR, typename _Sequence>
    inline bool
    operator<=(const _Safe_iterator<_IteratorL, _Sequence>& __lhs,
	       const _Safe_iterator<_IteratorR, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_order_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_order_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() <= __rhs.base();
    }

  template<typename _Iterator, typename _Sequence>
    inline bool
    operator<=(const _Safe_iterator<_Iterator, _Sequence>& __lhs,
	       const _Safe_iterator<_Iterator, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_order_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_order_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() <= __rhs.base();
    }

  template<typename _IteratorL, typename _IteratorR, typename _Sequence>
    inline bool
    operator>(const _Safe_iterator<_IteratorL, _Sequence>& __lhs,
	      const _Safe_iterator<_IteratorR, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_order_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_order_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() > __rhs.base();
    }

  template<typename _Iterator, typename _Sequence>
    inline bool
    operator>(const _Safe_iterator<_Iterator, _Sequence>& __lhs,
	      const _Safe_iterator<_Iterator, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_order_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_order_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() > __rhs.base();
    }

  template<typename _IteratorL, typename _IteratorR, typename _Sequence>
    inline bool
    operator>=(const _Safe_iterator<_IteratorL, _Sequence>& __lhs,
	       const _Safe_iterator<_IteratorR, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_order_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_order_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() >= __rhs.base();
    }

  template<typename _Iterator, typename _Sequence>
    inline bool
    operator>=(const _Safe_iterator<_Iterator, _Sequence>& __lhs,
	       const _Safe_iterator<_Iterator, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_iter_order_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_order_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() >= __rhs.base();
    }

  // _GLIBCXX_RESOLVE_LIB_DEFECTS
  // According to the resolution of DR179 not only the various comparison
  // operators but also operator- must accept mixed iterator/const_iterator
  // parameters.
  template<typename _IteratorL, typename _IteratorR, typename _Sequence>
    inline typename _Safe_iterator<_IteratorL, _Sequence>::difference_type
    operator-(const _Safe_iterator<_IteratorL, _Sequence>& __lhs,
	      const _Safe_iterator<_IteratorR, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
    {
      _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			    _M_message(__msg_distance_bad)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			    _M_message(__msg_distance_different)
			    ._M_iterator(__lhs, "lhs")
			    ._M_iterator(__rhs, "rhs"));
      return __lhs.base() - __rhs.base();
    }

   template<typename _Iterator, typename _Sequence>
     inline typename _Safe_iterator<_Iterator, _Sequence>::difference_type
     operator-(const _Safe_iterator<_Iterator, _Sequence>& __lhs,
	       const _Safe_iterator<_Iterator, _Sequence>& __rhs)
    _GLIBCXX_NOEXCEPT
     {
       _GLIBCXX_DEBUG_VERIFY(! __lhs._M_singular() && ! __rhs._M_singular(),
			     _M_message(__msg_distance_bad)
			     ._M_iterator(__lhs, "lhs")
			     ._M_iterator(__rhs, "rhs"));
       _GLIBCXX_DEBUG_VERIFY(__lhs._M_can_compare(__rhs),
			     _M_message(__msg_distance_different)
			     ._M_iterator(__lhs, "lhs")
			     ._M_iterator(__rhs, "rhs"));
       return __lhs.base() - __rhs.base();
     }

  template<typename _Iterator, typename _Sequence>
    inline _Safe_iterator<_Iterator, _Sequence>
    operator+(typename _Safe_iterator<_Iterator,_Sequence>::difference_type __n,
	      const _Safe_iterator<_Iterator, _Sequence>& __i) _GLIBCXX_NOEXCEPT
    { return __i + __n; }
} // namespace __gnu_debug

#include <debug/safe_iterator.tcc>

#endif
