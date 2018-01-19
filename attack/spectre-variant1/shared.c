#include "common.h"

const struct arr_type probe_array[10] __attribute__((aligned (64))) = {
	[0] = {
		.val1 = 10,
		.val2 = 20,
	},
};

const unsigned int arr_bound1 = 0;
const unsigned int arr_bound2 = 10;

unsigned int get_probe_array_val1(int idx)
{
	return probe_array[idx].val1;
}

