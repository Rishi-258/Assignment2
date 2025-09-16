#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_terms>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);  // convert string to int
    if (n <= 0) {
        printf("Please enter a positive number of terms.\n");
        return 1;
    }

    long long a = 0, b = 1, next;

    printf("Fibonacci series up to %d terms:\n", n);

    for (int i = 1; i <= n; i++) {
        printf("%lld ", a);
        next = a + b;
        a = b;
        b = next;
    }

    printf("\n");
    return 0;
}
