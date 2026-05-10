#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "rbtree.h"

void print_buffer(const int *buffer, int length);
int compare_ints(const void *left, const void *right);

int main(void)
{
    RbTree *tree;
    rbtree_new(&tree, sizeof(int), true, compare_ints, NULL);

    rbtree_add(tree, &(int){4});
    rbtree_add(tree, &(int){2});
    rbtree_add(tree, &(int){1});
    rbtree_add(tree, &(int){0});
    rbtree_add(tree, &(int){3});
    rbtree_add(tree, &(int){6});
    rbtree_add(tree, &(int){5});
    rbtree_add(tree, &(int){7});
    rbtree_add(tree, &(int){8});

    rbtree_remove(tree, &(int){6}, NULL);
    rbtree_remove(tree, &(int){5}, NULL);
    rbtree_remove(tree, &(int){7}, NULL);

    rbtree_add(tree, &(int){2});
    rbtree_add(tree, &(int){9});
    rbtree_add(tree, &(int){7});
    rbtree_add(tree, &(int){6});
    rbtree_add(tree, &(int){5});

    size_t count;
    rbtree_getcount(tree, &count);
    size_t size = count * sizeof(int);
    int *buffer = malloc(size);

    rbtree_toarray(tree, buffer, size);

    print_buffer(buffer, count);

    bool exists;
    rbtree_exists(tree, &(int){0}, &exists);
    printf("%d\n", exists);

    rbtree_exists(tree, &(int){-1}, &exists);
    printf("%d\n", exists);

    int value;
    rbtree_get(tree, &(int){6}, &value);
    printf("%d\n", value);

    RbTreeIterator *iterator;
    rbtree_iterator_new(tree, &iterator);

    int value2;
    while (rbtree_iterator_next(iterator, &value2) == RBTREE_OK)
    {
        // if (value2 <= 3 || value2 >= 8)
        // {
        //     continue;
        // }

        printf("%d\n", value2);

        rbtree_iterator_getvalue(iterator, &value2);

        printf("%d\n", value2);
    }

    rbtree_iterator_free(iterator);

    free(buffer);

    rbtree_free(tree, NULL);

    return 0;
}

void print_buffer(const int *buffer, int length)
{
    printf("[");
    if (length > 0)
    {
        printf(" %d", buffer[0]);
    }
    for (int i = 1; i < length; i++)
    {
        printf(", %d", buffer[i]);
    }
    printf(" ]\n");
}

int compare_ints(const void *left, const void *right)
{
    int l = *(int *)left;
    int r = *(int *)right;
    return (l > r) - (l < r);
}
