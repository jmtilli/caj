#ifndef _CAJUN_H_
#define _CAJUN_H_

#include "cajrbtree.h"
#include "cajlinkedlist.h"
#include "cajmurmur.h"
#include "cajcontainerof.h"
#include "../caj.h"
#include <stdint.h>
#include <stdlib.h>

enum cajun_type {
	CAJUN_DICT,
	CAJUN_ARRAY,
	CAJUN_STRING,
	CAJUN_NUMBER,
	CAJUN_BOOL,
	CAJUN_NULL
};
struct cajun_node {
	char *key;
	size_t keysz;
	enum cajun_type type;
	struct caj_rb_tree_node node;
	struct caj_linked_list_node llnode;
	union {
		struct {
			struct caj_rb_tree_nocmp heads[8];
			struct caj_linked_list_head llhead;
		} dict;
		struct {
			struct cajun_node **nodes;
			size_t nodesz;
			size_t nodecap;
		} array;
		struct {
			char *s;
			size_t sz;
		} string;
		struct {
			double d;
		} number;
		struct {
			int b;
		} boolean;
	} u;
};

static inline void cajun_node_init(struct cajun_node *n)
{
	memset(n, 0, sizeof(*n));
	n->key = NULL;
	n->keysz = 0;
	caj_linked_list_node_init(&n->llnode);
	n->type = CAJUN_NULL;
}

static inline struct cajun_node *cajun_node_new(void)
{
	struct cajun_node *result;
	result = malloc(sizeof(*result));
	if (result == NULL)
	{
		return NULL;
	}
	cajun_node_init(result);
	return result;
}

static inline void cajun_null_init(struct cajun_node *n)
{
	cajun_node_init(n);
	n->type = CAJUN_NULL;
}

static inline int cajun_string_init(struct cajun_node *n, const char *val, size_t valsz)
{
	char *val2;
	cajun_node_init(n);
	val2 = malloc(valsz + 1);
	if (val2 == NULL)
	{
		return -ENOMEM;
	}
	memcpy(val2, val, valsz);
	val2[valsz] = '\0';
	n->type = CAJUN_STRING;
	n->u.string.s = val2;
	n->u.string.sz = valsz;
	return 0;
}

static inline void cajun_array_init(struct cajun_node *n)
{
	cajun_node_init(n);
	n->type = CAJUN_ARRAY;
	n->u.array.nodes = NULL;
	n->u.array.nodesz = 0;
	n->u.array.nodecap = 0;
}

static inline void cajun_number_init(struct cajun_node *n, double d)
{
	cajun_node_init(n);
	n->type = CAJUN_NUMBER;
	n->u.number.d = d;
}

static inline void cajun_boolean_init(struct cajun_node *n, int b)
{
	cajun_node_init(n);
	n->type = CAJUN_BOOL;
	n->u.boolean.b = !!b;
}

static inline void cajun_dict_init(struct cajun_node *n)
{
	size_t i;
	cajun_node_init(n);
	n->type = CAJUN_DICT;
	caj_linked_list_head_init(&n->u.dict.llhead);
	for (i = 0; i < sizeof(n->u.dict.heads)/sizeof(*n->u.dict.heads); i++)
	{
		caj_rb_tree_nocmp_init(&n->u.dict.heads[i]);
	}
}

static inline uint32_t cajun_key_hash(const char *key, size_t keysz)
{
	return caj_murmur_buf(0x12345678U, key, keysz);
}
static inline uint32_t cajun_node_hash(struct cajun_node *n)
{
	return cajun_key_hash(n->key, n->keysz);
}

struct caj_string_plus_len
{
	const char *str;
	size_t len;
};

int cajun_node_cmp_asym(struct caj_string_plus_len *a, struct caj_rb_tree_node *b, void *ud);

int cajun_node_cmp(struct caj_rb_tree_node *a, struct caj_rb_tree_node *b, void *ud);

struct cajun_node *cajun_dict_get(struct cajun_node *n, const char *key, size_t keysz);

enum cajun_null_mode {
	CAJUN_FORBID_NONE = 0,
	CAJUN_FORBID_NULL,
	CAJUN_FORBID_MISSING,
	CAJUN_FORBID_BOTH,
};

static inline struct cajun_node *cajun_dict_get_object(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get(dict, key, keysz);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			abort();
		}
		return NULL;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			abort();
		}
		return n;
	}
	return n;
}

static inline struct cajun_node *cajun_dict_get_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	return n;
}

int cajun_dict_add(struct cajun_node *parent, const char *key, size_t keysz, struct cajun_node *child);

int cajun_array_add(struct cajun_node *parent, struct cajun_node *child);

struct cajun_ctx {
	struct cajun_node *n;
	struct cajun_node **ns;
	size_t nsz;
	size_t ncap;
};

static inline void cajun_ctx_init(struct cajun_ctx *ctx)
{
	ctx->n = NULL;
	ctx->ns = NULL;
	ctx->nsz = 0;
	ctx->ncap = 0;
}

int cajun_start_dict(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_end_dict(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_start_array(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_end_array(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_handle_null(struct caj_handler *cajh, const char *key, size_t keysz);
int cajun_handle_string(struct caj_handler *cajh, const char *key, size_t keysz, const char *val, size_t valsz);
int cajun_handle_number(struct caj_handler *cajh, const char *key, size_t keysz, double d);
int cajun_handle_boolean(struct caj_handler *cajh, const char *key, size_t keysz, int b);



// Boolean

static inline int cajun_dict_get_boolean_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_BOOL)
	{
		abort();
	}
	return n->u.boolean.b;
}

static inline int cajun_dict_is_boolean(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get(dict, key, keysz);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_BOOL;
}

static inline int cajun_dict_get_boolean(struct cajun_node *dict, const char *key, size_t keysz, int default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		return default_value;
	}
	if (n->type != CAJUN_BOOL)
	{
		abort();
	}
	return n->u.boolean.b;
}

// NULL

static inline int cajun_dict_is_null(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get(dict, key, keysz);
	if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
	{
		abort();
	}
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_NULL;
}

// String
static inline const char *cajun_dict_get_string_not_null(struct cajun_node *dict, const char *key, size_t keysz, size_t *valsz)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_STRING)
	{
		abort();
	}
	if (valsz)
	{
		*valsz = n->u.string.sz;
	}
	return n->u.string.s;
}

static inline const char *cajun_dict_get_string_object(struct cajun_node *dict, const char *key, size_t keysz, size_t *valsz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		if (valsz)
		{
			*valsz = 0;
		}
		return NULL;
	}
	if (n->type != CAJUN_STRING)
	{
		abort();
	}
	if (valsz)
	{
		*valsz = n->u.string.sz;
	}
	return n->u.string.s;
}

static inline int cajun_dict_is_string(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get(dict, key, keysz);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_STRING;
}

static inline const char *cajun_dict_get_string(struct cajun_node *dict, const char *key, size_t keysz, const char *default_value, enum cajun_null_mode null_mode, size_t *valsz)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		if (valsz)
		{
			*valsz = strlen(default_value);
		}
		return default_value;
	}
	if (n->type != CAJUN_STRING)
	{
		abort();
	}
	if (valsz)
	{
		*valsz = n->u.string.sz;
	}
	return n->u.string.s;
}

static inline const char *cajun_dict_get_string_len(struct cajun_node *dict, const char *key, size_t keysz, const char *default_value, size_t default_sz, enum cajun_null_mode null_mode, size_t *valsz)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		if (valsz)
		{
			*valsz = default_sz;
		}
		return default_value;
	}
	if (n->type != CAJUN_STRING)
	{
		abort();
	}
	if (valsz)
	{
		*valsz = n->u.string.sz;
	}
	return n->u.string.s;
}

// Array
static inline struct cajun_node *cajun_dict_get_array_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_ARRAY)
	{
		abort();
	}
	return n;
}

static inline struct cajun_node *cajun_dict_get_array_object(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		return NULL;
	}
	if (n->type != CAJUN_ARRAY)
	{
		abort();
	}
	return n;
}

static inline int cajun_dict_is_array(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get(dict, key, keysz);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_ARRAY;
}

// Dict
static inline struct cajun_node *cajun_dict_get_dict_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_DICT)
	{
		abort();
	}
	return n;
}

static inline struct cajun_node *cajun_dict_get_dict_object(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		return NULL;
	}
	if (n->type != CAJUN_DICT)
	{
		abort();
	}
	return n;
}

static inline int cajun_dict_is_dict(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get(dict, key, keysz);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_DICT;
}

// Basic number object
static inline struct cajun_node *cajun_dict_get_number_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_NUMBER)
	{
		abort();
	}
	return n;
}

static inline struct cajun_node *cajun_dict_get_number_object(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_object(dict, key, keysz, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		return NULL;
	}
	if (n->type != CAJUN_NUMBER)
	{
		abort();
	}
	return n;
}

static inline int cajun_dict_is_number(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get(dict, key, keysz);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_NUMBER;
}

// Double
static inline double cajun_dict_get_double_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	return n->u.number.d;
}

static inline int cajun_dict_is_double(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	return cajun_dict_is_number(dict, key, keysz, null_mode);
}

static inline double cajun_dict_get_double(struct cajun_node *dict, const char *key, size_t keysz, double default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	return n->u.number.d;
}

// Float
static inline float cajun_dict_get_float_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	return (float)n->u.number.d;
}

static inline int cajun_dict_is_float(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	return cajun_dict_is_number(dict, key, keysz, null_mode);
}

static inline float cajun_dict_get_float(struct cajun_node *dict, const char *key, size_t keysz, float default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	return (float)n->u.number.d;
}

// uint8
static inline uint8_t cajun_dict_get_uint8_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(uint8_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint8_t)n->u.number.d;
}

static inline int cajun_dict_is_uint8(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(uint8_t)n->u.number.d == n->u.number.d;
}

static inline uint8_t cajun_dict_get_uint8(struct cajun_node *dict, const char *key, size_t keysz, uint8_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(uint8_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint8_t)n->u.number.d;
}

// uint16
static inline uint16_t cajun_dict_get_uint16_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(uint16_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint16_t)n->u.number.d;
}

static inline int cajun_dict_is_uint16(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(uint16_t)n->u.number.d == n->u.number.d;
}

static inline uint16_t cajun_dict_get_uint16(struct cajun_node *dict, const char *key, size_t keysz, uint16_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(uint16_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint16_t)n->u.number.d;
}

// uint32
static inline uint32_t cajun_dict_get_uint32_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(uint32_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint32_t)n->u.number.d;
}

static inline int cajun_dict_is_uint32(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(uint32_t)n->u.number.d == n->u.number.d;
}

static inline uint32_t cajun_dict_get_uint32(struct cajun_node *dict, const char *key, size_t keysz, uint32_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(uint32_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint32_t)n->u.number.d;
}

// uint64
static inline uint64_t cajun_dict_get_uint64_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(uint64_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint64_t)n->u.number.d;
}

static inline int cajun_dict_is_uint64(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(uint64_t)n->u.number.d == n->u.number.d;
}

static inline uint64_t cajun_dict_get_uint64(struct cajun_node *dict, const char *key, size_t keysz, uint64_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(uint64_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint64_t)n->u.number.d;
}

// int8
static inline int8_t cajun_dict_get_int8_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(int8_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int8_t)n->u.number.d;
}

static inline int cajun_dict_is_int8(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int8_t)n->u.number.d == n->u.number.d;
}

static inline int8_t cajun_dict_get_int8(struct cajun_node *dict, const char *key, size_t keysz, int8_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int8_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int8_t)n->u.number.d;
}

// int16
static inline int16_t cajun_dict_get_int16_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(int16_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int16_t)n->u.number.d;
}

static inline int cajun_dict_is_int16(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int16_t)n->u.number.d == n->u.number.d;
}

static inline int16_t cajun_dict_get_int16(struct cajun_node *dict, const char *key, size_t keysz, int16_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int16_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int16_t)n->u.number.d;
}

// int32
static inline int32_t cajun_dict_get_int32_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(int32_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int32_t)n->u.number.d;
}

static inline int cajun_dict_is_int32(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int32_t)n->u.number.d == n->u.number.d;
}

static inline int32_t cajun_dict_get_int32(struct cajun_node *dict, const char *key, size_t keysz, int32_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int32_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int32_t)n->u.number.d;
}

// int64
static inline int64_t cajun_dict_get_int64_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(int64_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int64_t)n->u.number.d;
}

static inline int cajun_dict_is_int64(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int64_t)n->u.number.d == n->u.number.d;
}

static inline int64_t cajun_dict_get_int64(struct cajun_node *dict, const char *key, size_t keysz, int64_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int64_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int64_t)n->u.number.d;
}

// int
static inline int cajun_dict_get_int_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(int)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int)n->u.number.d;
}

static inline int cajun_dict_is_int(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int)n->u.number.d == n->u.number.d;
}

static inline int cajun_dict_get_int(struct cajun_node *dict, const char *key, size_t keysz, int default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int)n->u.number.d;
}

// unsigned
static inline unsigned cajun_dict_get_unsigned_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(unsigned)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (unsigned)n->u.number.d;
}

static inline int cajun_dict_is_unsigned(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(unsigned)n->u.number.d == n->u.number.d;
}

static inline unsigned cajun_dict_get_unsigned(struct cajun_node *dict, const char *key, size_t keysz, unsigned default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(unsigned)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (unsigned)n->u.number.d;
}

// short
static inline short cajun_dict_get_short_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(short)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (short)n->u.number.d;
}

static inline short cajun_dict_is_short(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(short)n->u.number.d == n->u.number.d;
}

static inline short cajun_dict_get_short(struct cajun_node *dict, const char *key, size_t keysz, short default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(short)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (short)n->u.number.d;
}

// ushort
static inline unsigned short cajun_dict_get_ushort_not_null(struct cajun_node *dict, const char *key, size_t keysz)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, CAJUN_FORBID_BOTH);
	if ((double)(unsigned short)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (unsigned short)n->u.number.d;
}

static inline int cajun_dict_is_ushort(struct cajun_node *dict, const char *key, size_t keysz, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_dict_is_number(dict, key, keysz, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(unsigned short)n->u.number.d == n->u.number.d;
}

static inline unsigned short cajun_dict_get_ushort(struct cajun_node *dict, const char *key, size_t keysz, unsigned short default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_dict_get_number_object(dict, key, keysz, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(unsigned short)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (unsigned short)n->u.number.d;
}

// Basic array
static inline size_t cajun_array_size(struct cajun_node *ar)
{
	if (ar->type != CAJUN_ARRAY)
	{
		abort();
	}
	return ar->u.array.nodesz;
}

static inline struct cajun_node *cajun_array_get(struct cajun_node *ar, size_t idx)
{
	if (ar->type != CAJUN_ARRAY)
	{
		abort();
	}
	if (idx >= ar->u.array.nodesz)
	{
		abort();
	}
	return ar->u.array.nodes[idx];
}

static inline struct cajun_node *cajun_array_get_object(struct cajun_node *ar, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get(ar, idx);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			abort();
		}
		return NULL;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			abort();
		}
		return n;
	}
	return n;
}

static inline struct cajun_node *cajun_array_get_not_null(struct cajun_node *ar, size_t idx)
{
	struct cajun_node *n = cajun_array_get_object(ar, idx, CAJUN_FORBID_BOTH);
	return n;
}

// Array Boolean

static inline int cajun_array_get_boolean_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_BOOL)
	{
		abort();
	}
	return n->u.boolean.b;
}

static inline int cajun_array_is_boolean(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get(dict, idx);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_BOOL;
}

static inline int cajun_array_get_boolean(struct cajun_node *dict, size_t idx, int default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		return default_value;
	}
	if (n->type != CAJUN_BOOL)
	{
		abort();
	}
	return n->u.boolean.b;
}

// Array NULL

static inline int cajun_array_is_null(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get(dict, idx);
	if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
	{
		abort();
	}
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_NULL;
}

// Array String
static inline const char *cajun_array_get_string_not_null(struct cajun_node *dict, size_t idx, size_t *valsz)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_STRING)
	{
		abort();
	}
	if (valsz)
	{
		*valsz = n->u.string.sz;
	}
	return n->u.string.s;
}

static inline const char *cajun_array_get_string_object(struct cajun_node *dict, size_t idx, size_t *valsz, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		if (valsz)
		{
			*valsz = 0;
		}
		return NULL;
	}
	if (n->type != CAJUN_STRING)
	{
		abort();
	}
	if (valsz)
	{
		*valsz = n->u.string.sz;
	}
	return n->u.string.s;
}

static inline int cajun_array_is_string(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get(dict, idx);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_STRING;
}

static inline const char *cajun_array_get_string(struct cajun_node *dict, size_t idx, const char *default_value, enum cajun_null_mode null_mode, size_t *valsz)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		if (valsz)
		{
			*valsz = strlen(default_value);
		}
		return default_value;
	}
	if (n->type != CAJUN_STRING)
	{
		abort();
	}
	if (valsz)
	{
		*valsz = n->u.string.sz;
	}
	return n->u.string.s;
}

static inline const char *cajun_array_get_string_len(struct cajun_node *dict, size_t idx, const char *default_value, size_t default_sz, enum cajun_null_mode null_mode, size_t *valsz)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		if (valsz)
		{
			*valsz = default_sz;
		}
		return default_value;
	}
	if (n->type != CAJUN_STRING)
	{
		abort();
	}
	if (valsz)
	{
		*valsz = n->u.string.sz;
	}
	return n->u.string.s;
}

// Array Array
static inline struct cajun_node *cajun_array_get_array_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_ARRAY)
	{
		abort();
	}
	return n;
}

static inline struct cajun_node *cajun_array_get_array_object(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		return NULL;
	}
	if (n->type != CAJUN_ARRAY)
	{
		abort();
	}
	return n;
}

static inline int cajun_array_is_array(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get(dict, idx);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_ARRAY;
}

// Array Dict
static inline struct cajun_node *cajun_array_get_dict_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_DICT)
	{
		abort();
	}
	return n;
}

static inline struct cajun_node *cajun_array_get_dict_object(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		return NULL;
	}
	if (n->type != CAJUN_DICT)
	{
		abort();
	}
	return n;
}

static inline int cajun_array_is_dict(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get(dict, idx);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_DICT;
}

// Array Basic number object
static inline struct cajun_node *cajun_array_get_number_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, CAJUN_FORBID_BOTH);
	if (n->type != CAJUN_NUMBER)
	{
		abort();
	}
	return n;
}

static inline struct cajun_node *cajun_array_get_number_object(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_object(dict, idx, null_mode);
	if (n == NULL || n->type == CAJUN_NULL)
	{
		return NULL;
	}
	if (n->type != CAJUN_NUMBER)
	{
		abort();
	}
	return n;
}

static inline int cajun_array_is_number(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get(dict, idx);
	if (n == NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_MISSING)
		{
			return 0;
		}
		return 1;
	}
	if (n->type == CAJUN_NULL)
	{
		if (null_mode == CAJUN_FORBID_BOTH || null_mode == CAJUN_FORBID_NULL)
		{
			return 0;
		}
		return 1;
	}
	return n->type == CAJUN_NUMBER;
}

// Array Double
static inline double cajun_array_get_double_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	return n->u.number.d;
}

static inline int cajun_array_is_double(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	return cajun_array_is_number(dict, idx, null_mode);
}

static inline double cajun_array_get_double(struct cajun_node *dict, size_t idx, double default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	return n->u.number.d;
}

// Array Float
static inline float cajun_array_get_float_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	return (float)n->u.number.d;
}

static inline int cajun_array_is_float(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	return cajun_array_is_number(dict, idx, null_mode);
}

static inline float cajun_array_get_float(struct cajun_node *dict, size_t idx, float default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	return (float)n->u.number.d;
}

// Array uint8
static inline uint8_t cajun_array_get_uint8_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(uint8_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint8_t)n->u.number.d;
}

static inline int cajun_array_is_uint8(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(uint8_t)n->u.number.d == n->u.number.d;
}

static inline uint8_t cajun_array_get_uint8(struct cajun_node *dict, size_t idx, uint8_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(uint8_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint8_t)n->u.number.d;
}

// Array uint16
static inline uint16_t cajun_array_get_uint16_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(uint16_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint16_t)n->u.number.d;
}

static inline int cajun_array_is_uint16(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(uint16_t)n->u.number.d == n->u.number.d;
}

static inline uint16_t cajun_array_get_uint16(struct cajun_node *dict, size_t idx, uint16_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(uint16_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint16_t)n->u.number.d;
}

// Array uint32
static inline uint32_t cajun_array_get_uint32_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(uint32_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint32_t)n->u.number.d;
}

static inline int cajun_array_is_uint32(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(uint32_t)n->u.number.d == n->u.number.d;
}

static inline uint32_t cajun_array_get_uint32(struct cajun_node *dict, size_t idx, uint32_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(uint32_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint32_t)n->u.number.d;
}

// Array uint64
static inline uint64_t cajun_array_get_uint64_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(uint64_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint64_t)n->u.number.d;
}

static inline int cajun_array_is_uint64(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(uint64_t)n->u.number.d == n->u.number.d;
}

static inline uint64_t cajun_array_get_uint64(struct cajun_node *dict, size_t idx, uint64_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(uint64_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (uint64_t)n->u.number.d;
}

// Array int8
static inline int8_t cajun_array_get_int8_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(int8_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int8_t)n->u.number.d;
}

static inline int cajun_array_is_int8(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int8_t)n->u.number.d == n->u.number.d;
}

static inline int8_t cajun_array_get_int8(struct cajun_node *dict, size_t idx, int8_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int8_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int8_t)n->u.number.d;
}

// Array int16
static inline int16_t cajun_array_get_int16_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(int16_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int16_t)n->u.number.d;
}

static inline int cajun_array_is_int16(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int16_t)n->u.number.d == n->u.number.d;
}

static inline int16_t cajun_array_get_int16(struct cajun_node *dict, size_t idx, int16_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int16_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int16_t)n->u.number.d;
}

// Array int32
static inline int32_t cajun_array_get_int32_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(int32_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int32_t)n->u.number.d;
}

static inline int cajun_array_is_int32(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int32_t)n->u.number.d == n->u.number.d;
}

static inline int32_t cajun_array_get_int32(struct cajun_node *dict, size_t idx, int32_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int32_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int32_t)n->u.number.d;
}

// Array int64
static inline int64_t cajun_array_get_int64_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(int64_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int64_t)n->u.number.d;
}

static inline int cajun_array_is_int64(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int64_t)n->u.number.d == n->u.number.d;
}

static inline int64_t cajun_array_get_int64(struct cajun_node *dict, size_t idx, int64_t default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int64_t)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int64_t)n->u.number.d;
}

// Array int
static inline int cajun_array_get_int_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(int)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int)n->u.number.d;
}

static inline int cajun_array_is_int(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(int)n->u.number.d == n->u.number.d;
}

static inline int cajun_array_get_int(struct cajun_node *dict, size_t idx, int default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(int)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (int)n->u.number.d;
}

// Array unsigned
static inline unsigned cajun_array_get_unsigned_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(unsigned)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (unsigned)n->u.number.d;
}

static inline int cajun_array_is_unsigned(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(unsigned)n->u.number.d == n->u.number.d;
}

static inline unsigned cajun_array_get_unsigned(struct cajun_node *dict, size_t idx, unsigned default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(unsigned)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (unsigned)n->u.number.d;
}

// Array short
static inline short cajun_array_get_short_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(short)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (short)n->u.number.d;
}

static inline short cajun_array_is_short(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(short)n->u.number.d == n->u.number.d;
}

static inline short cajun_array_get_short(struct cajun_node *dict, size_t idx, short default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(short)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (short)n->u.number.d;
}

// Array ushort
static inline unsigned short cajun_array_get_ushort_not_null(struct cajun_node *dict, size_t idx)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, CAJUN_FORBID_BOTH);
	if ((double)(unsigned short)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (unsigned short)n->u.number.d;
}

static inline int cajun_array_is_ushort(struct cajun_node *dict, size_t idx, enum cajun_null_mode null_mode)
{
	 struct cajun_node *n;
	 if (!cajun_array_is_number(dict, idx, null_mode))
	 {
		 return 0;
	 }
	 n = cajun_array_get_number_object(dict, idx, null_mode);
	 if (n == NULL || n->type == CAJUN_NULL)
	 {
		 return 1;
	 }
	 return (double)(unsigned short)n->u.number.d == n->u.number.d;
}

static inline unsigned short cajun_array_get_ushort(struct cajun_node *dict, size_t idx, unsigned short default_value, enum cajun_null_mode null_mode)
{
	struct cajun_node *n = cajun_array_get_number_object(dict, idx, null_mode);
	if (n == NULL)
	{
		return default_value;
	}
	if ((double)(unsigned short)n->u.number.d != n->u.number.d)
	{
		abort();
	}
	return (unsigned short)n->u.number.d;
}

#endif
