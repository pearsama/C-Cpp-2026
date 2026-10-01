#include <stdio.h>
#include <string.h>
#include <time.h>
#define MAX 1000

int main(void)
{
    clock_t start, end;
    double cost_time;
    int is_prime[MAX+1];
    memset(is_prime, 1, sizeof(is_prime));
    is_prime[0] = is_prime[1] = 0;

    start = clock();

    for(int i = 2; i*i <= MAX; i++)
    {
        if(is_prime[i])
        {
            for(int j = i*i; j <= MAX; j += i)
                is_prime[j] = 0;
        }
    }

    end = clock();
    cost_time = (double)(end - start) / CLOCKS_PER_SEC;

    printf("2~1000之间的质数：\n");
    for(int i = 2; i <= MAX; i++)
    {
        if(is_prime[i])
            printf("%d ", i);
    }

    printf("\n\n计算耗时：%.6f 秒\n", cost_time);
    return 0;
}

