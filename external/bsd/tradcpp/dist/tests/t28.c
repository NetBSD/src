#if 1
. right

# if 1
.. right
# elif 1
.. wrong
# elif 0
.. wrong
# else
.. wrong
# endif

#elif 1
. wrong

# if 1
.. wrong
# elif 1
.. wrong
# elif 0
.. wrong
# else
.. wrong
# endif

#elif 0
. wrong

# if 1
.. wrong
# elif 1
.. wrong
# elif 0
.. wrong
# else
.. wrong
# endif

#else
. wrong

# if 1
.. wrong
# elif 1
.. wrong
# elif 0
.. wrong
# else
.. wrong
# endif

#endif
