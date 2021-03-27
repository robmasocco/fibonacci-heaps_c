/* Roberto Masocco
 * 16/3/2020
 * -----------------------------------------------------------------------------
 * This file contains type definitions and declarations for the Fibonacci Heap
 * library. This implementation uses unsigned 64-bit integers
 * as keys and "void *s" as elements, so anything that fits in 8 bytes will do.
 * As a priority queue, this structure offers insertions, deletions, minimum
 * key search and key modifications on a specific node.
 * Functions intended to be used are marked as such, whilst other internal
 * subroutines should not be used outside of these source files.
 * See other comments for specific descriptions of functions and data
 * structures.
 * Much of this structure uses dynamic memory allocation, trying to reduce
 * overheads by recycling existing structures.
 * WARNING: It is possible to have nodes with same keys in this structure. In
 * such case, node pointers should be preferred to operate on data to avoid
 * aliasing, which is not preventable, e.g. "fhDelete" should be used instead
 * of "fhDeleteMin", even if the target node is the minimum.
 * NOTE: A value of "0" for the key is considered the minimum possible.
 * NOTE: This structure requires Double Linked Lists to work.
 * NOTE: This structure isn't meant to be indexed, but a series of functions
 * that target specific nodes have been provided. In this implementation, no
 * aliasing problems should arise with such node's pointers during normal
 * maintenance of the structure itself.
 * NOTE: Nodes's contents could be pointers to the heap as well. A binary flag
 * is provided to free them when total heap deletion is called.
 */
/* This code is released under the MIT license.
 * See the attached LICENSE file.
 */

#ifndef _FIBONACCIHEAP_UINT64_KEYS_H
#define _FIBONACCIHEAP_UINT64_KEYS_H

#include <stdint.h>
#include <sys/types.h>

#include "DoubleLinkedList/doubleLinkedList.h"

/* These options can be OR'd in a call to the delete functions to specify
 * if also the data in the nodes must be freed in the heap.
 * If nothing is specified, only the nodes are freed.
 */
#define DELETE_FREE_DATA 0x1

/* Fibonacci Tree Node.
 * Stores a key, an element, and other metadata needed to keep track of the
 * tree structure.
 */
typedef struct __fibTreeNode {
    uint64_t key;                    // Keys in [0, UINT64_MAX].
    void *elem;                      // Element stored in the node.
    struct __fibTreeNode *_father;   // Pointer to the father node, if present.
    struct __fibTreeNode *_firstSon; // Pointer to the first son, if present.
    struct __fibTreeNode *_nextBro;  // Pointer to the next brother, if present.
    struct __fibTreeNode *_prevBro;  // Pointer to the previous brother.
    Record *_posInForest;            // For roots, position in a forest list.
    ulong _sonsCnt;                  // Counter for a node' sons.
    unsigned char _grief;            // Indicates the loss of a son.
} FibTreeNode;

/* Fibonacci Tree. Stores a pointer to its root node. */
typedef struct {
    FibTreeNode *_root;
} FibTree;

/* Fibonacci Heap. Keeps a pointer to its minimum-key node (and some
 * metadata to better track it). The "forest" is seen as an array of dynamic
 * lists, which contain pointers to trees of a specific order.
 */
typedef struct {
    DLList **_forest;         // Array of lists for different tree sizes.
    FibTreeNode *min;         // Pointer to minimum key node.
    ulong _maxTreeOrd;        // Maximum size for a tree (changes if needed).
    ulong nodesCount;         // Counter for the nodes in the structure.
} FibHeap;

/* Library functions. */
FibHeap *createFibHeap(ulong initMaxTreeOrd);
void eraseFibHeap(FibHeap *heap, int opts);
void eraseFibTreeNode(FibTreeNode *node, int opts);
int isHeapEmpty(FibHeap *heap);
FibTreeNode *fhInsert(FibHeap *heap, void *elem, uint64_t key);
void *fhFindMin(FibHeap *heap);
FibTreeNode *fhDecreaseKey(FibHeap *heap, FibTreeNode *node, uint64_t dec);
FibTreeNode *fhDeleteMin(FibHeap *heap);
FibTreeNode *fhDelete(FibHeap *heap, FibTreeNode *node);
FibTreeNode *fhIncreaseKey(FibHeap *heap, FibTreeNode *node, uint64_t inc);

#endif
