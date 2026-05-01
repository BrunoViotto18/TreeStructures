#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "avltree.h"

void print_buffer(const int *buffer, int length);
int compare_ints(const void *left, const void *right);
bool filter(const void *value);

int main(void)
{
    AvlTree *tree;
    avltree_new(&tree, sizeof(int), compare_ints, NULL);

    avltree_add(tree, &(int){4});
    avltree_add(tree, &(int){2});
    avltree_add(tree, &(int){1});
    avltree_add(tree, &(int){0});
    avltree_add(tree, &(int){3});
    avltree_add(tree, &(int){6});
    avltree_add(tree, &(int){5});
    avltree_add(tree, &(int){7});
    avltree_add(tree, &(int){8});

    avltree_remove(tree, &(int){6}, NULL);
    avltree_remove(tree, &(int){5}, NULL);
    avltree_remove(tree, &(int){7}, NULL);

    avltree_add(tree, &(int){2});
    avltree_add(tree, &(int){9});
    avltree_add(tree, &(int){7});
    avltree_add(tree, &(int){6});
    avltree_add(tree, &(int){5});
    avltree_add(tree, &(int){-1});

    size_t count;
    avltree_getcount(tree, &count);
    size_t size = count * sizeof(int);
    int *buffer = malloc(size);

    avltree_toarray(tree, buffer, size);

    print_buffer(buffer, count);

    bool exists;
    avltree_exists(tree, &(int){0}, &exists);
    printf("%d\n", exists);

    avltree_exists(tree, &(int){-1}, &exists);
    printf("%d\n", exists);

    int value;
    avltree_get(tree, &(int){6}, &value);
    printf("%d\n", value);

    AvlTreeIterator *iterator;
    avltree_iterator_new(tree, &iterator, filter);

    int value2;
    while (avltree_iterator_next(iterator, &value2) == AVLTREE_OK)
    {
        printf("%d\n", value2);

        avltree_iterator_getvalue(iterator, &value2);

        printf("%d\n", value2);
    }

    avltree_iterator_free(iterator);

    free(buffer);

    avltree_free(tree, NULL);

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

bool filter(const void *value)
{
    return true;

    int v = *(int *)value;

    return v > 3 && v < 8;
}
