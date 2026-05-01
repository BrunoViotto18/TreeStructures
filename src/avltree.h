#ifndef AVLTREE_H
#define AVLTREE_H

#include <stdbool.h>
#include <stddef.h>

/// @brief Opaque AVL tree type.
///
/// The tree stores shallow byte copies of inserted values. If a value contains
/// pointers or other owned resources, use `free_function` to release those
/// resources when the stored value is removed or destroyed.
typedef struct AvlTree AvlTree;

typedef struct AvlTreeIterator AvlTreeIterator;

/// @brief Function used to extract a comparable key from a stored value.
///
/// @param value Pointer to a value.
/// @return Pointer to the key inside or derived from `value`.
/// @note The returned pointer must not be NULL.
/// @note For stored values, the returned key must remain valid for as long as
/// the value remains stored in the tree.
typedef const void *(*AvlTreeGetKeyFunction)(const void *value);

/// @brief Comparison function used to order keys in the tree.
///
/// @param left Pointer to the first key.
/// @param right Pointer to the second key.
///
/// @return A value less than, equal to, or greater than zero if `left` is
/// respectively less than, equal to, or greater than `right`.
typedef int (*AvlTreeCompareKeyFunction)(const void *left, const void *right);

typedef bool (*AvlTreeFilterFunction)(const void *value);

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
typedef void (*AvlTreeFreeValueFunction)(void *value);

/// @brief Result code returned by AvlTree operations.
typedef enum AvlTreeError AvlTreeError;

enum AvlTreeError
{
    /// @brief Operation completed successfully.
    AVLTREE_OK = 0,

    /// @brief Memory allocation failed.
    AVLTREE_OUT_OF_MEMORY,

    /// @brief A required pointer argument was NULL.
    AVLTREE_NULL_POINTER_ARGUMENT,

    /// @brief The requested value was not found in the tree.
    AVLTREE_KEY_NOT_FOUND,

    /// @brief The caller-provided output buffer is too small.
    AVLTREE_BUFFER_TOO_SMALL,

    /// @brief Adding another element would overflow the tree's element count.
    AVLTREE_COUNT_OVERFLOW,

    /// @brief The element size provided is zero or too large to allocate safely.
    AVLTREE_INVALID_ELEMENT_SIZE,

    /// @brief The value has a NULL key.
    AVLTREE_NULL_KEY,

    AVLTREE_ITERATOR_END,

    AVLTREE_ITERATOR_INVALID
};

/// @brief Creates a new empty AVL tree.
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
/// @return `AVLTREE_OK` if the tree was created successfully.
/// @retval AVLTREE_OK If the tree was created successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT If `tree == NULL` or `compare_keys == NULL`.
/// @retval AVLTREE_INVALID_ELEMENT_SIZE If `element_size == 0`, or `element_size` is too large to safely allocate a node.
/// @retval AVLTREE_OUT_OF_MEMORY If memory allocation failed.
AvlTreeError avltree_new(AvlTree **tree, size_t element_size, AvlTreeCompareKeyFunction compare_keys, AvlTreeGetKeyFunction get_key);

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
void avltree_free(AvlTree *tree, AvlTreeFreeValueFunction free_function);

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
/// @return `AVLTREE_OK` if the tree was cleared successfully.
/// @retval AVLTREE_OK If the tree was cleared successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT If `tree == NULL`.
AvlTreeError avltree_clear(AvlTree *tree, AvlTreeFreeValueFunction free_function);

/// @brief Gets the number of values stored in a binary tree.
///
/// @param tree Tree to inspect. Must not be NULL.
/// @param count Output pointer that receives the number of stored values. Must not be NULL.
///
/// @return `AVLTREE_OK` if the count was written successfully.
/// @retval AVLTREE_OK If the count was written successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT If `tree == NULL` or `count == NULL`.
AvlTreeError avltree_getcount(AvlTree *tree, size_t *count);

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
/// @return `AVLTREE_OK` if the values were copied successfully.
/// @retval AVLTREE_OK If the values were copied successfully, or if the tree is empty.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT If `tree == NULL`, or if `buffer == NULL` while the tree is non-empty and `length` is large enough.
/// @retval AVLTREE_BUFFER_TOO_SMALL If `length` is smaller than the number of values stored in the tree.
AvlTreeError avltree_toarray(AvlTree *tree, void *buffer, size_t length);

/// @brief Checks whether a key exists in the binary tree.
///
/// @param tree Tree to inspect. Must not be NULL.
/// @param key Pointer to the key to search for. Must not be NULL.
/// @param exists Output pointer that receives whether the key was found. Must not be NULL.
///
/// @return `AVLTREE_OK` if the lookup was performed successfully.
/// @retval AVLTREE_OK If the lookup was performed successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT If `tree == NULL`, `key == NULL`, or `exists == NULL`.
AvlTreeError avltree_exists(AvlTree *tree, const void *key, bool *exists);

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
/// @return `AVLTREE_OK` if a matching value was found.
/// @retval AVLTREE_OK If a matching value was found.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT If `tree == NULL`, `key == NULL`, or `value == NULL`.
/// @retval AVLTREE_KEY_NOT_FOUND If no matching key exists in the tree.
///
/// @warning The returned value pointer is owned by the tree. Do not free it.
///
/// @warning The returned value may be modified, but the modification must not
/// change the value's key. Changing the key corrupts the tree ordering.
AvlTreeError avltree_get(AvlTree *tree, const void *key, void *value);

AvlTreeError avltree_set(AvlTree *tree, const void *key, const void *value);

/// @brief Adds a value to a binary tree.
///
/// The value is copied into the tree using the element size provided when the
/// tree was created. The caller does not need to keep the original value alive
/// after this function returns.
///
/// @param tree Tree to modify. Must not be NULL.
/// @param value Pointer to the value to add. Must not be NULL.
///
/// @return `AVLTREE_OK` if the value was added successfully.
/// @retval AVLTREE_OK If the value was added successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT If `tree == NULL` or `value == NULL`.
/// @retval AVLTREE_COUNT_OVERFLOW If adding another value would overflow the tree's count.
/// @retval AVLTREE_NULL_KEY If the value key is NULL.
/// @retval AVLTREE_OUT_OF_MEMORY If memory allocation failed.
///
/// @note Duplicate values are allowed. Values that compare as equal are inserted
/// into the left subtree.
AvlTreeError avltree_add(AvlTree *tree, const void *value);

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
/// @return `AVLTREE_OK` if a matching value was removed successfully.
/// @retval AVLTREE_OK If a matching value was removed successfully.
/// @retval AVLTREE_NULL_POINTER_ARGUMENT If `tree == NULL` or `key == NULL`.
/// @retval AVLTREE_KEY_NOT_FOUND If no matching key exists in the tree.
AvlTreeError avltree_remove(AvlTree *tree, const void *key, void *value);

AvlTreeError avltree_iterator_new(AvlTree *tree, AvlTreeIterator **iterator, AvlTreeFilterFunction filter_function);

void avltree_iterator_free(AvlTreeIterator *iterator);

AvlTreeError avltree_iterator_next(AvlTreeIterator *iterator, void *value);

AvlTreeError avltree_iterator_getvalue(AvlTreeIterator *iterator, void *value);

#endif
