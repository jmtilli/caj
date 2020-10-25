#include "cajrbtree.h"
#include "cajlinkedlist.h"
#include "cajun.h"
#include <stdlib.h>

int cajun_node_cmp_asym(struct caj_string_plus_len *a, struct caj_rb_tree_node *b, void *ud)
{
	struct cajun_node *n_b = CAJ_CONTAINER_OF(b, struct cajun_node, node);
	size_t s_a = a->len;
	size_t s_b = n_b->keysz;
	size_t s_min = (s_a < s_b ? s_a : s_b);
	const char *b_a = a->str;
	const char *b_b = n_b->key;
	int res;
	res = memcmp(b_a, b_b, s_min);
	if (res != 0)
	{
		return res;
	}
	if (s_a < s_b)
	{
		return -1;
	}
	if (s_a > s_b)
	{
		return 1;
	}
	return 0;
}

int cajun_node_cmp(struct caj_rb_tree_node *a, struct caj_rb_tree_node *b, void *ud)
{
	struct cajun_node *n_a = CAJ_CONTAINER_OF(a, struct cajun_node, node);
	struct cajun_node *n_b = CAJ_CONTAINER_OF(b, struct cajun_node, node);
	size_t s_a = n_a->keysz;
	size_t s_b = n_b->keysz;
	size_t s_min = (s_a < s_b ? s_a : s_b);
	const char *b_a = n_a->key;
	const char *b_b = n_b->key;
	int res;
	res = memcmp(b_a, b_b, s_min);
	if (res != 0)
	{
		return res;
	}
	if (s_a < s_b)
	{
		return -1;
	}
	if (s_a > s_b)
	{
		return 1;
	}
	return 0;
}

struct cajun_node *cajun_dict_get(struct cajun_node *n, const char *key, size_t keysz)
{
	uint32_t hashval = cajun_key_hash(key, keysz);
	size_t hashloc = hashval % (sizeof(n->u.dict.heads)/sizeof(*n->u.dict.heads));
	struct caj_rb_tree_node *rn;
	struct caj_string_plus_len stringlen = {.str = key, .len = keysz};
	if (n->type != CAJUN_DICT)
	{
		abort();
	}
	rn = CAJ_RB_TREE_NOCMP_FIND(&n->u.dict.heads[hashloc], cajun_node_cmp_asym, NULL, &stringlen);
	if (rn == NULL)
	{
		return NULL;
	}
	return CAJ_CONTAINER_OF(rn, struct cajun_node, node);
}

int cajun_dict_add(struct cajun_node *parent, const char *key, size_t keysz, struct cajun_node *child)
{
	uint32_t hashval = cajun_key_hash(key, keysz);
	size_t hashloc = hashval % (sizeof(parent->u.dict.heads)/sizeof(*parent->u.dict.heads));
	struct caj_rb_tree_node *rn;
	struct caj_string_plus_len stringlen = {.str = key, .len = keysz};
	char *key2;
	int ret;
	if (parent->type != CAJUN_DICT || child->key != NULL || child->keysz != 0)
	{
		abort();
	}
	rn = CAJ_RB_TREE_NOCMP_FIND(&parent->u.dict.heads[hashloc], cajun_node_cmp_asym, NULL, &stringlen);
	if (rn != NULL)
	{
		return -EEXIST;
	}
	key2 = malloc(keysz + 1);
	if (key2 == NULL)
	{
		return -ENOMEM;
	}
	memcpy(key2, key, keysz);
	key2[keysz] = '\0';
	child->key = key2;
	child->keysz = keysz;
	ret = caj_rb_tree_nocmp_insert_nonexist(&parent->u.dict.heads[hashloc], cajun_node_cmp, NULL, &child->node);
	if (ret != 0)
	{
		free(child->key);
		child->key = NULL;
		child->keysz = 0;
		return ret;
	}
	caj_linked_list_add_tail(&child->llnode, &parent->u.dict.llhead);
	return 0;
}
