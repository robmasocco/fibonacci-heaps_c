/* Roberto Masocco
 * 16/3/2020
 * -----------------------------------------------------------------------------
 * Main source file for the Fibonacci Heap library.
 * See the header file for a description of the library.
 * See comments below for a brief description of what each function does.
 */
/* This code is released under the MIT license.
 * See the attached LICENSE file.
 */

#include <stdlib.h>
#include <limits.h>

#include "FibonacciHeap_uint64-keys.h"

/* Declarations of internal library subroutines. */
Record *_mergeRecordedTrees(FibTree *tree, FibTree *otherTree,
                            Record *firstTreeRecord, Record *otherTreeRecord);
void _cutSubtrees(FibTree *tree);
void _updateMin(FibHeap *heap, FibTreeNode *newNode);
void _rebuild(FibHeap *heap);
FibTreeNode *_insertNode(FibHeap *heap, FibTreeNode *node);
void _eraseTree(FibTree *tree, int opts);
void _eraseSubtree(FibTreeNode *root, int opts);
void _cascadedDetach(FibHeap *heap, FibTreeNode *decNode);

// LIBRARY FUNCTIONS //
/* Creates and initializes a new Fibonacci Heap.
 * An initial maximum tree order is required (an integer n such that 2^n should
 * be the maximum amount of data, i.e. nodes, in the heap), but such amount
 * can be automatically increased during normal usage.
 */
FibHeap *createFibHeap(ulong initMaxTreeOrd) {
    if (initMaxTreeOrd == 0) return NULL;
    FibHeap *newHeap = calloc(1, sizeof(FibHeap));
    DLList **treeList = calloc(initMaxTreeOrd, sizeof(DLList *));
    if (newHeap == NULL) return NULL;  // calloc failed.
    if (treeList == NULL) {
        free(newHeap);
        return NULL;
    }
    for (ulong i = 0; i < initMaxTreeOrd; i++) {
        treeList[i] = createDLList();
        if (treeList[i] == NULL) {
            // calloc failed.
            for (ulong j = 0; j < i; j++) {
                // Free all previous entries.
                free(treeList[j]);
            }
            free(treeList);
            free(newHeap);
            return NULL;
        }
    }
    newHeap->_forest = treeList;
    newHeap->min = NULL;
    newHeap->_maxTreeOrd = initMaxTreeOrd;
    newHeap->nodesCount = 0;
    return newHeap;
}

/* Destroys a Fibonacci Heap, freeing memory. */
void eraseFibHeap(FibHeap *heap, int opts) {
    if (heap == NULL) return;
    if (!isHeapEmpty(heap)) {
        for (ulong i = 0; i < heap->_maxTreeOrd; i++) {
            while (!isListEmpty((heap->_forest)[i])) {
                FibTree *currTree = popFirst((heap->_forest)[i]);
                _eraseTree(currTree, opts);
            }
            eraseList((heap->_forest)[i]);
        }
    } else {
        for (ulong i = 0; i < heap->_maxTreeOrd; i++)
            eraseList((heap->_forest)[i]);
    }
    free(heap->_forest);
    free(heap);
}

/* Deletes a given node, freeing memory. */
void eraseFibTreeNode(FibTreeNode *node, int opts) {
    if (node == NULL) return;
    if (opts & DELETE_FREE_DATA) free(node->elem);
    free(node);
}

/* Tells whether a given heap is empty or not. */
int isHeapEmpty(FibHeap *heap) {
    if (heap == NULL) return -1;
    for (ulong i = 0; i < heap->_maxTreeOrd; i++)
        if (!isListEmpty((heap->_forest)[i])) return 0;
    return 1;
}

/* Returns the element corresponding to the minimum key. */
void *fhFindMin(FibHeap *heap) {
    if (heap == NULL) return 0;
    if (heap->min == NULL) return 0;
    return heap->min->elem;
}

/* Creates a new node, as a B0 tree, and adds it to the heap. */
FibTreeNode *fhInsert(FibHeap *heap, void *elem, uint64_t key) {
    if (heap == NULL) return NULL;
    if (heap->nodesCount == ULONG_MAX) return NULL;  // The heap is full.
    // Create a new node.
    FibTreeNode *newNode = calloc(1, sizeof(FibTreeNode));
    if (newNode == NULL) return NULL;
    newNode->key = key;
    newNode->elem = elem;
    newNode->_father = NULL;
    newNode->_firstSon = NULL;
    newNode->_nextBro = NULL;
    newNode->_prevBro = NULL;
    newNode->_posInForest = NULL;
    newNode->_sonsCnt = 0;
    newNode->_grief = 0;
    return _insertNode(heap, newNode);
}

/* Decreases node's key of dec (key -= dec), updating the heap structure.
 * Returns a pointer to the node.
 */
FibTreeNode *fhDecreaseKey(FibHeap *heap, FibTreeNode *node, uint64_t dec) {
    if ((heap == NULL) || (node == NULL)) return NULL;

    // Decrement the key and eventually start detaching nodes to restore and
    // preserve the Fibonacci Tree structure.
    node->key -= dec;
    if ((node->_father != NULL) && (node->key < node->_father->key))
        _cascadedDetach(heap, node);

    // Check if the node is now a root.
    if (node->_father == NULL)
        // Update min node (this node could be the new min node).
        _updateMin(heap, node);
    return node;
}

/* Deletes the node with min key value from the heap and returns it.
 * "Rebuilds" the heap afterwards.
 */
FibTreeNode *fhDeleteMin(FibHeap *heap) {
    if (heap == NULL) return NULL;

    // Check if there is at least a node in the heap.
    if (isHeapEmpty(heap)) return  NULL;

    // Cut the tree with minimum root from the heap.
    FibTree *minTree = (FibTree *)(heap->min->_posInForest->recData);
    FibTreeNode *minNode = heap->min;
    Record *treeRecord = popRecord(heap->min->_posInForest,
                                   (heap->_forest)[heap->min->_sonsCnt]);
    eraseRecord(treeRecord);

    // Cut the subtrees from the root (i.e.: all sons have a NULL father now).
    _cutSubtrees(minTree);

    // Delete the minTree.
    free(minTree);

    // Create new subtrees and insert them in the correct lists of the heap.
    // Their order can be determined by looking at how many sons they have.
    FibTreeNode *newRoot = minNode->_firstSon;
    while (newRoot != NULL) {
        FibTreeNode *nextOne = newRoot->_nextBro;
        newRoot->_nextBro = NULL;
        newRoot->_prevBro = NULL;
        FibTree *newTree = calloc(1, sizeof(FibTree));
        if (newTree == NULL) return NULL;  // Shit incoming...
        newTree->_root = newRoot;
        Record *newTreeRec = addAsLast(newTree,
                                       (heap->_forest)[newRoot->_sonsCnt]);
        if (newTreeRec == NULL) {
            // Even worse shit incoming...
            free(newTree);
            return NULL;
        }
        newRoot->_posInForest = newTreeRec;
        newRoot = nextOne;
    }

    _rebuild(heap);
    heap->nodesCount--;

    minNode->_father = NULL;
    minNode->_firstSon = NULL;
    minNode->_nextBro = NULL;
    minNode->_prevBro = NULL;
    minNode->_posInForest = NULL;
    minNode->_grief = 0;
    minNode->_sonsCnt = 0;
    return minNode;
}

/* Deletes a node from the tree and returns it. */
FibTreeNode *fhDelete(FibHeap *heap, FibTreeNode *node) {
    if ((heap == NULL) || (node == NULL)) return NULL;

    // Save key value.
    uint64_t key = node->key;

    // Decrease key value to min.
    fhDecreaseKey(heap, node, node->key);

    // Delete the node with min key in heap; it will be the node to be deleted.
    FibTreeNode *deleted = fhDeleteMin(heap);

    // Restore node key.
    deleted->key = key;

    return deleted;
}

/* Increases node key of inc (key += inc), updating the heap structure.
 * Returns a pointer to the node.
 */
FibTreeNode *fhIncreaseKey(FibHeap *heap, FibTreeNode *node, uint64_t inc) {
    if ((heap == NULL) || (node == NULL)) return NULL;

    // Delete the node from the heap and re-insert it with the new key.
    FibTreeNode *deletedNode = fhDelete(heap, node);
    deletedNode->key += inc;
    _insertNode(heap, deletedNode);

    return deletedNode;
}

// INTERNAL LIBRARY SUBROUTINES //
/* Updates the minimum node pointer. */
void _updateMin(FibHeap *heap, FibTreeNode *newNode) {
    if (isHeapEmpty(heap)) {
        heap->min = NULL;
    } else {
        if (newNode == NULL) {
            // Slow mode: we don't have any clues apart from the fact that
            // the min must be a root.
            FibTreeNode *newMin = NULL;
            uint64_t newMinKey = UINT64_MAX;
            for (ulong i = 0; i < heap->_maxTreeOrd; i++) {
                Record *curr = (heap->_forest)[i]->first;
                while (curr != NULL) {
                    if (((FibTree *)(curr->recData))->_root->key < newMinKey) {
                        newMin = ((FibTree *)(curr->recData))->_root;
                        newMinKey = newMin->key;
                    }
                    curr = curr->next;
                }
            }
            heap->min = newMin;
        } else
            // Fast mode: we already know the node that has been modified.
            if ((heap->min == NULL) || (newNode->key < heap->min->key))
                heap->min = newNode;
    }
}

/* Merges identical trees and restores uniqueness property. */
void _rebuild(FibHeap *heap) {
    for (ulong i = 0; i < heap->_maxTreeOrd; i++) {
        while ((heap->_forest)[i]->recsCount > 1) {
            Record *aRecordedTree = popFirstRecord((heap->_forest)[i]);
            Record *bRecordedTree = popLastRecord((heap->_forest)[i]);
            FibTree *aTree = aRecordedTree->recData;
            FibTree *bTree = bRecordedTree->recData;
            Record *newRecordedTree = _mergeRecordedTrees(aTree, bTree,
                    aRecordedTree, bRecordedTree);
            if ((i + 1) >= heap->_maxTreeOrd) {
                // Extend the trees list.
                heap->_forest = reallocarray(heap->_forest,
                        heap->_maxTreeOrd + 1, sizeof(DLList *));
                if (heap->_forest == NULL)
                    // Happens only at the end, so exits the for too.
                    break;
                (heap->_forest)[i + 1] = createDLList();
                if ((heap->_forest)[i + 1] == NULL) break;  // Unlikely.
                heap->_maxTreeOrd++;
            }
            addAsLastRecord(newRecordedTree, (heap->_forest)[i + 1]);
        }
    }
    // Scan all roots (now one for each tree type) to find the new min.
    _updateMin(heap, NULL);
}

/* Merges two Fibonacci Trees. */
Record *_mergeRecordedTrees(FibTree *tree, FibTree *otherTree,
                            Record *firstTreeRecord, Record *otherTreeRecord) {
    FibTreeNode *thisRoot = tree->_root;
    FibTreeNode *otherRoot = otherTree->_root;
    // Check roots's keys and decide who becomes whose son.
    // Update node metadata accordingly.
    if (thisRoot->key <= otherRoot->key) {
        otherRoot->_father = thisRoot;
        otherRoot->_nextBro = NULL;
        otherRoot->_prevBro = NULL;
        otherRoot->_posInForest = NULL;
        thisRoot->_sonsCnt++;
        if (thisRoot->_firstSon != NULL) {
            otherRoot->_nextBro = thisRoot->_firstSon;
            thisRoot->_firstSon->_prevBro = otherRoot;
            thisRoot->_firstSon = otherRoot;
        } else thisRoot->_firstSon = otherRoot;
        free(otherTree);
        eraseRecord(otherTreeRecord);
        return firstTreeRecord;
    } else {
        thisRoot->_father = otherRoot;
        thisRoot->_nextBro = NULL;
        thisRoot->_prevBro = NULL;
        thisRoot->_posInForest = NULL;
        otherRoot->_sonsCnt++;
        if (otherRoot->_firstSon != NULL) {
            thisRoot->_nextBro = otherRoot->_firstSon;
            otherRoot->_firstSon->_prevBro = thisRoot;
            otherRoot->_firstSon = thisRoot;
        } else otherRoot->_firstSon = thisRoot;
        free(tree);
        eraseRecord(firstTreeRecord);
        return otherTreeRecord;
    }
}

/* Deletes a given Fibonacci Tree, freeing memory. */
void _eraseTree(FibTree *tree, int opts) {
    // Start recursive cancellation of the tree.
    _eraseSubtree(tree->_root, opts);
    free(tree);
}

/* Recursively deletes a subtree rooted in a given node. Works as a DFS. */
void _eraseSubtree(FibTreeNode *root, int opts) {
    FibTreeNode *currSon = root->_firstSon;
    while (currSon != NULL) {
        // Recursive step: visit all sons and delete them.
        FibTreeNode *nextOne = currSon->_nextBro;
        _eraseSubtree(currSon, opts);
        currSon = nextOne;
    }
    // Also base step: node has no sons, so delete it.
    if (opts & DELETE_FREE_DATA) free(root->elem);
    free(root);
}

/* Sets the father of all the first-level sons of a root to NULL. */
void _cutSubtrees(FibTree *tree) {
    FibTreeNode *currSon = tree->_root->_firstSon;
    while (currSon != NULL) {
        currSon->_father = NULL;
        currSon = currSon->_nextBro;
    }
}

/* Inserts an existing node as a new B0 in the heap. */
FibTreeNode *_insertNode(FibHeap *heap, FibTreeNode *node) {
    // Create new B0 tree.
    FibTree *newTree = calloc(1, sizeof(FibTree));
    if (newTree == NULL) {
        free(node);
        return NULL;
    }
    newTree->_root = node;
    // Add the new tree to the B0s list and update the min pointer.
    Record *newTreeRec = addAsLast(newTree, (heap->_forest)[0]);
    if (newTreeRec == NULL) {
        free(node);
        free(newTree);
        return NULL;
    }
    node->_posInForest = newTreeRec;
    _updateMin(heap, node);
    heap->nodesCount++;
    return newTree->_root;
}

/* Restores the structure of a Fibonacci Tree, detaching subtrees. */
void _cascadedDetach(FibHeap *heap, FibTreeNode *decNode) {
    FibTreeNode *father = decNode->_father;  // This always exists.
    // Detach this node from its brothers and father.
    if (father->_firstSon == decNode) father->_firstSon = decNode->_nextBro;
    if (decNode->_prevBro != NULL)
        decNode->_prevBro->_nextBro = decNode->_nextBro;
    if (decNode->_nextBro != NULL)
        decNode->_nextBro->_prevBro = decNode->_prevBro;
    decNode->_father = NULL;
    decNode->_nextBro = NULL;
    decNode->_prevBro = NULL;
    father->_sonsCnt--;
    // Create a new tree with this node as root.
    FibTree *newTree = calloc(1, sizeof(FibTree));
    if (newTree == NULL) return;  // Shit incoming...
    newTree->_root = decNode;
    // Add the new tree to the correct order list.
    // This can be determined by looking at how many sons the node has.
    Record *newTreeRec = addAsLast(newTree, (heap->_forest)[decNode->_sonsCnt]);
    if (newTreeRec == NULL) {
        // Even worse shit incoming...
        free(newTree);
        return;
    }
    decNode->_posInForest = newTreeRec;
    // Reset this node's grief.
    decNode->_grief = 0;
    // Now, you may have to do this again. Go up and check out!
    // Note that, in Fibonacci Trees, each node is allowed to lose one son only.
    if (father->_father != NULL) {
        if (father->_grief == 1) _cascadedDetach(heap, father);
        else father->_grief = 1;  // Mark the loss of the node above.
    } else
        // The father is a root. Since it lost a son, it must be moved to the
        // previous trees list.
        addAsLastRecord(popRecord(father->_posInForest,
                (heap->_forest)[father->_sonsCnt + 1]),
                (heap->_forest)[father->_sonsCnt]);
}
