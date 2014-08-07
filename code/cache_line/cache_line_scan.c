#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int *arr = malloc(sizeof(int) * 1024 * 1024 * 1024 );
    for (int i = 0, j = 0; i < 10000000; i++){ 
        arr[j] *= 3;
        j += 1;
        //j += 16;
        //j + 16比j + 1要慢很多 因为每次都是访问下一个cache line
    }
    return 0;
}
