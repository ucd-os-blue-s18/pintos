#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*Defines structure and methods for fixed point arithmetic
17.14 split is used with a single bit for sign.
f is used as constant value 2^14 used internally for
methods.

Methods follow method(first_operand, second_operand)
Return value is a fixP unless the method is specifically
for returning intergers. If a fixed point number and an
integer are passed as arguments, the fixP should be
passed first.*/

/*Constant "f" for internal math */
static int f = 1 << 14;

/* fixP struct */
struct fixP
{
  int i;
};


static inline void setToInt(struct fixP * x, int n)
{
  x->i = n * f;
}

static inline struct fixP intToFixed(int n)
{
  struct fixP newFixed;
  newFixed.i = n * f;
  return newFixed;
}

static inline int fixedToInt(struct fixP x)
{
  return x.i/f;
}

static inline int fixedToIntRound(struct fixP x)
{
  if (x.i >= 0){
    return ((x.i + (f / 2))/f);
  }
  else{
    return ((x.i - (f / 2))/f);
  }
}

static inline struct fixP fixedAdd(struct fixP x, struct fixP y)
{
  struct fixP z;
  z.i = x.i + y.i;
  return z;
}

static inline struct fixP fixedSub(struct fixP x, struct fixP y)
{
  struct fixP z;
  z.i = x.i - y.i;
  return z;
}

static inline struct fixP fixedIntAdd(struct fixP x, int n)
{
  struct fixP z;
  z.i = x.i + n * f;
  return z;
}

static inline struct fixP fixedIntSub(struct fixP x, int n)
{
  struct fixP z;
  z.i = x.i - n * f;
  return z;
}

static inline struct fixP fixedMult(struct fixP x, struct fixP y)
{
  struct fixP z;
  z.i = ((int64_t)x.i) * y.i / f;
  return z;
}

static inline struct fixP fixedDiv(struct fixP x, struct fixP y)
{
  struct fixP z;
  z.i = ((int64_t)x.i) * f / y.i;
  return z;
}

static inline struct fixP fixedIntMult(struct fixP x, int n)
{
  struct fixP z;
  z.i = x.i * n;
  return z;
}

static inline struct fixP fixedIntDiv(struct fixP x, int n)
{
  struct fixP z;
  z.i = x.i / n;
  return z;
}

#endif /*threads/fixed-point.h */
