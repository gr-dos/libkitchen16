/* Call-site fixture for tests/check-abi, the comparison operators (docs/spec/fpc.md,
 * "Comparison"): each operator as a value, as a branch and through pointers
 * (the E forms with far data), against a zero literal (which wcc compiles with
 * no helper), and against a zero that is not a literal (which it compiles
 * as a helper call). */

extern void yes(void), no(void);

int v_eq(double a, double b) { return a == b; }
int v_ne(double a, double b) { return a != b; }
int v_lt(double a, double b) { return a <  b; }
int v_le(double a, double b) { return a <= b; }
int v_gt(double a, double b) { return a >  b; }
int v_ge(double a, double b) { return a >= b; }
int p_eq(double *a, double *b) { return *a == *b; }
int p_ne(double *a, double *b) { return *a != *b; }
int p_lt(double *a, double *b) { return *a <  *b; }
int p_le(double *a, double *b) { return *a <= *b; }
int p_gt(double *a, double *b) { return *a >  *b; }
int p_ge(double *a, double *b) { return *a >= *b; }
void b_eq(double a, double b) { if (a == b) yes(); else no(); }
void b_ne(double a, double b) { if (a != b) yes(); else no(); }
void b_lt(double a, double b) { if (a <  b) yes(); else no(); }
void b_le(double a, double b) { if (a <= b) yes(); else no(); }
void b_gt(double a, double b) { if (a >  b) yes(); else no(); }
void b_ge(double a, double b) { if (a >= b) yes(); else no(); }

int z_eq(double a) { return a == 0.0; }
int z_ne(double a) { return a != 0.0; }
int z_lt(double a) { return a <  0.0; }
int z_le(double a) { return a <= 0.0; }
int z_gt(double a) { return a >  0.0; }
int z_ge(double a) { return a >= 0.0; }
int z_not(double a) { return !a; }
void z_if(double a) { if (a) yes(); else no(); }
int z_cond(double a) { return a ? 7 : 9; }
int z_and(double a, double b) { return a && b; }
int zp_eq(double *a) { return *a == 0.0; }

static volatile double zero;
int n_eq(double a) { return a == zero; }
int n_lt(double a) { return a < zero; }
int k_eq(double a) { return a == 1.0; }
