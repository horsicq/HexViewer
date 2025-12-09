/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2019 */

#if defined(CAPSTONE_HAS_OSXKERNEL)
#include <Availability.h>
#include <libkern/libkern.h>
#else
#include <stdlib.h>
#endif
#include <string.h>

#include "utils.h"

static unsigned short *make_id2insn(const insn_map *insns, unsigned int size)
{
	unsigned short max_id = insns[size - 1].id;
	unsigned short i;

	unsigned short *cache = (unsigned short *)cs_mem_calloc(max_id + 1, sizeof(*cache));

	for (i = 1; i < size; i++)
		cache[insns[i].id] = i;

	return cache;
}

unsigned short insn_find(const insn_map *insns, unsigned int max, unsigned int id, unsigned short **cache)
{
	if (id > insns[max - 1].id)
		return 0;

	if (*cache == NULL)
		*cache = make_id2insn(insns, max);

	return (*cache)[id];
}

int name2id(const name_map* map, int max, const char *name)
{
	int i;

	for (i = 0; i < max; i++) {
		if (!strcmp(map[i].name, name)) {
			return map[i].id;
		}
	}

	return -1;
}

const char *id2name(const name_map* map, int max, const unsigned int id)
{
	int i;

	for (i = 0; i < max; i++) {
		if (map[i].id == id) {
			return map[i].name;
		}
	}

	return NULL;
}

unsigned int count_positive(const uint16_t *list)
{
	unsigned int c;

	for (c = 0; list[c] > 0; c++);

	return c;
}

unsigned int count_positive8(const unsigned char *list)
{
	unsigned int c;

	for (c = 0; list[c] > 0; c++);

	return c;
}

char *cs_strdup(const char *str)
{
	size_t len = strlen(str)+ 1;
	void *new = cs_mem_malloc(len);

	if (new == NULL)
		return NULL;

	return (char *)memmove(new, str, len);
}

int cs_snprintf(char *buffer, size_t size, const char *fmt, ...)
{
	int ret;

	va_list ap;
	va_start(ap, fmt);
	ret = cs_vsnprintf(buffer, size, fmt, ap);
	va_end(ap);

	return ret;
}

bool arr_exist8(unsigned char *arr, unsigned char max, unsigned int id)
{
	int i;

	for (i = 0; i < max; i++) {
		if (arr[i] == id)
			return true;
	}

	return false;
}

bool arr_exist(uint16_t *arr, unsigned char max, unsigned int id)
{
	int i;

	for (i = 0; i < max; i++) {
		if (arr[i] == id)
			return true;
	}

	return false;
}

unsigned int binsearch_IndexTypeEncoding(const struct IndexType *index, size_t size, uint16_t encoding)
{
	size_t left, right, m;

	right = size - 1;

	if (encoding < index[0].encoding || encoding > index[right].encoding)
		return -1;

	left = 0;

	while(left <= right) {
		m = (left + right) / 2;
		if (encoding == index[m].encoding) {
			return m;
		}

		if (encoding < index[m].encoding)
			right = m - 1;
		else
			left = m + 1;
	}

	return -1;
}
