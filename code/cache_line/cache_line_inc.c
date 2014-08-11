#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

struct test{
    int a;
    char x[64];
    int b;
};

int main(int argc, char **argv)
{
    struct test t;
    printf("a:%p, b:%p\n", &t.a, &t.b);
    for (unsigned long i = 0; i < 10000000000; i++) {
        t.a++;
        t.b++;
    }
    return 0;
}
