/***********************************************************************************************
created: 		2022-12-17

author:			chensong

purpose:		bmem
************************************************************************************************/


#ifndef _C_B_MEM_H
#define _C_B_MEM_H

//#include "c99defs.h"
//#include "base.h"
#include <wchar.h>
#include <string.h>

#define EXPORT 

#ifdef __cplusplus
extern "C" {
#endif

struct base_allocator {
	void *(*malloc)(size_t);
	void *(*realloc)(void *, size_t);
	void (*free)(void *);
};

EXPORT void base_set_allocator(struct base_allocator *defs);

EXPORT void *bmalloc(size_t size);
EXPORT void *brealloc(void *ptr, size_t size);
EXPORT void bfree(void *ptr);

EXPORT int base_get_alignment(void);

EXPORT long bnum_allocs(void);

EXPORT void *bmemdup(const void *ptr, size_t size);

static inline void *bzalloc(size_t size)
{
	void *mem = bmalloc(size);
	if (mem)
		memset(mem, 0, size);
	return mem;
}

static inline char *bstrdup_n(const char *str, size_t n)
{
	char *dup;
	if (!str)
		return NULL;

	dup = (char *)bmemdup(str, n + 1);
	dup[n] = 0;

	return dup;
}

static inline wchar_t *bwstrdup_n(const wchar_t *str, size_t n)
{
	wchar_t *dup;
	if (!str)
		return NULL;

	dup = (wchar_t *)bmemdup(str, (n + 1) * sizeof(wchar_t));
	dup[n] = 0;

	return dup;
}

static inline char *bstrdup(const char *str)
{
	if (!str)
		return NULL;

	return bstrdup_n(str, strlen(str));
}

static inline wchar_t *bwstrdup(const wchar_t *str)
{
	if (!str)
		return NULL;

	return bwstrdup_n(str, wcslen(str));
}

#ifdef __cplusplus
}
#endif

#endif // _C_B_MEM_H
