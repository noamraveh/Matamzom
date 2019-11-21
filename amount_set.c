//
// Created by Noam Raveh & Carmel David on 18/11/2019.
//

#include "amount_set.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#define IsNULL(ptr1,ptr2)    ((ptr1 == NULL || ptr2 == NULL) ? (true) : (false))

typdef struct ElementNode_t* ElementNode;
struct ElementNode_t{
    ASElement element;
    double amount;
    ElementNode* next_node;
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
static ElementNode FindElement(AmountSet set, ElementNode element);

AmountSet asCreate(CopyASElement copyElement,
                   FreeASElement freeElement,
                   CompareASElements compareElements){
    if (copyElement != NULL || freeElement != NULL || compareElement != NULL){
        return NULL;
    }

    AmountSet amount_set_ptr = malloc(sizeof(*amount_set_ptr));
    if (amount_set_ptr == NULL){
        return NULL;
    }

    amount_set_ptr->size = 0;
    amount_set_ptr->iterator = NULL;
    amount_set_ptr->copyElement = copyElement;
    amount_set_ptr->freeElement = freeElement;
    amount_set_ptr->compareElements = compareElements;
    amount_set_ptr->first_node = createElementNode(amount_set_ptr, NULL)
    return amount_set_ptr;
}

AmountSetResult asDelete(AmountSet set, ASElement element){
    //what should we do with the iterator here?
    if (element == NULL){
        return AS_NULL_ARGUMENT;
    }
    if (!asContains(set, element)){
        return AS_ITEM_DOES_NOT_EXIST;
    }
    ElementNode ptr = set->first_node;
    //deleting the first element
    if (set->compareElements(element,ptr->element) == 0){
        set->first_node = set->first_node->next_node;
        set->freeElement(ptr->element);
        free(ptr);
        set->size--;
        return AS_SUCCESS;
    }
    while(ptr->next_node != NULL){
        //deleting the last element
        if(ptr->next_node->next_node == NULL){
            assert(set->compareElements(element,ptr->next_node->element)==0);
            ptr->next_node = NULL;
            set->freeElement(ptr->next_node->element);
            free(ptr->next_node);
            set->size--;
            return AS_SUCCESS;
        }
        //deleting an element in the middle
        if(set->compareElements(ptr->next_node->element,element) == 0){
            ElementNode to_delete = ptr->next_node;
            ptr->next_node = ptr->next_node->next_node;
            set->freeElement(to_delete->element);
            free(to_delete);
            set->size--;
            return AS_SUCCESS;
        }
        //moving to the next node
        ptr = ptr->next_node;
    }
    return AS_SUCCESS;
}

AmountSetResult asClear(AmountSet set){
    if (set == NULL) {
        return AS_NULL_ARGUMENT;
    }
    ElementNode ptr = set->first_node;
    //deleting all elements except the last one
    while (ptr->next_node != NULL) {
        ElementNode to_delete = ptr;
        ptr = ptr->next_node;
        set->freeElement(to_delete->element);
        free(to_delete);
    }
    //deleting the last node
    asDelete(set,ptr);
    return AS_SUCCESS;
}

void asDestroy(AmountSet set){
    if (set == NULL){
        return;
    }
    asClear(set); //clear all elements from the set
    free(set);
}

AmountSet asCopy(AmountSet set){
    if (set == NULL){
        return NULL;
    }
    AmountSet new_set = asCreate(set->copyElement,set->freeElement,set->compareElements);
    ElementNode ptr = set->first_node;
    while (ptr != NULL){
        if (asRegister(new_set,ptr->element) != AS_SUCCESS){
            return NULL;
        }
        ptr = ptr->next_node;
    }
    return new_set;
}

int asGetSize(AmountSet set){
    if (set == NULL){
        return (-1);
    }
    return set->size;
}

bool asContains(AmountSet set, ASElement element){
    if (IsNULL(set,element)){
        return false;
    }
    if (findElement(set,element) == NULL){
        return false;
    }
    return true;
}


AmountSetResult asGetAmount(AmountSet set, ASElement element, double *outAmount){
    if (IsNULL(set,element)){
        return AS_NULL_ARGUMENT;
    }
    ElementNode ptr = findElement(set,element);
   if (ptr == NULL){
        return AS_ITEM_DOES_NOT_EXIST;
    }
   *outAmount = ptr->amount;
   return AS_SUCCESS;
}

AmountSetResult asRegister(AmountSet set, ASElement element){
    if (IsNULL(set,element)){
        return AS_NULL_ARGUMENT;
    }
    if (asContains(set,element)){
        return AS_ITEM_ALREADY_EXISTS;
    }
    ElementNode new_node = createElementNode(set,element);
    if (new_node == NULL){
        return AS_NULL_ARGUMENT;
    }
    new_node->next_node = first_node; //adding the new element to the list's head
    return AS_SUCCESS;
}

AmountSetResult asChangeAmount(AmountSet set, ASElement element, const double amount){
    if (IsNULL(set,element)) {
        return AS_NULL_ARGUMENT;
    }
    ElementNode ptr = findElement(set,element);
    if (ptr == NULL){
        return AS_ITEM_DOES_NOT_EXIST;
    }
    if (ptr->amount + amount < 0)
    {
        return AS_INSUFFICIENT_AMOUNT;
    }
    ptr->amount += amount;
    return AS_SUCCESS;
}


static ElementNode findElement(AmountSet set, ElementNode element){
    assert(set != NULL && element != NULL);
    ElementNode ptr = set->first_node;
    while(ptr != NULL){
        if (set->compareElements(ptr->element,element) == 0){
            return ptr;
        }
        ptr = ptr->next_node;
    }
    return NULL;
}

static ElementNode createElementNode(AmountSet amount_set_ptr, ASElement element){
    ElementNode ptr = malloc(sizeof(*ptr));
    if (ptr == NULL){
        return NULL;
    }
    ptr->element = amount_set_ptr->copyElement(element);
    ptr->amount= 0;
    ptr->next_node = NULL;
    return ptr;
}

