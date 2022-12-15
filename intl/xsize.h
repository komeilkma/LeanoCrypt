#ifndef _XSIZE_H
#define _XSIZE_H

/* Get size_t.  */
#include <stddef.h>

/* Get SIZE_MAX.  */
#include <limits.h>
#if HAVE_STDINT_H
# include <stdint.h>
#endif


/* Convert an arbitrary value >= 0 to type size_t.  */
#define xcast_size_t(N) \
  ((N) <= SIZE_MAX ? (size_t) (N) : SIZE_MAX)

/* Sum of two sizes, with overflow check.  */
static inline size_t
#if __GNUC__ >= 3
__attribute__ ((__pure__))
#endif
xsum (size_t size1, size_t size2)
{
  size_t sum = size1 + size2;
  return (sum >= size1 ? sum : SIZE_MAX);
}

/* Sum of three sizes, with overflow check.  */
static inline size_t
#if __GNUC__ >= 3
__attribute__ ((__pure__))
#endif
xsum3 (size_t size1, size_t size2, size_t size3)
{
  return xsum (xsum (size1, size2), size3);
}

/* Sum of four sizes, with overflow check.  */
static inline size_t
#if __GNUC__ >= 3
__attribute__ ((__pure__))
#endif
xsum4 (size_t size1, size_t size2, size_t size3, size_t size4)
{
  return xsum (xsum (xsum (size1, size2), size3), size4);
}

/* Maximum of two sizes, with overflow check.  */
static inline size_t
#if __GNUC__ >= 3
__attribute__ ((__pure__))
#endif
xmax (size_t size1, size_t size2)
{
  /* No explicit check is needed here, because for any n:
     max (SIZE_MAX, n) == SIZE_MAX and max (n, SIZE_MAX) == SIZE_MAX.  */
  return (size1 >= size2 ? size1 : size2);
}

/* Multiplication of a count with an element size, with overflow check.
   The count must be >= 0 and the element size must be > 0.
   This is a macro, not an inline function, so that it works correctly even
   when N is of a wider tupe and N > SIZE_MAX.  */
#define xtimes(N, ELSIZE) \
  ((N) <= SIZE_MAX / (ELSIZE) ? (size_t) (N) * (ELSIZE) : SIZE_MAX)

/* Check for overflow.  */
#define size_overflow_p(SIZE) \
  ((SIZE) == SIZE_MAX)
/* Check against overflow.  */
#define size_in_bounds_p(SIZE) \
  ((SIZE) != SIZE_MAX)

#endif /* _XSIZE_H */
