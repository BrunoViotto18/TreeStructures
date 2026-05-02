#ifndef BTREE_H
#define BTREE_H

#include <stdbool.h>
#include <stddef.h>

/// @brief Opaque binary tree type.
typedef struct BTree BTree;

/// @brief Opaque binary tree iterator type.
typedef struct BTreeIterator BTreeIterator;

/// @brief Result code returned by BTree operations.
typedef enum
{
    /// @brief Operation completed successfully.
    BTREE_OK = 0,

    /// @brief Memory allocation failed.
    BTREE_OUT_OF_MEMORY,

    /// @brief A required pointer argument was NULL.
    BTREE_NULL_POINTER_ARGUMENT,

    /// @brief The requested key was not found.
    BTREE_KEY_NOT_FOUND,

    /// @brief The provided output buffer is too small.
    BTREE_BUFFER_TOO_SMALL,

    /// @brief The tree count would overflow.
    BTREE_COUNT_OVERFLOW,

    /// @brief The element size is invalid.
    BTREE_INVALID_ELEMENT_SIZE,

    /// @brief The value key is NULL.
    BTREE_NULL_KEY,

    /// @brief The provided key already exists.
    BTREE_KEY_ALREADY_EXISTS,

    /// @brief The iterator reached the end.
    BTREE_ITERATOR_END,

    /// @brief The iterator was invalidated by a tree change.
    BTREE_ITERATOR_INVALID
} BTreeStatus;

/// @brief Gets the comparable key from a value.
/// @param value Value to get the key from.
/// @return Pointer to the value key, or NULL if the key is invalid.
typedef const void *(*BTreeGetKeyFunction)(const void *value);

/// @brief Compares two keys.
/// @param left First key to compare.
/// @param right Second key to compare.
/// @return A value less than, equal to, or greater than zero.
typedef int (*BTreeCompareKeyFunction)(const void *left, const void *right);

/// @brief Frees resources owned by a stored value.
/// @param value Stored value whose owned resources should be freed.
/// @warning Do not free value itself; free only resources owned by it.
typedef void (*BTreeFreeValueFunction)(void *value);

/// @brief Creates a new binary tree.
/// @param tree Output pointer that receives the created tree.
/// @param element_size Size in bytes of each stored value.
/// @param unique_keys Indicates whether keys should be unique.
/// @param compare_keys Function used to compare keys.
/// @param get_key Function used to get a key from a value, or NULL to use the value itself.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The tree was created successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT tree or compare_keys was NULL.
/// @retval BTREE_INVALID_ELEMENT_SIZE element_size was invalid.
/// @retval BTREE_OUT_OF_MEMORY Memory allocation failed.
BTreeStatus btree_new(BTree **tree, size_t element_size, bool unique_keys, BTreeCompareKeyFunction compare_keys, BTreeGetKeyFunction get_key);

/// @brief Frees a binary tree and all its nodes.
/// @param tree Tree to free, or NULL to do nothing.
/// @param free_function Function used to free resources owned by stored values, or NULL.
/// @warning The tree must not be used after this function returns.
void btree_free(BTree *tree, BTreeFreeValueFunction free_function);

/// @brief Removes all values from a binary tree.
/// @param tree Tree to clear.
/// @param free_function Function used to free resources owned by stored values, or NULL.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The tree was cleared successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT tree was NULL.
BTreeStatus btree_clear(BTree *tree, BTreeFreeValueFunction free_function);

/// @brief Gets the number of values stored in the tree.
/// @param tree Tree to inspect.
/// @param count Output pointer that receives the value count.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The count was written successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT tree or count was NULL.
BTreeStatus btree_getcount(BTree *tree, size_t *count);

/// @brief Copies the tree values into a buffer in sorted order.
/// @param tree Tree to copy from.
/// @param buffer Output buffer that receives the copied values.
/// @param length Number of elements the buffer can hold.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The values were copied successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT tree was NULL, or buffer was NULL when required.
/// @retval BTREE_BUFFER_TOO_SMALL length was smaller than the tree count.
BTreeStatus btree_toarray(BTree *tree, void *buffer, size_t length);

/// @brief Checks whether a key exists in the tree.
/// @param tree Tree to inspect.
/// @param key Key to search for.
/// @param exists Output pointer that receives whether the key exists.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The lookup was completed successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT tree, key, or exists was NULL.
BTreeStatus btree_exists(BTree *tree, const void *key, bool *exists);

/// @brief Copies the value matching a key into an output buffer.
/// @param tree Tree to inspect.
/// @param key Key to search for.
/// @param value Output buffer that receives the copied value.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The value was found and copied successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT tree, key, or value was NULL.
/// @retval BTREE_KEY_NOT_FOUND No value with the given key was found.
BTreeStatus btree_get(BTree *tree, const void *key, void *value);

/// @brief Adds a value to the tree.
/// @param tree Tree to modify.
/// @param value Value to copy into the tree.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The value was added successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT tree or value was NULL.
/// @retval BTREE_COUNT_OVERFLOW Adding the value would overflow the tree count.
/// @retval BTREE_NULL_KEY The value key was NULL.
/// @retval BTREE_KEY_ALREADY_EXISTS The value key already exists and unique_keys is enabled.
/// @retval BTREE_OUT_OF_MEMORY Memory allocation failed.
/// @note Duplicate keys are only allowed when unique_keys is false.
BTreeStatus btree_add(BTree *tree, const void *value);

/// @brief Removes one value matching a key.
/// @param tree Tree to modify.
/// @param key Key to search for.
/// @param value Output buffer that receives the removed value, or NULL if unused.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The value was removed successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT tree or key was NULL.
/// @retval BTREE_KEY_NOT_FOUND No value with the given key was found.
/// @warning This function does not free resources owned by the removed value.
BTreeStatus btree_remove(BTree *tree, const void *key, void *value);

/// @brief Creates a new in-order tree iterator.
/// @param tree Tree to iterate.
/// @param iterator Output pointer that receives the created iterator.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The iterator was created successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT tree or iterator was NULL.
/// @retval BTREE_OUT_OF_MEMORY Memory allocation failed.
/// @warning The tree must outlive the iterator.
/// @warning Adding, removing, or clearing a non-empty tree invalidates the iterator.
BTreeStatus btree_iterator_new(BTree *tree, BTreeIterator **iterator);

/// @brief Frees a tree iterator.
/// @param iterator Iterator to free, or NULL to do nothing.
void btree_iterator_free(BTreeIterator *iterator);

/// @brief Advances the iterator and copies the current value.
/// @param iterator Iterator to advance.
/// @param value Output buffer that receives the copied value.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The iterator advanced successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT iterator or value was NULL.
/// @retval BTREE_ITERATOR_END The iterator reached the end.
/// @retval BTREE_ITERATOR_INVALID The iterator was invalidated by a tree change.
BTreeStatus btree_iterator_next(BTreeIterator *iterator, void *value);

/// @brief Copies the current iterator value.
/// @param iterator Iterator to inspect.
/// @param value Output buffer that receives the copied value.
/// @return Result code indicating success or failure.
/// @retval BTREE_OK The current value was copied successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT iterator or value was NULL.
/// @retval BTREE_ITERATOR_END The iterator has no current value.
/// @retval BTREE_ITERATOR_INVALID The iterator was invalidated by a tree change.
BTreeStatus btree_iterator_getvalue(BTreeIterator *iterator, void *value);

#endif
