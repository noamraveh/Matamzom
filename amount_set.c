//
// Created by Noam Raveh & Carmel David on 18/11/2019.
//

#include "amount_set.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#define IS_NULL(ptr1, ptr2)    ((ptr1 == NULL || ptr2 == NULL) ? (true) : (false))

typedef struct ElementNode_t *ElementNode;
struct ElementNode_t {
    ASElement element;
    double amount;
    ElementNode next_node;
};

struct AmountSet_t {
    ElementNode first_node;
    ElementNode iterator;
    int size;
    CopyASElement copyElement;
    FreeASElement freeElement;
    CompareASElements compareElements;
};

static ElementNode createElementNode(AmountSet amount_set_ptr, ASElement element);

static ElementNode findElement(AmountSet set, ASElement element);

AmountSet asCreate(CopyASElement copyElement,
                   FreeASElement freeElement,
                   CompareASElements compareElements) {
    if (copyElement == NULL || freeElement == NULL || compareElements == NULL) {
        return NULL;
    }
    AmountSet as_ptr = malloc(sizeof(*as_ptr));
    if (as_ptr == NULL) {
        return NULL;
    }
    assert(as_ptr != NULL);
    as_ptr->size = 0;
    as_ptr->iterator = NULL;
    as_ptr->copyElement = copyElement;
    as_ptr->freeElement = freeElement;
    as_ptr->compareElements = compareElements;
    as_ptr->first_node = NULL;
    return as_ptr;
}

AmountSetResult asDelete(AmountSet set, ASElement element) {
    if (IS_NULL(set, element)) {
        return AS_NULL_ARGUMENT;
    }
    if (!asContains(set, element)) {
        return AS_ITEM_DOES_NOT_EXIST;
    }
    ElementNode ptr = set->first_node;
    assert(ptr != NULL);
    if (set->compareElements(element, ptr->element) == 0) { //deleting the first element
        set->first_node = set->first_node->next_node;
        set->freeElement(ptr->element);
        free(ptr);
        set->size--;
        set->iterator = NULL;
        return AS_SUCCESS;
    }
    while (ptr->next_node != NULL) {
        if (ptr->next_node->next_node == NULL) { //deleting the last element
            assert(set->compareElements(element, ptr->next_node->element) == 0);
            set->freeElement(ptr->next_node->element);
            free(ptr->next_node);
            ptr->next_node = NULL;
            set->size--;
            set->iterator = NULL;
            return AS_SUCCESS;
        }
        if (set->compareElements(ptr->next_node->element, element) == 0) { //deleting an element in the middle
            ElementNode to_delete = ptr->next_node;
            ptr->next_node = ptr->next_node->next_node;
            set->freeElement(to_delete->element);
            free(to_delete);
            set->size--;
            set->iterator = NULL;
            return AS_SUCCESS;
        }
        ptr = ptr->next_node; //moving to the next node
    }
    return AS_SUCCESS;
}

AmountSetResult asClear(AmountSet set) {
    if (set == NULL) {
        return AS_NULL_ARGUMENT;
    }
    if (set->first_node == NULL) {
        return AS_SUCCESS;
    }
    ElementNode ptr = set->first_node;
    while (ptr != NULL) {
        ElementNode to_delete = ptr;
        ptr = ptr->next_node;
        set->freeElement(to_delete->element);
        free(to_delete);
        set->size--;
    }
    set->first_node = NULL;
    return AS_SUCCESS;
}

void asDestroy(AmountSet set) {
    if (set == NULL) {
        return;
    }
    asClear(set); //clear all elements from the set
    free(set);
}

AmountSet asCopy(AmountSet set) {
    if (set == NULL) {
        return NULL;
    }
    AmountSet new_set = asCreate(set->copyElement, set->freeElement, set->compareElements);
    if (new_set == NULL) {
        return NULL;
    }
    ElementNode ptr = set->first_node;
    while (ptr != NULL) {
        if (asRegister(new_set, ptr->element) != AS_SUCCESS) {
            asDestroy(new_set);
            return NULL;
        }
        ElementNode ptr_to_last = new_set->first_node;
        while (ptr_to_last->next_node != NULL) {
            ptr_to_last = ptr_to_last->next_node;
        }
        asChangeAmount(new_set, ptr_to_last->element, ptr->amount);
        ptr = ptr->next_node;
    }
    set->iterator = NULL;
    new_set->iterator = NULL;
    return new_set;
}

int asGetSize(AmountSet set) {
    if (set == NULL) {
        return -1;
    }
    return set->size;
}

bool asContains(AmountSet set, ASElement element) {
    if (IS_NULL(set, element)) {
        return false;
    }
    if (findElement(set, element) == NULL) {
        return false;
    }
    return true;
}

AmountSetResult asGetAmount(AmountSet set, ASElement element, double *outAmount) {
    if (IS_NULL(set, element)) {
        return AS_NULL_ARGUMENT;
    }
    if (outAmount == NULL) {
        return AS_NULL_ARGUMENT;
    }
    ElementNode ptr = findElement(set, element);
    if (ptr == NULL) {
        return AS_ITEM_DOES_NOT_EXIST;
    }
    *outAmount = ptr->amount;
    return AS_SUCCESS;
}

AmountSetResult asRegister(AmountSet set, ASElement element) { // new version
    if (IS_NULL(set, element)) {
        return AS_NULL_ARGUMENT;
    }
    if (asContains(set, element)) {
        return AS_ITEM_ALREADY_EXISTS;
    }
    ElementNode new_node = createElementNode(set, element);
    if (new_node == NULL) {
        return AS_OUT_OF_MEMORY;
    }
    ElementNode ptr = set->first_node;
    ElementNode previous = NULL;
    if (ptr == NULL) { // adding element to an empty set
        set->first_node = new_node;
        new_node->next_node = NULL;
        (set->size)++;
        set->iterator = NULL;
        return AS_SUCCESS;
    }
    while ((ptr != NULL) && (set->compareElements(element, ptr->element) > 0)) { // adding element to an existing set
        previous = ptr;
        ptr = ptr->next_node;
    }
    if (ptr == NULL) { //adding element to the end of the list
        previous->next_node = new_node;
        new_node->next_node = NULL;
        set->size++;
        set->iterator = NULL;
        return AS_SUCCESS;
    }
    if (previous != NULL) { // adding element in the middle of the list
        previous->next_node = new_node;
        new_node->next_node = ptr;
    }
    else { // adding element to the list's head
        new_node->next_node = set->first_node;
        set->first_node = new_node;
    }
    set->size++;
    set->iterator = NULL;
    return AS_SUCCESS;
}

AmountSetResult asChangeAmount(AmountSet set, ASElement element, const double amount) {
    if (IS_NULL(set, element)) {
        return AS_NULL_ARGUMENT;
    }
    ElementNode ptr = findElement(set, element);
    if (ptr == NULL) {
        return AS_ITEM_DOES_NOT_EXIST;
    }
    if (ptr->amount + amount < 0) {
        return AS_INSUFFICIENT_AMOUNT;
    }
    ptr->amount += amount;
    return AS_SUCCESS;
}

ASElement asGetFirst(AmountSet set) {
    if (IS_NULL(set, set->first_node)) {
        return NULL;
    }
    set->iterator = set->first_node;
    return set->first_node->element;
}

ASElement asGetNext(AmountSet set) {
    if (IS_NULL(set, set->iterator)) {
        return NULL;
    }
    if (set->first_node == NULL) {
        return NULL;
    }
    if (set->iterator->next_node == NULL) {// iterator points on the last element
        return NULL;
    }
    set->iterator = set->iterator->next_node;
    return set->iterator->element;
}


static ElementNode findElement(AmountSet set, ASElement element) {
    assert(set != NULL && element != NULL);
    ElementNode ptr = set->first_node;
    while (ptr != NULL) {
        if (ptr->element == NULL) {
            break;
        }
        if (set->compareElements(ptr->element, element) == 0) {
            return ptr;
        }
        ptr = ptr->next_node;
    }
    return NULL;
}

static ElementNode createElementNode(AmountSet amount_set_ptr, ASElement element) {
    ElementNode ptr = malloc(sizeof(*ptr));
    if (ptr == NULL) {
        return NULL;
    }
    ptr->amount = 0;
    ptr->next_node = NULL;
    ptr->element = amount_set_ptr->copyElement(element);
    return ptr;
}



