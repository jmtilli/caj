#ifndef _CAJUN_H_
#define _CAJUN_H_

#include "cajrbtree.h"
#include "cajlinkedlist.h"
#include "cajmurmur.h"
#include "cajcontainerof.h"
#include <stdint.h>

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
	union {
		struct {
			struct caj_rb_tree_nocmp heads[8];
			struct caj_linked_list_head llhead;
		} dict;
		struct {
			struct cajun_node *nodes;
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

struct cajun_node *cajun_dict_get(struct cajun_node *n, char *key, size_t keysz);

#endif
