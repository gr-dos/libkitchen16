/* sprintf with integer, string and character conversions only, and no
 * floating-point operation in the program: it must link no floating-point
 * helper (tests/fmt/members lists exactly the members it links), and it
 * uses nothing of libc16 but the startup, so a helper that the engine
 * called for its own use would show in the map. */

#include <kitchen16.h>
#include "check.h"

static char buf[200];

static int same(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static int len(const char *s)
{
    int n = 0;

    while (s[n])
        n++;
    return n;
}

/* the call-site line is the one reported */
#define TEST(call, want) \
    do { \
        int r_ = (call); \
        CHECK(same(buf, want)); \
        CHECK(r_ == len(want)); \
    } while (0)

int main(void)
{
    TEST(sprintf(buf, "%d|%5d|%-5d|%05d|%+d|% d", 42, 42, 42, 42, 42, 42), "42|   42|42   |00042|+42| 42");
    TEST(sprintf(buf, "%i %u %o %x %X %#o %#x", -7, 65535u, 8, 255, 255, 8, 255), "-7 65535 10 ff FF 010 0xff");
    TEST(sprintf(buf, "%ld %lu %lx %lo", -2147483647L - 1, 4294967295UL, 3735928559UL, 8UL), "-2147483648 4294967295 deadbeef 10");
    TEST(sprintf(buf, "[%s][%10s][%-10s][%.2s]", "abc", "abc", "abc", "abc"), "[abc][       abc][abc       ][ab]");
    TEST(sprintf(buf, "%c%c%c %5c|%-5c|", 'a', 'b', 'c', 'd', 'e'), "abc     d|e    |");
    TEST(sprintf(buf, "%*d|%-*d|%.*d|%*.*d", 6, 7, 6, 7, 6, 7, 8, 4, 7), "     7|7     |000007|    0007");
    TEST(sprintf(buf, "100%% %n %lld", 1), "100% %n %lld");
    TEST(sprintf(buf, "%s", (char *)0), "(null)");
    TEST(sprintf(buf, "%.0d|%#.0o|%.3d|%8.3d", 0, 0, 5, -5), "|0|005|    -005");
    TEST(sprintf(buf, ""), "");
    return done();
}
