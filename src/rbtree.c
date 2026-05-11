#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"

typedef struct RbTreeNode RbTreeNode;
typedef enum RbTreeNodeColor RbTreeNodeColor;

static RbTreeNode *node_new(RbTree *tree, const void *value);
static void node_detach_full(RbTree *tree, RbTreeNode *node);
static void node_detach_simple(RbTree *tree, RbTreeNode *node);
static const void *get_value_key(const void *value);

static RbTreeNode **get_parent_child_ref(RbTree *tree, RbTreeNode *node);
static RbTreeNode *get_node_by_key(RbTree *tree, const void *key);
static RbTreeNode *get_brother_node(RbTreeNode *node);
static void swap_colors(RbTreeNode *x, RbTreeNode *y);
static void rebalance_after_insert(RbTree *tree, RbTreeNode *node);
static void rebalance_after_remove(RbTree *tree, RbTreeNode *parent, RbTreeNode *node);
static RbTreeNode *get_first_node_inorder(RbTreeNode *node);
static RbTreeNode *get_next_node_inorder(RbTreeNode *node);
static RbTreeNode *get_first_node_postorder(RbTreeNode *node);
static RbTreeNode *get_next_node_postorder(RbTreeNode *node);

static void rotate_left(RbTree *tree, RbTreeNode *node);
static void rotate_right(RbTree *tree, RbTreeNode *node);
// static void rotate_right_left(RbTree *tree, RbTreeNode *node);
// static void rotate_left_right(RbTree *tree, RbTreeNode *node);

enum RbTreeNodeColor
{
    RED,
    BLACK
};

struct RbTree
{
    RbTreeNode *root;
    size_t count;
    size_t element_size;
    bool unique_keys;
    RbTreeGetKeyFunction get_key;
    RbTreeCompareKeyFunction compare_keys;
    size_t version;
};

struct RbTreeNode
{
    RbTreeNode *left;
    RbTreeNode *right;
    RbTreeNode *parent;
    RbTreeNodeColor color;
    max_align_t align;
    unsigned char value[];
};

struct RbTreeIterator
{
    RbTree *tree;
    RbTreeNode *current;
    RbTreeNode *next;
    size_t version;
};

RbTreeStatus rbtree_new(RbTree **tree, size_t element_size, bool unique_keys, RbTreeCompareKeyFunction compare_keys, RbTreeGetKeyFunction get_key)
{
    if (tree == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    *tree = NULL;

    if (element_size == 0 || element_size > SIZE_MAX - sizeof(RbTreeNode))
    {
        return RBTREE_INVALID_ELEMENT_SIZE;
    }

    if (compare_keys == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    RbTree *new_tree = malloc(sizeof **tree);

    if (new_tree == NULL)
    {
        return RBTREE_OUT_OF_MEMORY;
    }

    new_tree->root = NULL;
    new_tree->count = 0;
    new_tree->element_size = element_size;
    new_tree->unique_keys = unique_keys;
    new_tree->get_key = get_key != NULL ? get_key : get_value_key;
    new_tree->compare_keys = compare_keys;
    new_tree->version = 0;

    *tree = new_tree;

    return RBTREE_OK;
}

void rbtree_free(RbTree *tree, RbTreeFreeValueFunction free_function)
{
    if (tree == NULL)
    {
        return;
    }

    rbtree_clear(tree, free_function);
    free(tree);
}

RbTreeStatus rbtree_clear(RbTree *tree, RbTreeFreeValueFunction free_function)
{
    if (tree == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    if (tree->count == 0)
    {
        return RBTREE_OK;
    }

    RbTreeNode *node = get_first_node_postorder(tree->root);

    while (node != NULL)
    {
        RbTreeNode *next = get_next_node_postorder(node);

        if (free_function != NULL)
        {
            free_function(node->value);
        }

        free(node);

        node = next;
    }

    tree->root = NULL;
    tree->count = 0;
    tree->version++;

    return RBTREE_OK;
}

RbTreeStatus rbtree_getcount(RbTree *tree, size_t *count)
{
    if (tree == NULL || count == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    *count = tree->count;

    return RBTREE_OK;
}

RbTreeStatus rbtree_toarray(RbTree *tree, void *buffer, size_t length)
{
    if (tree == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    if (length < tree->count)
    {
        return RBTREE_BUFFER_TOO_SMALL;
    }

    if (tree->count == 0)
    {
        return RBTREE_OK;
    }

    if (buffer == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    unsigned char *bytes = buffer;

    RbTreeNode *node = get_first_node_inorder(tree->root);
    while (node != NULL)
    {
        memcpy(bytes, node->value, tree->element_size);
        bytes += tree->element_size;

        node = get_next_node_inorder(node);
    }

    return RBTREE_OK;
}

RbTreeStatus rbtree_exists(RbTree *tree, const void *key, bool *exists)
{
    if (tree == NULL || key == NULL || exists == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    RbTreeNode *node = get_node_by_key(tree, key);

    *exists = node != NULL;

    return RBTREE_OK;
}

RbTreeStatus rbtree_get(RbTree *tree, const void *key, void *value)
{
    if (tree == NULL || key == NULL || value == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    RbTreeNode *node = get_node_by_key(tree, key);

    if (node == NULL)
    {
        return RBTREE_KEY_NOT_FOUND;
    }

    memcpy(value, node->value, tree->element_size);

    return RBTREE_OK;
}

RbTreeStatus rbtree_add(RbTree *tree, const void *value)
{
    if (tree == NULL || value == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    if (tree->count == SIZE_MAX)
    {
        return RBTREE_COUNT_OVERFLOW;
    }

    const void *value_key = tree->get_key(value);

    if (value_key == NULL)
    {
        return RBTREE_NULL_KEY;
    }

    RbTreeNode *parent = NULL;
    RbTreeNode **node_ref = &tree->root;
    while (*node_ref != NULL)
    {
        parent = *node_ref;

        const void *key = tree->get_key((*node_ref)->value);
        int comparison = tree->compare_keys(value_key, key);
        if (comparison <= 0)
        {
            if (comparison == 0 && tree->unique_keys)
            {
                return RBTREE_KEY_ALREADY_EXISTS;
            }

            node_ref = &(*node_ref)->left;
        }
        else
        {
            node_ref = &(*node_ref)->right;
        }
    }

    RbTreeNode *new_node = node_new(tree, value);

    if (new_node == NULL)
    {
        return RBTREE_OUT_OF_MEMORY;
    }

    new_node->parent = parent;
    *node_ref = new_node;

    rebalance_after_insert(tree, new_node);

    tree->count++;
    tree->version++;

    return RBTREE_OK;
}

RbTreeStatus rbtree_remove(RbTree *tree, const void *key, void *value)
{
    if (tree == NULL || key == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    if (tree->count == 0)
    {
        return RBTREE_KEY_NOT_FOUND;
    }

    RbTreeNode *node = get_node_by_key(tree, key);

    if (node == NULL)
    {
        return RBTREE_KEY_NOT_FOUND;
    }

    node_detach_full(tree, node);

    if (value != NULL)
    {
        memcpy(value, node->value, tree->element_size);
    }

    free(node);

    tree->count--;
    tree->version++;

    return RBTREE_OK;
}

RbTreeStatus rbtree_iterator_new(RbTree *tree, RbTreeIterator **iterator)
{
    if (tree == NULL || iterator == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    *iterator = NULL;

    RbTreeIterator *new_iterator = malloc(sizeof(RbTreeIterator));

    if (new_iterator == NULL)
    {
        return RBTREE_OUT_OF_MEMORY;
    }

    new_iterator->tree = tree;
    new_iterator->current = NULL;
    new_iterator->next = get_first_node_inorder(tree->root);
    new_iterator->version = tree->version;

    *iterator = new_iterator;

    return RBTREE_OK;
}

void rbtree_iterator_free(RbTreeIterator *iterator)
{
    if (iterator == NULL)
    {
        return;
    }

    free(iterator);
}

RbTreeStatus rbtree_iterator_next(RbTreeIterator *iterator, void *value)
{
    if (iterator == NULL || value == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    if (iterator->version != iterator->tree->version)
    {
        return RBTREE_ITERATOR_INVALID;
    }

    iterator->current = iterator->next;

    if (iterator->current == NULL)
    {
        return RBTREE_ITERATOR_END;
    }

    memcpy(value, iterator->current->value, iterator->tree->element_size);

    iterator->next = get_next_node_inorder(iterator->current);

    return RBTREE_OK;
}

RbTreeStatus rbtree_iterator_getvalue(RbTreeIterator *iterator, void *value)
{
    if (iterator == NULL || value == NULL)
    {
        return RBTREE_NULL_POINTER_ARGUMENT;
    }

    if (iterator->version != iterator->tree->version)
    {
        return RBTREE_ITERATOR_INVALID;
    }

    if (iterator->current == NULL)
    {
        return RBTREE_ITERATOR_END;
    }

    memcpy(value, iterator->current->value, iterator->tree->element_size);

    return RBTREE_OK;
}

RbTreeNode *node_new(RbTree *tree, const void *value)
{
    RbTreeNode *node = malloc(sizeof *node + tree->element_size);

    if (node == NULL)
    {
        return NULL;
    }

    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->color = RED;
    memcpy(node->value, value, tree->element_size);

    return node;
}

void node_detach_full(RbTree *tree, RbTreeNode *node)
{
    RbTreeNode **node_ref = get_parent_child_ref(tree, node);

    if (node->color == RED && node->left == NULL && node->right == NULL)
    {
        *node_ref = NULL;

        node->left = NULL;
        node->right = NULL;
        node->parent = NULL;

        return;
    }

    if (node->color == BLACK && (node->left == NULL || node->right == NULL))
    {
        if (node->left != NULL && node->left->color == RED)
        {
            *node_ref = node->left;
            node->left = node->parent;

            node->left = NULL;
            node->right = NULL;
            node->parent = NULL;

            return;
        }

        if (node->right != NULL && node->right->color == RED)
        {
            *node_ref = node->right;
            node->right = node->parent;

            node->left = NULL;
            node->right = NULL;
            node->parent = NULL;

            return;
        }

        if (node->left == NULL && node->right == NULL)
        {
            *node_ref = NULL;

            // TODO: Implement rebalance

            node->left = NULL;
            node->right = NULL;
            node->parent = NULL;

            return;
        }
    }

    node_ref = get_parent_child_ref(tree, node);
    RbTreeNode *parent = node->parent;

    RbTreeNode *replacement = node->left;
    while (replacement->right != NULL)
    {
        replacement = replacement->right;
    }

    node_detach_simple(tree, replacement);

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

    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;

    if (replacement->color == BLACK)
    {
        // TODO: Implement
    }
}

void node_detach_simple(RbTree *tree, RbTreeNode *node)
{
    assert(node->left == NULL || node->right == NULL);

    RbTreeNode **node_ref = get_parent_child_ref(tree, node);
    RbTreeNode *parent = node->parent;

    RbTreeNode *child = node->left != NULL
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
}

const void *get_value_key(const void *value)
{
    return value;
}

RbTreeNode **get_parent_child_ref(RbTree *tree, RbTreeNode *node)
{
    RbTreeNode *parent = node->parent;
    RbTreeNode **node_ref;
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

RbTreeNode *get_node_by_key(RbTree *tree, const void *key)
{
    RbTreeNode *node = tree->root;

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

RbTreeNode *get_brother_node(RbTreeNode *node)
{
    RbTreeNode *parent = node->parent;
    if (parent == NULL)
    {
        return NULL;
    }

    if (parent->left == node)
    {
        return parent->right;
    }

    return parent->left;
}

void swap_colors(RbTreeNode *x, RbTreeNode *y)
{
    RbTreeNodeColor temp = x->color;
    x->color = y->color;
    y->color = temp;
}

void rebalance_after_insert(RbTree *tree, RbTreeNode *node)
{
    while (node != tree->root && node->parent != tree->root && node->parent->color == RED)
    {
        RbTreeNode *parent = node->parent;
        RbTreeNode *grandparent = parent->parent;
        RbTreeNode *uncle = get_brother_node(parent);

        if (uncle != NULL && uncle->color == RED)
        {
            parent->color = BLACK;
            uncle->color = BLACK;
            grandparent->color = RED;

            node = grandparent;
        }
        else if (grandparent->right == uncle)
        {
            if (parent->right == node)
            {
                rotate_left(tree, parent);
                node = parent;
                parent = node->parent;
            }

            rotate_right(tree, grandparent);
            swap_colors(grandparent, parent);

            node = parent;
        }
        else if (grandparent != NULL)
        {

            if (parent->left == node)
            {
                rotate_right(tree, parent);
                node = parent;
                parent = node->parent;
            }

            rotate_left(tree, grandparent);
            swap_colors(grandparent, parent);

            node = parent;
        }
    }

    if (tree->root != NULL)
    {
        tree->root->color = BLACK;
    }
}

void rebalance_after_remove(RbTree *tree, RbTreeNode *parent, RbTreeNode *node)
{
    while (true)
    {
    }
}

RbTreeNode *get_first_node_inorder(RbTreeNode *node)
{
    if (node == NULL)
    {
        return NULL;
    }

    while (node->left != NULL)
    {
        node = node->left;
    }

    return node;
}

RbTreeNode *get_next_node_inorder(RbTreeNode *node)
{
    if (node == NULL)
    {
        return NULL;
    }

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

RbTreeNode *get_first_node_postorder(RbTreeNode *node)
{
    if (node == NULL)
    {
        return NULL;
    }

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

RbTreeNode *get_next_node_postorder(RbTreeNode *node)
{
    if (node == NULL)
    {
        return NULL;
    }

    RbTreeNode *parent = node->parent;

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

void rotate_left(RbTree *tree, RbTreeNode *node)
{
    RbTreeNode *parent = node->parent;
    RbTreeNode **node_ref = get_parent_child_ref(tree, node);

    RbTreeNode *child = node->right;

    *node_ref = child;
    child->parent = parent;

    node->right = child->left;
    if (child->left != NULL)
    {
        child->left->parent = node;
    }

    child->left = node;
    node->parent = child;
}

void rotate_right(RbTree *tree, RbTreeNode *node)
{
    RbTreeNode *parent = node->parent;
    RbTreeNode **node_ref = get_parent_child_ref(tree, node);

    RbTreeNode *child = node->left;

    *node_ref = child;
    child->parent = parent;

    node->left = child->right;
    if (child->right != NULL)
    {
        child->right->parent = node;
    }

    child->right = node;
    node->parent = child;
}

// void rotate_right_left(RbTree *tree, RbTreeNode *node)
// {
//     rotate_right(tree, node->right);
//     rotate_left(tree, node);
// }

// void rotate_left_right(RbTree *tree, RbTreeNode *node)
// {
//     rotate_left(tree, node->left);
//     rotate_right(tree, node);
// }
