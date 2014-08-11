#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

int g = 0;

int main(int argc, char **argv)
{
    unsigned long *arr = malloc(sizeof(unsigned long) * 2 * 1024 * 1024 * 10);
    int j = 0;
    for (int i = 0; i < 10000000; i++){ 
        arr[j]++;
        j += 4096;
        //每次j加4096，这样所有的访问会在同一个set内导致cache冲突
        //j++;
        //j &= (4096-1);
        if ( j >= 4096 * 12) {
            j = 0;
        }
    }
    return 0;
}
