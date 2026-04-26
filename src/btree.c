#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "btree.h"

typedef struct BTreeNode BTreeNode;

static BTreeNode *node_new(BTree *tree, const void *value);
static void node_free(BTree *tree, BTreeNode *node);
static void node_detach(BTree *tree, BTreeNode *node);
static void noop_free(void *value);
static const void *get_value_key(const void *value);
BTreeNode **get_parent_child_ref(BTree *tree, BTreeNode *node);

static BTreeNode *get_node_by_key(BTree *tree, const void *key);
static BTreeNode *get_first_node_inorder(BTreeNode *node);
static BTreeNode *get_next_node_inorder(BTreeNode *node);
static BTreeNode *get_first_node_postorder(BTreeNode *node);
static BTreeNode *get_next_node_postorder(BTreeNode *node);

struct BTree
{
    BTreeNode *root;
    size_t count;
    size_t element_size;
    BTreeGetKeyFunction get_key;
    BTreeCompareKeyFunction compare_keys;
    BTreeFreeValueFunction free_value;
};

struct BTreeNode
{
    BTreeNode *left;
    BTreeNode *right;
    BTreeNode *parent;
    max_align_t align;
    unsigned char value[];
};

BTreeError btree_new(BTree **tree, size_t element_size, BTreeCompareKeyFunction compare_keys, BTreeGetKeyFunction get_key, BTreeFreeValueFunction free_value)
{
    if (tree == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    *tree = NULL;

    if (element_size == 0 || element_size > SIZE_MAX - sizeof(BTreeNode))
    {
        return BTREE_INVALID_ELEMENT_SIZE;
    }

    if (compare_keys == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    *tree = malloc(sizeof **tree);

    if (*tree == NULL)
    {
        return BTREE_OUT_OF_MEMORY;
    }

    (*tree)->root = NULL;
    (*tree)->count = 0;
    (*tree)->element_size = element_size;
    (*tree)->get_key = get_key != NULL ? get_key : get_value_key;
    (*tree)->compare_keys = compare_keys;
    (*tree)->free_value = free_value != NULL ? free_value : noop_free;

    return BTREE_OK;
}

void btree_free(BTree *tree)
{
    if (tree == NULL)
    {
        return;
    }

    btree_clear(tree);
    free(tree);
}

BTreeError btree_clear(BTree *tree)
{
    if (tree == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    if (tree->count == 0)
    {
        return BTREE_OK;
    }

    BTreeNode *node = get_first_node_postorder(tree->root);

    while (node != NULL)
    {
        BTreeNode *next = get_next_node_postorder(node);

        node_detach(tree, node);
        node_free(tree, node);

        node = next;
    }

    tree->root = NULL;
    tree->count = 0;

    return BTREE_OK;
}

BTreeError btree_getcount(BTree *tree, size_t *count)
{
    if (tree == NULL || count == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    *count = tree->count;

    return BTREE_OK;
}

BTreeError btree_toarray(BTree *tree, void *buffer, size_t length)
{
    if (tree == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    if (length < tree->count)
    {
        return BTREE_BUFFER_TOO_SMALL;
    }

    if (tree->count == 0)
    {
        return BTREE_OK;
    }

    if (buffer == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    unsigned char *bytes = buffer;

    BTreeNode *node = get_first_node_inorder(tree->root);
    while (node != NULL)
    {
        memcpy(bytes, node->value, tree->element_size);
        bytes += tree->element_size;

        node = get_next_node_inorder(node);
    }

    return BTREE_OK;
}

BTreeError btree_exists(BTree *tree, const void *key, bool *exists)
{
    if (tree == NULL || key == NULL || exists == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    BTreeNode *node = get_node_by_key(tree, key);

    *exists = node != NULL;

    return BTREE_OK;
}

BTreeError btree_get(BTree *tree, const void *key, void **value)
{
    if (tree == NULL || key == NULL || value == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    BTreeNode *node = get_node_by_key(tree, key);

    if (node == NULL)
    {
        *value = NULL;
        return BTREE_VALUE_NOT_FOUND;
    }

    *value = node->value;

    return BTREE_OK;
}

BTreeError btree_add(BTree *tree, const void *value)
{
    if (tree == NULL || value == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    if (tree->count == SIZE_MAX)
    {
        return BTREE_COUNT_OVERFLOW;
    }

    const void *value_key = tree->get_key(value);

    if (value_key == NULL)
    {
        return BTREE_NULL_KEY;
    }

    BTreeNode *parent = NULL;
    BTreeNode **node_ref = &tree->root;
    while (*node_ref != NULL)
    {
        parent = *node_ref;

        const void *key = tree->get_key((*node_ref)->value);
        int comparison = tree->compare_keys(value_key, key);
        if (comparison <= 0)
        {
            node_ref = &(*node_ref)->left;
        }
        else
        {
            node_ref = &(*node_ref)->right;
        }
    }

    BTreeNode *new_node = node_new(tree, value);

    if (new_node == NULL)
    {
        return BTREE_OUT_OF_MEMORY;
    }

    new_node->parent = parent;

    *node_ref = new_node;

    tree->count++;

    return BTREE_OK;
}

BTreeError btree_remove(BTree *tree, const void *key)
{
    if (tree == NULL || key == NULL)
    {
        return BTREE_NULL_POINTER_ARGUMENT;
    }

    if (tree->count == 0)
    {
        return BTREE_VALUE_NOT_FOUND;
    }

    BTreeNode *node = get_node_by_key(tree, key);

    if (node == NULL)
    {
        return BTREE_VALUE_NOT_FOUND;
    }

    node_detach(tree, node);
    node_free(tree, node);

    tree->count--;

    return BTREE_OK;
}

BTreeNode *node_new(BTree *tree, const void *value)
{
    BTreeNode *node = malloc(sizeof *node + tree->element_size);

    if (node == NULL)
    {
        return NULL;
    }

    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    memcpy(node->value, value, tree->element_size);

    return node;
}

void node_free(BTree *tree, BTreeNode *node)
{
    tree->free_value(node->value);
    free(node);
}

void node_detach(BTree *tree, BTreeNode *node)
{
    BTreeNode **node_ref = get_parent_child_ref(tree, node);

    if (node->left == NULL)
    {
        *node_ref = node->right;
    }
    else if (node->right == NULL)
    {
        *node_ref = node->left;
    }
    else
    {
        BTreeNode *replacement = node->left;
        while (replacement->right != NULL)
        {
            replacement = replacement->right;
        }

        node_detach(tree, replacement);

        replacement->left = node->left;
        if (replacement->left != NULL)
        {
            replacement->left->parent = replacement;
        }

        replacement->right = node->right;
        if (replacement->right != NULL)
        {
            replacement->right->parent = replacement;
        }

        *node_ref = replacement;
    }

    if (*node_ref != NULL)
    {
        (*node_ref)->parent = node->parent;
    }

    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
}

void noop_free(void *value)
{
    (void)value;
}

const void *get_value_key(const void *value)
{
    return value;
}

BTreeNode **get_parent_child_ref(BTree *tree, BTreeNode *node)
{
    BTreeNode *parent = node->parent;

    if (parent == NULL)
    {
        return &tree->root;
    }

    if (node == parent->left)
    {
        return &parent->left;
    }

    return &parent->right;
}

BTreeNode *get_node_by_key(BTree *tree, const void *key)
{
    BTreeNode *node = tree->root;

    int comparison;
    while (node != NULL && (comparison = tree->compare_keys(key, tree->get_key(node->value))) != 0)
    {
        if (comparison < 0)
        {
            node = node->left;
        }
        else
        {
            node = node->right;
        }
    }

    return node;
}

BTreeNode *get_first_node_inorder(BTreeNode *node)
{
    while (node->left != NULL)
    {
        node = node->left;
    }

    return node;
}

BTreeNode *get_next_node_inorder(BTreeNode *node)
{
    if (node->right != NULL)
    {
        return get_first_node_inorder(node->right);
    }

    while (node->parent != NULL && node == node->parent->right)
    {
        node = node->parent;
    }

    return node->parent;
}

BTreeNode *get_first_node_postorder(BTreeNode *node)
{
    while (node->left != NULL || node->right != NULL)
    {
        if (node->left != NULL)
        {
            node = node->left;
        }
        else
        {
            node = node->right;
        }
    }

    return node;
}

BTreeNode *get_next_node_postorder(BTreeNode *node)
{
    BTreeNode *parent = node->parent;

    if (parent == NULL)
    {
        return NULL;
    }

    if (node == parent->left && parent->right != NULL)
    {
        return get_first_node_postorder(parent->right);
    }

    return parent;
}
