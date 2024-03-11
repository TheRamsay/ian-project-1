#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Values
int a_int = 132432110;
short b_short = 4324;
char c_char = 'a';
char d_char = 102;

// Uninitialized
int e_int_uninit;

// Zero initialized
short f_short_zero = 0;

int main() {
    printf("a_int: %d, b_short: %d, c_char: %c, d_char: %c, e_int_uninit: %d, f_short_zero: %d\n", a_int, b_short, c_char, d_char, e_int_uninit, f_short_zero);
    return 0;
}