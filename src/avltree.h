#ifndef AVLTREE_H
#define AVLTREE_H

#include <stdbool.h>
#include <stddef.h>

/// @brief Opaque AVL tree type.
typedef struct AvlTree AvlTree;

/// @brief Opaque AVL tree iterator type.
typedef struct AvlTreeIterator AvlTreeIterator;

/// @brief Result code returned by AvlTree operations.
typedef enum
{
    /// @brief Operation completed successfully.
    AVLTREE_OK = 0,

    /// @brief Memory allocation failed.
    AVLTREE_OUT_OF_MEMORY,

    /// @brief A required pointer argument was NULL.
    AVLTREE_NULL_POINTER_ARGUMENT,

    /// @brief The requested key was not found.
    AVLTREE_KEY_NOT_FOUND,

    /// @brief The provided output buffer is too small.
    AVLTREE_BUFFER_TOO_SMALL,

    /// @brief The tree count would overflow.
    AVLTREE_COUNT_OVERFLOW,

    /// @brief The element size is invalid.
    AVLTREE_INVALID_ELEMENT_SIZE,

    /// @brief The value key is NULL.
    AVLTREE_NULL_KEY,

    /// @brief The provided key already exists.
    AVLTREE_KEY_ALREADY_EXISTS,

    /// @brief The iterator reached the end.
    AVLTREE_ITERATOR_END,

    /// @brief The iterator was invalidated by a tree change.
    AVLTREE_ITERATOR_INVALID
} AvlTreeStatus;

/// @brief Gets the comparable key from a value.
/// @param value Value to get the key from.
/// @return Pointer to the value key, or NULL if the key is invalid.
typedef const void *(*AvlTreeGetKeyFunction)(const void *value);

/// @brief Compares two keys.
/// @param left First key to compare.
/// @param right Second key to compare.
/// @return A value less than, equal to, or greater than zero.
typedef int (*AvlTreeCompareKeyFunction)(const void *left, const void *right);

/// @brief Checks whether a value should be included.
/// @param value Value to check.
/// @return true if the value should be included, false otherwise.
typedef bool (*AvlTreeFilterFunction)(const void *value);

/// @brief Frees resources owned by a stored value.
/// @param value Stored value whose owned resources should be freed.
/// @warning Do not free value itself; free only resources owned by it.
typedef void (*AvlTreeFreeValueFunction)(void *value);

/// @brief Creates a new AVL tree.
/// @param tree Output pointer that receives the created tree.
/// @param element_size Size in bytes of each stored value.
/// @param unique_keys Indicates whether keys should be unique.
/// @param compare_keys Function used to compare keys.
/// @param get_key Function used to get a key from a value, or NULL to use the value itself.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The tree was created successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT tree or compare_keys was NULL.
/// @retval AVLTREE_INVALID_ELEMENT_SIZE element_size was invalid.
/// @retval AVLTREE_OUT_OF_MEMORY Memory allocation failed.
AvlTreeStatus avltree_new(AvlTree **tree, size_t element_size, bool unique_keys, AvlTreeCompareKeyFunction compare_keys, AvlTreeGetKeyFunction get_key);

/// @brief Frees an AVL tree and all its nodes.
/// @param tree Tree to free, or NULL to do nothing.
/// @param free_function Function used to free resources owned by stored values, or NULL.
/// @warning The tree must not be used after this function returns.
void avltree_free(AvlTree *tree, AvlTreeFreeValueFunction free_function);

/// @brief Removes all values from an AVL tree.
/// @param tree Tree to clear.
/// @param free_function Function used to free resources owned by stored values, or NULL.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The tree was cleared successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT tree was NULL.
AvlTreeStatus avltree_clear(AvlTree *tree, AvlTreeFreeValueFunction free_function);

/// @brief Gets the number of values stored in the tree.
/// @param tree Tree to inspect.
/// @param count Output pointer that receives the value count.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The count was written successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT tree or count was NULL.
AvlTreeStatus avltree_getcount(AvlTree *tree, size_t *count);

/// @brief Copies the tree values into a buffer in sorted order.
/// @param tree Tree to copy from.
/// @param buffer Output buffer that receives the copied values.
/// @param length Number of elements the buffer can hold.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The values were copied successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT tree was NULL, or buffer was NULL when required.
/// @retval AVLTREE_BUFFER_TOO_SMALL length was smaller than the tree count.
AvlTreeStatus avltree_toarray(AvlTree *tree, void *buffer, size_t length);

/// @brief Checks whether a key exists in the tree.
/// @param tree Tree to inspect.
/// @param key Key to search for.
/// @param exists Output pointer that receives whether the key exists.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The lookup was completed successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT tree, key, or exists was NULL.
AvlTreeStatus avltree_exists(AvlTree *tree, const void *key, bool *exists);

/// @brief Copies the value matching a key into an output buffer.
/// @param tree Tree to inspect.
/// @param key Key to search for.
/// @param value Output buffer that receives the copied value.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The value was found and copied successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT tree, key, or value was NULL.
/// @retval AVLTREE_KEY_NOT_FOUND No value with the given key was found.
AvlTreeStatus avltree_get(AvlTree *tree, const void *key, void *value);

/// @brief Adds a value to the tree.
/// @param tree Tree to modify.
/// @param value Value to copy into the tree.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The value was added successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT tree or value was NULL.
/// @retval AVLTREE_COUNT_OVERFLOW Adding the value would overflow the tree count.
/// @retval AVLTREE_NULL_KEY The value key was NULL.
/// @retval AVLTREE_KEY_ALREADY_EXISTS The value key already exists and unique_keys is enabled.
/// @retval AVLTREE_OUT_OF_MEMORY Memory allocation failed.
/// @note Duplicate keys are only allowed when unique_keys is false.
AvlTreeStatus avltree_add(AvlTree *tree, const void *value);

/// @brief Removes one value matching a key.
/// @param tree Tree to modify.
/// @param key Key to search for.
/// @param value Output buffer that receives the removed value, or NULL if unused.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The value was removed successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT tree or key was NULL.
/// @retval AVLTREE_KEY_NOT_FOUND No value with the given key was found.
/// @warning This function does not free resources owned by the removed value.
AvlTreeStatus avltree_remove(AvlTree *tree, const void *key, void *value);

/// @brief Creates a new in-order tree iterator.
/// @param tree Tree to iterate.
/// @param iterator Output pointer that receives the created iterator.
/// @param filter Optional function used to filter values, or NULL.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The iterator was created successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT tree or iterator was NULL.
/// @retval AVLTREE_OUT_OF_MEMORY Memory allocation failed.
/// @warning The tree must outlive the iterator.
/// @warning Adding, removing, or clearing the tree invalidates the iterator.
AvlTreeStatus avltree_iterator_new(AvlTree *tree, AvlTreeIterator **iterator, AvlTreeFilterFunction filter);

/// @brief Frees a tree iterator.
/// @param iterator Iterator to free, or NULL to do nothing.
void avltree_iterator_free(AvlTreeIterator *iterator);

/// @brief Advances the iterator and copies the current value.
/// @param iterator Iterator to advance.
/// @param value Output buffer that receives the copied value.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The iterator advanced successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT iterator or value was NULL.
/// @retval AVLTREE_ITERATOR_END The iterator reached the end.
/// @retval AVLTREE_ITERATOR_INVALID The iterator was invalidated by a tree change.
AvlTreeStatus avltree_iterator_next(AvlTreeIterator *iterator, void *value);

/// @brief Copies the current iterator value.
/// @param iterator Iterator to inspect.
/// @param value Output buffer that receives the copied value.
/// @return Result code indicating success or failure.
/// @retval AVLTREE_OK The current value was copied successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT iterator or value was NULL.
/// @retval AVLTREE_ITERATOR_END The iterator has no current value.
/// @retval AVLTREE_ITERATOR_INVALID The iterator was invalidated by a tree change.
AvlTreeStatus avltree_iterator_getvalue(AvlTreeIterator *iterator, void *value);

#endif
