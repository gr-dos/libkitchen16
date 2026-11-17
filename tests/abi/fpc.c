/* Call-site fixture for tests/check-abi: every operation that wcc -fpc
 * compiles as a call to a helper of docs/spec/fpc.md, once with operands
 * in registers and once with the right operand reached through a pointer
 * (the E forms, with far data). */

double f_add(double a, double b) { return a + b; }
double f_sub(double a, double b) { return a - b; }
double f_mul(double a, double b) { return a * b; }
double f_div(double a, double b) { return a / b; }
double f_neg(double a) { return -a; }
int f_lt(double a, double b) { return a < b; }
int f_eq(double a, double b) { return a == b; }
int f_ge(double a, double b) { return a >= b; }
int f_ne(double a, double b) { return a != b; }
double f_addp(double *p, double *q) { return *p + *q; }
double f_subp(double *p, double *q) { return *p - *q; }
double f_mulp(double *p, double *q) { return *p * *q; }
double f_divp(double *p, double *q) { return *p / *q; }
int f_ltp(double *p, double *q) { return *p < *q; }
long f_d2l(double a) { return (long)a; }
int f_d2i(double a) { return (int)a; }
unsigned long f_d2u(double a) { return (unsigned long)a; }
unsigned f_d2ui(double a) { return (unsigned)a; }
double f_l2d(long a) { return (double)a; }
double f_i2d(int a) { return (double)a; }
double f_u2d(unsigned long a) { return (unsigned long)a; }
double f_ui2d(unsigned a) { return (double)a; }
float f_d2f(double a) { return (float)a; }
double f_f2d(float a) { return (double)a; }
