//
// Created by Noam Raveh & Carmel David on 18/11/2019.
//

#include "amount_set.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#define IsNULL(ptr1,ptr2)    ((ptr1 == NULL || ptr2 == NULL) ? (true) : (false))

typedef struct ElementNode_t* ElementNode;
struct ElementNode_t {
    ASElement element;
    double amount;
    ElementNode next_node;
} ;

struct AmountSet_t {
    ElementNode first_node;
    ElementNode iterator;
    int size;
    CopyASElement copyElement;
    FreeASElement freeElement;
    CompareASElements compareElements;
};

static ElementNode createElementNode(AmountSet amount_set_ptr, ASElement element);
static ElementNode findElement(AmountSet set, ElementNode element);

AmountSet asCreate(CopyASElement copyElement,
                   FreeASElement freeElement,
                   CompareASElements compareElements){
    if (copyElement == NULL || freeElement == NULL || compareElements == NULL){
        return NULL;
    }

    AmountSet as_ptr = malloc(sizeof(*as_ptr));
    if (as_ptr == NULL){
        return NULL;
    }
    assert(as_ptr!= NULL);
    as_ptr->size = 0;
    as_ptr->iterator = NULL;
    as_ptr->copyElement = copyElement;
    as_ptr->freeElement = freeElement;
    as_ptr->compareElements = compareElements;
    as_ptr->first_node =  NULL;
       /*     createElementNode(as_ptr,NULL);
    if (as_ptr->first_node == NULL) {
        free(as_ptr);
        return NULL;
    }*/
    return as_ptr;
}

AmountSetResult asDelete(AmountSet set, ASElement element){
    if (element == NULL){
        return AS_NULL_ARGUMENT;
    }
    if (!asContains(set, element)){
        return AS_ITEM_DOES_NOT_EXIST;
    }
    ElementNode ptr = set->first_node;
    assert(ptr != NULL);
    if (set->compareElements(element,ptr->element) == 0){ //deleting the first element
        set->first_node = set->first_node->next_node;
        set->freeElement(ptr->element);
        free(ptr);
        set->size--;
        set->iterator = NULL;
        return AS_SUCCESS;
    }
    while(ptr->next_node != NULL){
        if(ptr->next_node->next_node == NULL){ //deleting the last element
            assert(set->compareElements(element,ptr->next_node->element)==0);
            set->freeElement(ptr->next_node->element);
            free(ptr->next_node);
            ptr->next_node = NULL;
            set->size--;
            set->iterator = NULL;
            return AS_SUCCESS;
        }
        if(set->compareElements(ptr->next_node->element,element) == 0){ //deleting an element in the middle
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

AmountSetResult asClear(AmountSet set){
    if (set == NULL) {
        return AS_NULL_ARGUMENT;
    }
    if (set->first_node == NULL){
        return AS_SUCCESS;
    }
    ElementNode ptr = set->first_node;

    while (ptr != NULL) {  //deleting all elements except the last one
        ElementNode to_delete = ptr;
        ptr = ptr->next_node;
        set->freeElement(to_delete->element);
        free(to_delete);
        set->size--;
    }
 //   asDelete(set,ptr);     //deleting the last node
    set->first_node = NULL;
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
    if (new_set == NULL) {
        return NULL;
    }
    assert(new_set!= NULL);
    ElementNode ptr = set->first_node;
    while (ptr != NULL){
        if (asRegister(new_set,ptr->element) != AS_SUCCESS){
            asDestroy(new_set);
            return NULL;
        }
        ptr = ptr->next_node;
    }
    //should we change the iterator of the original set if new set failed?
    set->iterator = NULL;
    new_set->iterator = NULL;
    return new_set;
}

int asGetSize(AmountSet set){
    if (set == NULL){
        return -1;
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
    /*if (set->first_node->element == NULL){
        set->first_node->element = element;
        set->size++;
        set->iterator = NULL;
        return AS_SUCCESS;
    }*/

    ElementNode new_node = createElementNode(set,element);
    if (new_node == NULL){
        return AS_OUT_OF_MEMORY;
    }
    assert(new_node != NULL);
    new_node->next_node = set->first_node; //adding the new element to the list's head
    set->first_node = new_node;
    set->size++;
    set->iterator = NULL;
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


ASElement asGetFirst(AmountSet set){
    if (set == NULL){
        return NULL;
    }
    ElementNode ptr = set->first_node;
    if (ptr == NULL){
        return NULL;
    }
    if (ptr->next_node == NULL){
        set->iterator = ptr;
    }
    while (ptr->next_node != NULL){//find smallest node according to compare
        if (set->compareElements(ptr->element, ptr->next_node->element) > 0){//next node < current node
            set->iterator = ptr->next_node;
        }
        ptr = ptr->next_node;
    }
    return set->iterator->element;
}

ASElement asGetNext(AmountSet set){
    if (set == NULL || set->iterator == NULL){
        return NULL;
    }
    assert(set);

    ElementNode iterating_ptr = set->first_node;
    ElementNode chosen_ptr = NULL;

    while (iterating_ptr != NULL) { //find first element greater than current iterator
        if (set->compareElements (iterating_ptr->element,set->iterator->element)>0) {
            chosen_ptr = iterating_ptr;
            break;
        }
        iterating_ptr = iterating_ptr->next_node;
    }

    if (chosen_ptr == NULL){
        return NULL;
    }

    iterating_ptr = set->first_node; //initialize iterator

    while (iterating_ptr != NULL) {
        if (set->compareElements(iterating_ptr->element,set->iterator->element) > 0 &&
        set->compareElements(iterating_ptr->element,chosen_ptr->element) < 0) {
            chosen_ptr = iterating_ptr;
        }
        iterating_ptr = iterating_ptr->next_node;
    }
    set->iterator = chosen_ptr;
    return chosen_ptr->element;
}




static ElementNode findElement(AmountSet set, ElementNode element){
    assert(set != NULL && element != NULL);
    ElementNode ptr = set->first_node;
    while(ptr != NULL){
        if(ptr->element == NULL){
             break;
        }
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
/*    ptr->element = malloc(sizeof(*element));
    if(ptr->element == NULL){
        return NULL;
    }*/
    ptr->amount = 0;
    ptr->next_node = NULL;
    ptr->element = amount_set_ptr->copyElement(element);
    return ptr;
}

