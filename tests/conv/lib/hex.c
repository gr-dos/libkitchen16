#include "testio.h"
#include "hex.h"

void t_puthex16(unsigned u)
{
    char s[5];
    int i;

    for (i = 3; i >= 0; i--) {
        s[i] = "0123456789ABCDEF"[u & 15];
        u >>= 4;
    }
    s[4] = '\0';
    t_puts(s);
}
