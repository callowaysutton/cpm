#include "liba.h"
#include "sht.h"
#include <stdio.h>

int liba_add(int a, int b) {
    int result = a + b;
    printf("liba_add: %d + %d = %d\n", a, b, result);
    return result;
}

int liba_multiply(int a, int b) {
    int result = a * b;
    printf("liba_multiply: %d * %d = %d\n", a, b, result);
    return result;
}
