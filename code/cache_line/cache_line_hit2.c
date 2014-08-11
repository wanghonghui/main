#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>


int main(int argc, char **argv)
{
    char *arr = malloc(sizeof(char) * 6 * 1024 * 1024);
    printf("arr %p\n", arr);
    arr = (char *)((unsigned long)arr & ~(64-1)) + 64;
    printf("arr %p\n", arr);
    int j = 0;
    for (int i = 0; i < 500000000; i++){ 
        arr[j] += i;
        j += 512 * 64;
        if (j >= 6 * 1024 * 1024) {
            j = 0;
        }
    }
    return 0;
}
