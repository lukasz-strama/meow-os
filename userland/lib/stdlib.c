#include "stdlib.h"

static unsigned int next = 1;

void srand(unsigned int seed) {
    next = seed;
}

int rand() {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}
