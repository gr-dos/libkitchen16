#!/usr/bin/env python3
# Generates src/kc.h: constants derived from the mathematics (Decimal arithmetic).
# usage: python3 scripts/gen-math-consts.py src/kc.h
from decimal import Decimal as D, getcontext
from fractions import Fraction as F
import struct, math, sys
getcontext().prec = 450

def dpi():
    # Machin: pi = 16 atan(1/5) - 4 atan(1/239)
    def at(x):
        x = D(1)/D(x); s = D(0); t = x; k = 0; x2 = x*x
        while abs(t) > D(10)**-440:
            s += t/(2*k+1) * (1 if k%2==0 else -1)
            t *= x2; k += 1
        return s
    return 16*at(5) - 4*at(239)
PI = dpi()
LN2 = D(2).ln(); LN10 = D(10).ln()

def datan(x):
    x = D(x)
    k = 0
    while abs(x) > D('0.01'):
        x = x/(1+(1+x*x).sqrt()); k += 1
    s = D(0); t = x; i = 0; x2 = x*x
    while abs(t) > D(10)**-440:
        s += t/(2*i+1)*(1 if i%2==0 else -1); t *= x2; i += 1
    return s * (2**k)

def todbl(v):
    return float(v)   # correctly rounded
def split_hi(v, bits):
    # leading `bits` significant bits of v, as a double (truncated)
    f = F(v); 
    if f == 0: return 0.0
    e = 0
    a = abs(f)
    while a >= 2: a /= 2; e += 1
    while a < 1: a *= 2; e -= 1
    scaled = a * (1 << (bits-1))
    n = int(scaled)   # floor
    r = F(n, 1<<(bits-1)) * (F(2)**e)
    return float(r) if f>0 else -float(r)
def tofrac(x): return F(x)
def fmt(x):
    return repr(float(x)) if False else '%.17g' % x
def parts(v, bits, n):
    """v as n parts: first n-1 truncated to `bits` bits, last a full double"""
    out = []; rem = F(v)
    for i in range(n-1):
        p = split_hi(rem, bits); out.append(p); rem -= F(p)
    out.append(float(rem)); return out
def dd(v):
    h = float(v); l = float(F(v) - F(h)); return h, l

lines = []
def macro(name, val, comment=''):
    lines.append('#define %-14s %s' % (name, fmt(val)) + ('   /* %s */' % comment if comment else ''))
def arr(name, vals, comment=''):
    lines.append('#define %-14s { %s }' % (name, ', '.join(fmt(v) for v in vals)) + ('   /* %s */' % comment if comment else ''))

# ---- polynomial fitting (Chebyshev interpolation, power basis) ----
def dcos(x):
    x = D(x); s = D(0); t = D(1); k = 0
    while abs(t) > D(10)**-300:
        s += t; t = -t*x*x/((2*k+1)*(2*k+2)); k += 1
    return s
def cheb_fit(f, a, b, deg):
    n = deg+1
    a = D(a); b = D(b)
    ts = [dcos(PI*(2*k+1)/(2*n)) for k in range(n)]
    xs = [(a+b)/2 + (b-a)/2*t for t in ts]
    fs = [f(x) for x in xs]
    # chebyshev coefficients
    c = []
    for j in range(n):
        s = sum(fs[k]*dcos(PI*j*(2*k+1)/(2*n)) for k in range(n))
        c.append(s*2/n)
    c[0] /= 2
    # T_j(t) as power series in t, then t = (2x-(a+b))/(b-a) = alpha x + beta
    T = [[D(1)], [D(0), D(1)]]
    for j in range(2, n):
        prev, pp = T[j-1], T[j-2]
        new = [D(0)]*(j+1)
        for i, v in enumerate(prev): new[i+1] += 2*v
        for i, v in enumerate(pp): new[i] -= v
        T.append(new)
    poly_t = [D(0)]*n
    for j in range(n):
        for i, v in enumerate(T[j]): poly_t[i] += c[j]*v
    alpha = 2/(b-a); beta = -(a+b)/(b-a)
    # substitute t = alpha x + beta
    res = [D(0)]*n
    base = [D(1)]
    for i in range(n):
        for k, v in enumerate(base): res[k] += poly_t[i]*v
        nb = [D(0)]*(len(base)+1)
        for k, v in enumerate(base):
            nb[k] += v*beta; nb[k+1] += v*alpha
        base = nb
    return res
def horner_f(coef, x):
    r = 0.0
    for c in reversed(coef): r = r*x + c
    return r
def horner_d(coef, x):
    r = D(0)
    for c in reversed(coef): r = r*x + c
    return r
def maxerr(f, coefd, a, b, w=lambda x: 1, npts=3000):
    coef = [float(c) for c in coefd]
    worst = 0
    for i in range(npts+1):
        x = float(D(a) + (D(b)-D(a))*i/npts)
        ref = f(D(x))
        got = D(horner_f(coef, x))
        worst = max(worst, abs((got-ref)*D(w(x))))
    return worst

# exp: Q(r) = (e^r - 1 - r)/r^2
def exp_q(x):
    s = D(0); t = D(1)/2; k = 2
    while abs(t) > D(10)**-100:
        s += t; t = t*x/(k+1); k += 1
    return s
# log tail: P(z) = 2/5 + 2z/7 + 2z^2/9 ...
def log_p(z):
    s = D(0); t = D(1); k = 0
    while t > D(10)**-100 or k < 5:
        s += 2*t/(2*k+5); t *= z; k += 1
        if k > 400: break
    return s
def sin_s(z):  # (sin r - r)/r^3 as series in z=r^2
    s = D(0); t = D(-1)/6; k = 0
    while abs(t) > D(10)**-100:
        s += t; t = -t*z/((2*k+4)*(2*k+5)); k += 1
    return s
def cos_c(z):  # (cos r - 1 + z/2)/z^2
    s = D(0); t = D(1)/24; k = 0
    while abs(t) > D(10)**-100:
        s += t; t = -t*z/((2*k+5)*(2*k+6)); k += 1
    return s
def sinh_s(z):
    s = D(0); t = D(1)/6; k = 0
    while abs(t) > D(10)**-100:
        s += t; t = t*z/((2*k+4)*(2*k+5)); k += 1
    return s
def cosh_c(z):  # (cosh a - 1 - z/2)/z^2
    s = D(0); t = D(1)/24; k = 0
    while abs(t) > D(10)**-100:
        s += t; t = t*z/((2*k+5)*(2*k+6)); k += 1
    return s
def atan_a(z):  # (atan u - u)/u^3 = -1/3 + z/5 - ...
    s = D(0); t = D(-1)/3; k = 0
    while abs(t) > D(10)**-100:
        s += t; t = -t*z*(2*k+3)/(2*k+5); k += 1
    return s

report = {}
def fit(name, f, a, b, deg, w):
    coefd = cheb_fit(f, a, b, deg)
    e = maxerr(f, coefd, a, b, w)
    report[name] = (deg, float(e))
    return [float(c) for c in coefd]

if __name__ == '__main__':
    lines.append('/* kc.h - constants of the math members, generated by scripts/gen-math-consts.py from')
    lines.append(' * the mathematics (Decimal arithmetic): Chebyshev interpolants of the series, the')
    lines.append(' * digits of pi, ln 2 and ln 10. Not hand edited. */')
    # exp
    macro('KC_INVLN2', todbl(1/LN2), '1/ln 2')
    p = parts(LN2, 40, 3)
    macro('KC_LN2A', p[0], 'ln 2, 40 bits'); macro('KC_LN2B', p[1], 'next 40 bits'); macro('KC_LN2C', p[2], 'the rest')
    h, l = dd(LN2); macro('KC_LN2H', h, 'ln 2 as a double'); macro('KC_LN2L', l, 'and its low part')
    hh, ll = dd(1/LN10); macro('KC_L10EH', hh, 'log10(e)'); macro('KC_L10EL', ll, 'low part')
    h, l = dd(PI/2); macro('KC_PIO2H', h, 'pi/2'); macro('KC_PIO2L', l)
    h, l = dd(PI); macro('KC_PIH', h, 'pi'); macro('KC_PIL', l)
    h, l = dd(PI/4); macro('KC_PIO4H', h, 'pi/4'); macro('KC_PIO4L', l)
    h, l = dd(3*PI/4); macro('KC_3PIO4H', h, '3pi/4'); macro('KC_3PIO4L', l)
    macro('KC_INVPIO2', float(2/PI), '2/pi')
    pp = parts(PI/2, 33, 4)
    macro('KC_PP1', pp[0], 'pi/2 in 33-bit parts'); macro('KC_PP2', pp[1]); macro('KC_PP3', pp[2]); macro('KC_PP4', pp[3])
    def dsplit(x):
        t = 134217729.0 * x; h = t - (t - x); return h, x - h
    for nm, v in (('KC_L10EH', 1/LN10), ('KC_PIO2H', PI/2)):
        h, l = dsplit(float(v)); macro(nm + '_H', h, 'Dekker split of %s (a compiler may fold a split of a constant)' % nm); macro(nm + '_L', l)
    # polynomials
    lim = float(LN2/2)*1.001
    arr('KC_EXPQ', fit('exp', exp_q, -lim, lim, 12, lambda x: x*x), 'Q(r) = (e^r-1-r)/r^2 on |r|<=ln2/2')
    arr('KC_LOGP', fit('log', log_p, 0, 0.0295, 6, lambda x: 1), 'P(z), z=s^2: 2s^3 atanh tail s*z^2*P(z)')
    arr('KC_SINS', fit('sin', sin_s, 0, 0.62, 7, lambda x: x**1.5*0+x*0+1), 'S(z)=(sin r - r)/r^3, z=r^2 <= pi^2/16')
    arr('KC_COSC', fit('cos', cos_c, 0, 0.62, 7, lambda x: 1), 'C(z)=(cos r - 1 + z/2)/z^2')
    arr('KC_SINHS', fit('sinh', sinh_s, 0, 1.0001, 10, lambda x: 1), 'S(z)=(sinh x - x)/x^3')
    arr('KC_COSHC', fit('cosh', cosh_c, 0, 1.0001, 9, lambda x: 1), 'C(z)=(cosh x - 1 - z/2)/z^2')
    arr('KC_ATANA', fit('atan', atan_a, 0, 1/256+1e-4, 6, lambda x: 1), 'A(z)=(atan u-u)/u^3, |u|<=1/16')
    # atan table
    th = []; tl = []
    for j in range(9):
        v = datan(D(j)/8) if j else D(0)
        h, l = dd(v); th.append(h); tl.append(l)
    arr('KC_ATANH', th, 'atan(j/8), j=0..8'); arr('KC_ATANL', tl, 'low parts')
    # 2/pi table: 1280 bits as 16-bit words
    bits = int((2/PI) * (D(2)**1280))
    words = [(bits >> (16*(79-i))) & 0xFFFF for i in range(80)]
    lines.append('#define KC_TWOPI_WORDS %d' % len(words))
    lines.append('#define KC_TWOPI { ' + ', '.join('0x%04X' % w for w in words) + ' }   /* 2/pi, 16 bits per word, first word = bits 1..16 after the point */')
    open(sys.argv[1], 'w').write('\n'.join(lines) + '\n')
    for k, v in report.items(): sys.stderr.write('%s: degree %d maxerr %.3g\n' % (k, v[0], v[1]))
