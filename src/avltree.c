#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "avltree.h"

typedef struct AvlTreeNode AvlTreeNode;

static AvlTreeNode *node_new(AvlTree *tree, const void *value);
static void node_free(AvlTree *tree, AvlTreeNode *node);
static void node_detach_full(AvlTree *tree, AvlTreeNode *node);
static AvlTreeNode *node_detach_simple(AvlTree *tree, AvlTreeNode *node);
static void noop_free(void *value);
static const void *get_value_key(const void *value);

static AvlTreeNode **get_parent_child_ref(AvlTree *tree, AvlTreeNode *node);
static AvlTreeNode *get_node_by_key(AvlTree *tree, const void *key);
static AvlTreeNode *get_first_node_inorder(AvlTreeNode *node);
static AvlTreeNode *get_next_node_inorder(AvlTreeNode *node);
static AvlTreeNode *get_first_node_postorder(AvlTreeNode *node);
static AvlTreeNode *get_next_node_postorder(AvlTreeNode *node);

static int get_node_height(AvlTreeNode *node);
static void update_height(AvlTreeNode *node);

static void rebalance_from(AvlTree *tree, AvlTreeNode *node);
static void rebalance_node(AvlTree *tree, AvlTreeNode *node);
static int get_balancing_factor(AvlTreeNode *node);
static void rotate_left(AvlTree *tree, AvlTreeNode *node);
static void rotate_right(AvlTree *tree, AvlTreeNode *node);
static void rotate_right_left(AvlTree *tree, AvlTreeNode *node);
static void rotate_left_right(AvlTree *tree, AvlTreeNode *node);

struct AvlTree
{
    AvlTreeNode *root;
    size_t count;
    size_t element_size;
    AvlTreeGetKeyFunction get_key;
    AvlTreeCompareKeyFunction compare_keys;
    AvlTreeFreeValueFunction free_value;
};

struct AvlTreeNode
{
    AvlTreeNode *left;
    AvlTreeNode *right;
    AvlTreeNode *parent;
    int height;
    max_align_t align;
    unsigned char value[];
};

AvlTreeError avltree_new(AvlTree **tree, size_t element_size, AvlTreeCompareKeyFunction compare_keys, AvlTreeGetKeyFunction get_key, AvlTreeFreeValueFunction free_value)
{
    if (tree == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    *tree = NULL;

    if (element_size == 0 || element_size > SIZE_MAX - sizeof(AvlTreeNode))
    {
        return AVLTREE_INVALID_ELEMENT_SIZE;
    }

    if (compare_keys == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    *tree = malloc(sizeof **tree);

    if (*tree == NULL)
    {
        return AVLTREE_OUT_OF_MEMORY;
    }

    (*tree)->root = NULL;
    (*tree)->count = 0;
    (*tree)->element_size = element_size;
    (*tree)->get_key = get_key != NULL ? get_key : get_value_key;
    (*tree)->compare_keys = compare_keys;
    (*tree)->free_value = free_value != NULL ? free_value : noop_free;

    return AVLTREE_OK;
}

void avltree_free(AvlTree *tree)
{
    if (tree == NULL)
    {
        return;
    }

    avltree_clear(tree);
    free(tree);
}

AvlTreeError avltree_clear(AvlTree *tree)
{
    if (tree == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    if (tree->count == 0)
    {
        return AVLTREE_OK;
    }

    AvlTreeNode *node = get_first_node_postorder(tree->root);

    while (node != NULL)
    {
        AvlTreeNode *next = get_next_node_postorder(node);
        node_free(tree, node);
        node = next;
    }

    tree->root = NULL;
    tree->count = 0;

    return AVLTREE_OK;
}

AvlTreeError avltree_getcount(AvlTree *tree, size_t *count)
{
    if (tree == NULL || count == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    *count = tree->count;

    return AVLTREE_OK;
}

AvlTreeError avltree_toarray(AvlTree *tree, void *buffer, size_t length)
{
    if (tree == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    if (length < tree->count)
    {
        return AVLTREE_BUFFER_TOO_SMALL;
    }

    if (tree->count == 0)
    {
        return AVLTREE_OK;
    }

    if (buffer == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    unsigned char *bytes = buffer;

    AvlTreeNode *node = get_first_node_inorder(tree->root);
    while (node != NULL)
    {
        memcpy(bytes, node->value, tree->element_size);
        bytes += tree->element_size;

        node = get_next_node_inorder(node);
    }

    return AVLTREE_OK;
}

AvlTreeError avltree_exists(AvlTree *tree, const void *key, bool *exists)
{
    if (tree == NULL || key == NULL || exists == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    AvlTreeNode *node = get_node_by_key(tree, key);

    *exists = node != NULL;

    return AVLTREE_OK;
}

AvlTreeError avltree_get(AvlTree *tree, const void *key, void **value)
{
    if (tree == NULL || key == NULL || value == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    AvlTreeNode *node = get_node_by_key(tree, key);

    if (node == NULL)
    {
        *value = NULL;
        return AVLTREE_VALUE_NOT_FOUND;
    }

    *value = node->value;

    return AVLTREE_OK;
}

AvlTreeError avltree_add(AvlTree *tree, const void *value)
{
    if (tree == NULL || value == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    if (tree->count == SIZE_MAX)
    {
        return AVLTREE_COUNT_OVERFLOW;
    }

    const void *value_key = tree->get_key(value);

    if (value_key == NULL)
    {
        return AVLTREE_NULL_KEY;
    }

    AvlTreeNode *parent = NULL;
    AvlTreeNode **node_ref = &tree->root;
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

    AvlTreeNode *new_node = node_new(tree, value);

    if (new_node == NULL)
    {
        return AVLTREE_OUT_OF_MEMORY;
    }

    new_node->parent = parent;

    *node_ref = new_node;

    rebalance_from(tree, new_node);

    tree->count++;

    return AVLTREE_OK;
}

AvlTreeError avltree_remove(AvlTree *tree, const void *key)
{
    if (tree == NULL || key == NULL)
    {
        return AVLTREE_NULL_POINTER_ARGUMENT;
    }

    if (tree->count == 0)
    {
        return AVLTREE_VALUE_NOT_FOUND;
    }

    AvlTreeNode *node = get_node_by_key(tree, key);

    if (node == NULL)
    {
        return AVLTREE_VALUE_NOT_FOUND;
    }

    node_detach_full(tree, node);
    node_free(tree, node);

    tree->count--;

    return AVLTREE_OK;
}

AvlTreeNode *node_new(AvlTree *tree, const void *value)
{
    AvlTreeNode *node = malloc(sizeof *node + tree->element_size);

    if (node == NULL)
    {
        return NULL;
    }

    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->height = 1;
    memcpy(node->value, value, tree->element_size);

    return node;
}

void node_free(AvlTree *tree, AvlTreeNode *node)
{
    tree->free_value(node->value);
    free(node);
}

void node_detach_full(AvlTree *tree, AvlTreeNode *node)
{
    AvlTreeNode *rebalance_node;

    if (node->left == NULL || node->right == NULL)
    {
        rebalance_node = node_detach_simple(tree, node);
        rebalance_from(tree, rebalance_node);
        return;
    }

    AvlTreeNode **node_ref = get_parent_child_ref(tree, node);
    AvlTreeNode *parent = node->parent;

    AvlTreeNode *replacement = node->left;
    while (replacement->right != NULL)
    {
        replacement = replacement->right;
    }

    rebalance_node = node_detach_simple(tree, replacement);

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

    replacement->parent = parent;
    *node_ref = replacement;

    update_height(replacement);

    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    if (rebalance_node != node)
    {
        rebalance_from(tree, rebalance_node);
    }

    rebalance_from(tree, replacement);
}

AvlTreeNode *node_detach_simple(AvlTree *tree, AvlTreeNode *node)
{
    assert(node->left == NULL || node->right == NULL);

    AvlTreeNode **node_ref = get_parent_child_ref(tree, node);
    AvlTreeNode *parent = node->parent;

    AvlTreeNode *child = node->left != NULL
                             ? node->left
                             : node->right;

    *node_ref = child;

    if (child != NULL)
    {
        child->parent = node->parent;
    }

    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    return parent;
}

void noop_free(void *value)
{
    (void)value;
}

const void *get_value_key(const void *value)
{
    return value;
}

AvlTreeNode **get_parent_child_ref(AvlTree *tree, AvlTreeNode *node)
{
    AvlTreeNode *parent = node->parent;
    AvlTreeNode **node_ref;
    if (parent == NULL)
    {
        node_ref = &tree->root;
    }
    else
    {
        node_ref = node == parent->left ? &parent->left : &parent->right;
    }

    return node_ref;
}

AvlTreeNode *get_node_by_key(AvlTree *tree, const void *key)
{
    AvlTreeNode *node = tree->root;

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

AvlTreeNode *get_first_node_inorder(AvlTreeNode *node)
{
    while (node->left != NULL)
    {
        node = node->left;
    }

    return node;
}

AvlTreeNode *get_next_node_inorder(AvlTreeNode *node)
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

AvlTreeNode *get_first_node_postorder(AvlTreeNode *node)
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

AvlTreeNode *get_next_node_postorder(AvlTreeNode *node)
{
    AvlTreeNode *parent = node->parent;

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

int get_node_height(AvlTreeNode *node)
{
    return node != NULL ? node->height : 0;
}

void update_height(AvlTreeNode *node)
{
    int left_height = get_node_height(node->left);
    int right_height = get_node_height(node->right);

    node->height = 1 + (left_height > right_height ? left_height : right_height);
}

void rebalance_from(AvlTree *tree, AvlTreeNode *node)
{
    while (node != NULL)
    {
        update_height(node);

        AvlTreeNode *parent = node->parent;

        rebalance_node(tree, node);

        node = parent;
    }
}

void rebalance_node(AvlTree *tree, AvlTreeNode *node)
{
    int factor = get_balancing_factor(node);

    int child_factor;
    if (factor > 1)
    {
        child_factor = get_balancing_factor(node->left);

        if (child_factor >= 0)
        {
            rotate_right(tree, node);
        }
        else
        {
            rotate_left_right(tree, node);
        }
    }
    else if (factor < -1)
    {
        child_factor = get_balancing_factor(node->right);

        if (child_factor <= 0)
        {
            rotate_left(tree, node);
        }
        else
        {
            rotate_right_left(tree, node);
        }
    }
}

int get_balancing_factor(AvlTreeNode *node)
{
    return get_node_height(node->left) - get_node_height(node->right);
}

void rotate_left(AvlTree *tree, AvlTreeNode *node)
{
    AvlTreeNode *parent = node->parent;
    AvlTreeNode **node_ref = get_parent_child_ref(tree, node);

    AvlTreeNode *child = node->right;

    *node_ref = child;
    child->parent = parent;

    node->right = child->left;
    if (child->left != NULL)
    {
        child->left->parent = node;
    }

    child->left = node;
    node->parent = child;

    update_height(node);
    update_height(child);
}

void rotate_right(AvlTree *tree, AvlTreeNode *node)
{
    AvlTreeNode *parent = node->parent;
    AvlTreeNode **node_ref = get_parent_child_ref(tree, node);

    AvlTreeNode *child = node->left;

    *node_ref = child;
    child->parent = parent;

    node->left = child->right;
    if (child->right != NULL)
    {
        child->right->parent = node;
    }

    child->right = node;
    node->parent = child;

    update_height(node);
    update_height(child);
}

void rotate_right_left(AvlTree *tree, AvlTreeNode *node)
{
    rotate_right(tree, node->right);
    rotate_left(tree, node);
}

void rotate_left_right(AvlTree *tree, AvlTreeNode *node)
{
    rotate_left(tree, node->left);
    rotate_right(tree, node);
}
