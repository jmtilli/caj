#ifndef _CAJ_RBTREE_H_
#define _CAJ_RBTREE_H_

#include <errno.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct caj_rb_tree_node {
  int is_black;
  struct caj_rb_tree_node *left;
  struct caj_rb_tree_node *right;
  struct caj_rb_tree_node *parent;
};

typedef int (*caj_rb_tree_cmp)(struct caj_rb_tree_node *a, struct caj_rb_tree_node *b, void *ud);

struct caj_rb_tree_nocmp {
  struct caj_rb_tree_node *root;
};

struct caj_rb_tree {
  struct caj_rb_tree_nocmp nocmp;
  caj_rb_tree_cmp cmp;
  void *cmp_userdata;
};

static inline void caj_rb_tree_nocmp_init(struct caj_rb_tree_nocmp *tree)
{
  tree->root = NULL;
}

static inline void caj_rb_tree_init(struct caj_rb_tree *tree, caj_rb_tree_cmp cmp, void *cmp_userdata)
{
  caj_rb_tree_nocmp_init(&tree->nocmp);
  tree->cmp = cmp;
  tree->cmp_userdata = cmp_userdata;
}

static inline struct caj_rb_tree_node *caj_rb_tree_nocmp_root(struct caj_rb_tree_nocmp *tree)
{
  return tree->root;
}

static inline struct caj_rb_tree_node *caj_rb_tree_root(struct caj_rb_tree *tree)
{
  return caj_rb_tree_nocmp_root(&tree->nocmp);
}

int caj_rb_tree_nocmp_valid(struct caj_rb_tree_nocmp *tree);

static inline int caj_rb_tree_valid(struct caj_rb_tree *tree)
{
  return caj_rb_tree_nocmp_valid(&tree->nocmp);
}

struct caj_rb_tree_node *caj_rb_tree_nocmp_leftmost(struct caj_rb_tree_nocmp *tree);

static inline struct caj_rb_tree_node *caj_rb_tree_leftmost(struct caj_rb_tree *tree)
{
  return caj_rb_tree_nocmp_leftmost(&tree->nocmp);
}

struct caj_rb_tree_node *caj_rb_tree_nocmp_rightmost(struct caj_rb_tree_nocmp *tree);

static inline struct caj_rb_tree_node *caj_rb_tree_rightmost(struct caj_rb_tree *tree)
{
  return caj_rb_tree_nocmp_rightmost(&tree->nocmp);
}

void caj_rb_tree_nocmp_insert_repair(struct caj_rb_tree_nocmp *tree, struct caj_rb_tree_node *node);

static inline void caj_rb_tree_insert_repair(struct caj_rb_tree *tree, struct caj_rb_tree_node *node)
{
  caj_rb_tree_nocmp_insert_repair(&tree->nocmp, node);
}

void caj_rb_tree_insert(struct caj_rb_tree *tree, struct caj_rb_tree_node *node);

void caj_rb_tree_nocmp_delete(struct caj_rb_tree_nocmp *tree, struct caj_rb_tree_node *node);

static inline void caj_rb_tree_delete(struct caj_rb_tree *tree, struct caj_rb_tree_node *node)
{
  caj_rb_tree_nocmp_delete(&tree->nocmp, node);
}

#define CAJ_RB_TREE_NOCMP_FIND(tree, cmp, cmp_userdata, tofind) \
  ({ \
    const struct caj_rb_tree_nocmp *__caj_rb_tree_find_tree = (tree); \
    struct caj_rb_tree_node *__caj_rb_tree_find_node = __caj_rb_tree_find_tree->root; \
    while (__caj_rb_tree_find_node != NULL) \
    { \
      int __caj_rb_tree_find_res = \
        (cmp)((tofind), __caj_rb_tree_find_node, (cmp_userdata)); \
      if (__caj_rb_tree_find_res < 0) \
      { \
        __caj_rb_tree_find_node = __caj_rb_tree_find_node->left; \
      } \
      else if (__caj_rb_tree_find_res > 0) \
      { \
        __caj_rb_tree_find_node = __caj_rb_tree_find_node->right; \
      } \
      else \
      { \
        break; \
      } \
    } \
    __caj_rb_tree_find_node; \
  })

/*
 * NB: this is slower than the macro version
 */
static inline struct caj_rb_tree_node *caj_rb_tree_nocmp_find(
  struct caj_rb_tree_nocmp *tree, caj_rb_tree_cmp cmp, void *cmp_userdata,
  struct caj_rb_tree_node *tofind)
{
  struct caj_rb_tree_node *node = tree->root;
  while (node != NULL)
  {
    int res = cmp(tofind, node, cmp_userdata);
    if (res < 0)
    {
      node = node->left;
    }
    else if (res > 0)
    {
      node = node->right;
    }
    else
    {
      break;
    }
  }
  return node;
}

static inline int caj_rb_tree_nocmp_insert_nonexist(
  struct caj_rb_tree_nocmp *tree, caj_rb_tree_cmp cmp, void *cmp_userdata,
  struct caj_rb_tree_node *toinsert)
{
  void *cmp_ud = cmp_userdata;
  struct caj_rb_tree_node *node = tree->root;
  int finalres = 0;

  toinsert->is_black = 0;
  toinsert->left = NULL;
  toinsert->right = NULL;
  if (node == NULL)
  {
    tree->root = toinsert;
    toinsert->parent = NULL;
    caj_rb_tree_nocmp_insert_repair(tree, toinsert);
  }
  while (node != NULL)
  {
    int res = cmp(toinsert, node, cmp_ud);
    if (res < 0)
    {
      if (node->left == NULL)
      {
        node->left = toinsert;
        toinsert->parent = node;
        caj_rb_tree_nocmp_insert_repair(tree, toinsert);
        break;
      }
      node = node->left;
    }
    else if (res > 0)
    {
      if (node->right == NULL)
      {
        node->right = toinsert;
        toinsert->parent = node;
        caj_rb_tree_nocmp_insert_repair(tree, toinsert);
        break;
      }
      node = node->right;
    }
    else
    {
      finalres = -EEXIST;
      break;
    }
  }
  return finalres;
}

/*
 * NB: this is slower than the non-macro version
 */
#define CAJ_RB_TREE_NOCMP_INSERT_NONEXIST(tree, cmp, cmp_userdata, toinsert) \
  ({ \
    struct caj_rb_tree_nocmp *__caj_rb_tree_insert_tree = (tree); \
    caj_rb_tree_cmp __caj_rb_tree_insert_cmp = (cmp); \
    void *__caj_rb_tree_insert_cmp_ud = (cmp_userdata); \
    struct caj_rb_tree_node *__caj_rb_tree_insert_toinsert = (toinsert); \
    struct caj_rb_tree_node *__caj_rb_tree_insert_node = __caj_rb_tree_insert_tree->root; \
    int __caj_rb_tree_insert_finalres = 0; \
    __caj_rb_tree_insert_toinsert->is_black = 0; \
    __caj_rb_tree_insert_toinsert->left = NULL; \
    __caj_rb_tree_insert_toinsert->right = NULL; \
    if (__caj_rb_tree_insert_node == NULL) \
    { \
      __caj_rb_tree_insert_tree->root = __caj_rb_tree_insert_toinsert; \
      __caj_rb_tree_insert_toinsert->parent = NULL; \
      caj_rb_tree_nocmp_insert_repair(__caj_rb_tree_insert_tree, \
                                  __caj_rb_tree_insert_toinsert); \
    } \
    while (__caj_rb_tree_insert_node != NULL) \
    { \
      int __caj_rb_tree_insert_res = \
        __caj_rb_tree_insert_cmp(__caj_rb_tree_insert_toinsert, __caj_rb_tree_insert_node, \
                             __caj_rb_tree_insert_cmp_ud); \
      if (__caj_rb_tree_insert_res < 0) \
      { \
        if (__caj_rb_tree_insert_node->left == NULL) \
        { \
          __caj_rb_tree_insert_node->left = __caj_rb_tree_insert_toinsert; \
          __caj_rb_tree_insert_toinsert->parent = __caj_rb_tree_insert_node; \
          caj_rb_tree_nocmp_insert_repair(__caj_rb_tree_insert_tree, \
                                      __caj_rb_tree_insert_toinsert); \
          break; \
        } \
        __caj_rb_tree_insert_node = __caj_rb_tree_insert_node->left; \
      } \
      else if (__caj_rb_tree_insert_res > 0) \
      { \
        if (__caj_rb_tree_insert_node->right == NULL) \
        { \
          __caj_rb_tree_insert_node->right = __caj_rb_tree_insert_toinsert; \
          __caj_rb_tree_insert_toinsert->parent = __caj_rb_tree_insert_node; \
          caj_rb_tree_nocmp_insert_repair(__caj_rb_tree_insert_tree, \
                                      __caj_rb_tree_insert_toinsert); \
          break; \
        } \
        __caj_rb_tree_insert_node = __caj_rb_tree_insert_node->right; \
      } \
      else \
      { \
        __caj_rb_tree_insert_finalres = -EEXIST; \
        break; \
      } \
    } \
    __caj_rb_tree_insert_finalres; \
  })

#ifdef __cplusplus
};
#endif

#endif
