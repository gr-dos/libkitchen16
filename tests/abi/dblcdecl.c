/* Call-site fixture for tests/check-abi: a double argument and a double
 * result of a __cdecl function (docs/ABI.md, "Floating-point arguments and
 * results"). */

extern double __cdecl ext(double a, int n);

double d_call(double a)
{
    return ext(a, 3) + 1.0;
}

double __cdecl d_def(double a, double b)
{
    return a * b;
}
