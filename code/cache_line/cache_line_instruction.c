#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int *arr = malloc(sizeof(int) * 2);
    for (int i = 0; i < 10000000; i++){ 
        arr[0]++;
        arr[0]++;
        //arr[1]++;
        //arr[1]的时候比arr[0]的时候要快很多，因为指令之间没有依赖，CPU可以做一些并行执行
    }
    return 0;
}
