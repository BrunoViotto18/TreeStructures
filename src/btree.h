#ifndef BTREE_H
#define BTREE_H

#include <stdbool.h>
#include <stddef.h>

/// @brief Opaque binary search tree type.
///
/// The tree stores shallow byte copies of inserted values. If a value contains
/// pointers or other owned resources, use `free_function` to release those
/// resources when the stored value is removed or destroyed.
typedef struct BTree BTree;

typedef struct BTreeIterator BTreeIterator;

/// @brief Function used to extract a comparable key from a stored value.
///
/// @param value Pointer to a value.
/// @return Pointer to the key inside or derived from `value`.
/// @note The returned pointer must not be NULL.
/// @note For stored values, the returned key must remain valid for as long as
/// the value remains stored in the tree.
typedef const void *(*BTreeGetKeyFunction)(const void *value);

/// @brief Comparison function used to order keys in the tree.
///
/// @param left Pointer to the first key.
/// @param right Pointer to the second key.
///
/// @return A value less than, equal to, or greater than zero if `left` is
/// respectively less than, equal to, or greater than `right`.
typedef int (*BTreeCompareKeyFunction)(const void *left, const void *right);

/// @brief Cleanup function called before a stored value is destroyed.
///
/// The `value` pointer points to the value stored inside the tree node.
/// Do not call `free(value)` directly. The `value` pointer points inside the
/// tree node allocation.
///
/// If the stored element is itself a pointer type, cast and dereference `value`,
/// then free the pointed object.
///
/// This function should only free resources owned by the stored value, such as
/// heap pointers inside a struct.
typedef void (*BTreeFreeValueFunction)(void *value);

/// @brief Result code returned by BTree operations.
typedef enum BTreeError BTreeError;

enum BTreeError
{
    /// @brief Operation completed successfully.
    BTREE_OK = 0,

    /// @brief Memory allocation failed.
    BTREE_OUT_OF_MEMORY,

    /// @brief A required pointer argument was NULL.
    BTREE_NULL_POINTER_ARGUMENT,

    /// @brief The requested value was not found in the tree.
    BTREE_VALUE_NOT_FOUND,

    /// @brief The caller-provided output buffer is too small.
    BTREE_BUFFER_TOO_SMALL,

    /// @brief Adding another element would overflow the tree's element count.
    BTREE_COUNT_OVERFLOW,

    /// @brief The element size provided is zero or too large to allocate safely.
    BTREE_INVALID_ELEMENT_SIZE,

    /// @brief The value has a NULL key.
    BTREE_NULL_KEY,

    BTREE_ITERATOR_END,

    BTREE_ITERATOR_INVALID
};

/// @brief Creates a new empty binary tree.
///
/// If creation fails and `tree != NULL`, `*tree` is set to NULL.
///
/// @param tree Output pointer that receives the created tree. Must not be NULL.
/// @param element_size Size, in bytes, of each stored element.
/// @param compare_keys Function used to compare two keys. Must not be NULL.
/// The function must return a value less than, equal to, or greater than zero
/// if the first key is respectively less than, equal to, or greater than the second.
/// @param get_key Optional function used to extract a key from a value.
/// May be NULL. If NULL, the value itself is used as the key.
/// The returned key pointer must not be NULL.
/// @param free_function Optional cleanup function called when a stored value is removed or destroyed.
/// May be NULL. If NULL, no value-specific cleanup is performed.
///
/// @return `BTREE_OK` if the tree was created successfully.
/// @retval BTREE_OK If the tree was created successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT If `tree == NULL` or `compare_keys == NULL`.
/// @retval BTREE_INVALID_ELEMENT_SIZE If `element_size == 0`, or `element_size` is too large to safely allocate a node.
/// @retval BTREE_OUT_OF_MEMORY If memory allocation failed.
BTreeError btree_new(BTree **tree, size_t element_size, BTreeCompareKeyFunction compare_keys, BTreeGetKeyFunction get_key, BTreeFreeValueFunction free_function);

/// @brief Frees a binary tree and all values stored in it.
///
/// If `tree == NULL`, this function does nothing.
///
/// For each stored value, the tree's cleanup function is called before the node
/// is freed. If no cleanup function was provided when the tree was created,
/// no value-specific cleanup is performed.
///
/// @param tree Tree to free. May be NULL.
///
/// @warning After this function returns, `tree` must not be used again.
void btree_free(BTree *tree);

/// @brief Removes all values from a binary tree.
///
/// The tree object itself remains valid and can be reused after this function.
/// After success, the tree count is zero.
///
/// For each stored value, the tree's cleanup function is called before the node
/// is freed. If no cleanup function was provided when the tree was created,
/// no value-specific cleanup is performed.
///
/// @param tree Tree to clear. Must not be NULL.
///
/// @return `BTREE_OK` if the tree was cleared successfully.
/// @retval BTREE_OK If the tree was cleared successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT If `tree == NULL`.
BTreeError btree_clear(BTree *tree);

/// @brief Gets the number of values stored in a binary tree.
///
/// @param tree Tree to inspect. Must not be NULL.
/// @param count Output pointer that receives the number of stored values. Must not be NULL.
///
/// @return `BTREE_OK` if the count was written successfully.
/// @retval BTREE_OK If the count was written successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT If `tree == NULL` or `count == NULL`.
BTreeError btree_getcount(BTree *tree, size_t *count);

/// @brief Copies the stored values into a buffer in sorted order.
///
/// The output order follows the tree's comparison function. Values that compare
/// as equal may appear in insertion-dependent order.
///
/// @param tree Tree to copy from. Must not be NULL.
/// @param buffer Destination buffer that receives the copied values.
/// Must not be NULL if the tree is non-empty.
/// May be NULL if the tree is empty.
/// @param length Number of elements the `buffer` can hold.
///
/// @return `BTREE_OK` if the values were copied successfully.
/// @retval BTREE_OK If the values were copied successfully, or if the tree is empty.
/// @retval BTREE_NULL_POINTER_ARGUMENT If `tree == NULL`, or if `buffer == NULL` while the tree is non-empty and `length` is large enough.
/// @retval BTREE_BUFFER_TOO_SMALL If `length` is smaller than the number of values stored in the tree.
BTreeError btree_toarray(BTree *tree, void *buffer, size_t length);

/// @brief Checks whether a key exists in the binary tree.
///
/// @param tree Tree to inspect. Must not be NULL.
/// @param key Pointer to the key to search for. Must not be NULL.
/// @param exists Output pointer that receives whether the key was found. Must not be NULL.
///
/// @return `BTREE_OK` if the lookup was performed successfully.
/// @retval BTREE_OK If the lookup was performed successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT If `tree == NULL`, `key == NULL`, or `exists == NULL`.
BTreeError btree_exists(BTree *tree, const void *key, bool *exists);

/// @brief Gets a stored value by key.
///
/// The key is compared against stored values using the tree's key comparison
/// function. The key pointer is passed directly to the comparison function; it
/// is not passed through the tree's get_key function.
///
/// If a matching key is found, `*value` receives a mutable pointer to the stored
/// value inside the tree.
///
/// @param tree Tree to inspect. Must not be NULL.
/// @param key Pointer to the key to search for. Must not be NULL.
/// @param value Output pointer that receives a pointer to the stored value. Must not be NULL.
///
/// @return `BTREE_OK` if a matching value was found.
/// @retval BTREE_OK If a matching value was found.
/// @retval BTREE_NULL_POINTER_ARGUMENT If `tree == NULL`, `key == NULL`, or `value == NULL`.
/// @retval BTREE_VALUE_NOT_FOUND If no matching key exists in the tree.
///
/// @warning The returned value pointer is owned by the tree. Do not free it.
///
/// @warning The returned value may be modified, but the modification must not
/// change the value's key. Changing the key corrupts the tree ordering.
BTreeError btree_get(BTree *tree, const void *key, void **value);

/// @brief Adds a value to a binary tree.
///
/// The value is copied into the tree using the element size provided when the
/// tree was created. The caller does not need to keep the original value alive
/// after this function returns.
///
/// @param tree Tree to modify. Must not be NULL.
/// @param value Pointer to the value to add. Must not be NULL.
///
/// @return `BTREE_OK` if the value was added successfully.
/// @retval BTREE_OK If the value was added successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT If `tree == NULL` or `value == NULL`.
/// @retval BTREE_COUNT_OVERFLOW If adding another value would overflow the tree's count.
/// @retval BTREE_NULL_KEY If the value key is NULL.
/// @retval BTREE_OUT_OF_MEMORY If memory allocation failed.
///
/// @note Duplicate values are allowed. Values that compare as equal are inserted
/// into the left subtree.
BTreeError btree_add(BTree *tree, const void *value);

/// @brief Removes one matching value from a binary tree.
///
/// If multiple stored values compare as equal, only one matching value is removed.
///
/// Before the removed node is freed, the tree's cleanup function is called for
/// the stored value. If no cleanup function was provided when the tree was
/// created, no value-specific cleanup is performed.
///
/// @param tree Tree to modify. Must not be NULL.
/// @param key Pointer to the key to search for. Must not be NULL.
///
/// @return `BTREE_OK` if a matching value was removed successfully.
/// @retval BTREE_OK If a matching value was removed successfully.
/// @retval BTREE_NULL_POINTER_ARGUMENT If `tree == NULL` or `key == NULL`.
/// @retval BTREE_VALUE_NOT_FOUND If no matching key exists in the tree.
BTreeError btree_remove(BTree *tree, const void *key);

BTreeError btree_iterator_new(BTree *tree, BTreeIterator **iterator);

void btree_iterator_free(BTreeIterator *iterator);

BTreeError btree_iterator_next(BTreeIterator *iterator, void *value);

BTreeError btree_iterator_getvalue(BTreeIterator *iterator, void *value);

#endif
