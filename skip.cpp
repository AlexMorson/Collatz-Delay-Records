#include <cmath>
#include <stdio.h>

unsigned int  k;
unsigned int* c;
unsigned int* d;
unsigned int* e;

void fillMem() {
	printf("Starting memory fill.\n");
	for (unsigned int i=0; i<2<<(k-1); ++i) {
		unsigned int n = i;
		unsigned int odds = 0;
		for (unsigned int j=0; j<k; ++j) {
			if (n & 1) {
				++odds;
				n = (3*n+1)/2;
			} else {
				n /= 2;
			}
		}
		c[i] = pow(3, odds);
		d[i] = n;
		e[i] = odds;
	}
	printf("Memory filled.\n");
}

unsigned int slowCollatz(unsigned int n) {
	unsigned int l = 0;
	while (n != 1) {
		if (n & 1) {
			n = (3*n+1)/2;
			l += 2;
		} else {
			n /= 2;
			++l;
		}
	}
	return l;
}

unsigned int fastCollatz(unsigned int n) {
	unsigned int l = 0;
	unsigned int a;
	unsigned int b;
	while (1) {
		a = n >> k;
		b = n & ((1<<k)-1);
		n = a*c[b] + d[b];
		l += k + e[b];

		if (n <= 4) return l + 7;
	}
}

void findRecordsFast() {
	unsigned int n = 1;
	unsigned int record = 0;

	while (1) {
		if (fastCollatz(n) >= record) {
			unsigned int l = slowCollatz(n);
			if (l > record) {
				record = l;
				printf("%u with length %u\n", n, l);
			}
		}
		++n;
	}
}

void findRecordsSlow() {
	unsigned int n = 1;
	unsigned int record = 0;

	while (1) {
		unsigned int l = slowCollatz(n);
		if (l > record) {
			record = l;
			printf("%u with length %u\n", n, l);
				++n;}

	}
}

int main() {
	k = 23;
	c = new unsigned int[1<<k];
	d = new unsigned int[1<<k];
	e = new unsigned int[1<<k];

	fillMem();
	findRecordsFast();

	return 0;
}