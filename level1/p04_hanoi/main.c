#include <stdio.h>

/* 把 n 个盘子从 from 移到 to，借助 aux */
void hanoi(int n, char from, char to, char aux)
{
    if (n == 1)
    {
        printf("%c -> %c\n", from, to);
        return;
    }
    hanoi(n - 1, from, aux, to);   /* 先把上面 n-1 个挪到辅助柱 */
    printf("%c -> %c\n", from, to);
    hanoi(n - 1, aux, to, from);   /* 再把 n-1 个从辅助柱挪回目标柱 */
}

int main(void)
{
    int n;
    printf("Enter the number of disks: ");
    if (scanf("%d", &n) != 1 || n < 1)
    {
        printf("Invalid input, using n = 3 by default.\n");
        n = 3;
    }
    hanoi(n, 'A', 'C', 'B');
    return 0;
}
