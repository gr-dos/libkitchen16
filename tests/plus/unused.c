/* A library member that nothing references, with code and data. Adding it
 * to LIBK16.LIB must leave every test program byte-identical (DESIGN.md,
 * section 5; tests/check-selective). */

int libkitchen16_unused_table[64] = { 1 };

int libkitchen16_unused(int i)
{
    return libkitchen16_unused_table[i & 63];
}
