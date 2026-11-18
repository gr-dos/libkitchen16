/* kdd.h - the error term of a sum whose larger operand is known (Dekker);
 * the others are kd_sum, kd_prod and kd_prodc (k_dd.c). */

#ifndef KDD_H
#define KDD_H

/* s = a + b rounded, e the exact error (|a| >= |b| or a = 0) */
#define K_FASTSUM(a, b, s, e) { (s) = (a) + (b); (e) = (b) - ((s) - (a)); }

#endif
