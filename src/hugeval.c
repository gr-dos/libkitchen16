/* __kitchen16_huge_val: +inf, the object behind <math.h>'s HUGE_VAL
 * (docs/spec/math.md). An object rather than a constant because wcc folds
 * an infinity constant in a return to zero. */

#include <math.h>

const double __cdecl __kitchen16_huge_val = 1e308 * 10.0;
