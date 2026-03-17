#pragma once
#include <stdlib.h>
c = 0;
void my_free(int* p) {
	free(p);
	c++;
}