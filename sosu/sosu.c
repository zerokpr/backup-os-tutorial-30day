#include <stdio.h>
#include "roastd.h"

#define MAX 10000

void RoaMain(void) {
	char not_prime[MAX], s[8];
	int i, j;
	for(i = 0; i < MAX; ++i) not_prime[i] = 0;
	for (i = 2; i < MAX; ++i) {
		if (not_prime[i]) continue;
		sprintf(s, "%d ", i);
		api_putstr(s);
		for (j = i * 2; j < MAX; j += i) not_prime[j] = 1;
	}

	api_end();
}
