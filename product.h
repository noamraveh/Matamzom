#ifndef PRODUCT_H_
#define PRODUCT_H_

#include <stdio.h>
#include <stdbool.h>
#include "amount_set.h"

/**
 * Product Struct Template
 *
 * Implements a sorted amount set container type.
 * The set has an internal iterator for external use. For all functions
 * where the state of the iterator after calling that function is not stated,
 * it is undefined. That is you cannot assume anything about it.
 * The set is sorted in ascending order - iterating over the set is done in the
 * same order.
 *
 * The following functions are available:
 *   asCreate           - Creates a new empty set
 *   asDestroy          - Deletes an existing set and frees all resources
 *   asCopy             - Copies an existing set
 *   asGetSize          - Returns the size of the set
 *   asContains         - Checks if an element exists in the set
 *   asGetAmount         - Returns the amount of an element in the set
 *   asRegister         - Add a new element into the set
 *   asChangeAmount     - Increase or decrease the amount of an element in the set
 *   asDelete           - Delete an element completely from the set
 *   asClear            - Deletes all elements from target set
 *   asGetFirst         - Sets the internal iterator to the first element
 *                        in the set, and returns it.
 *   asGetNext          - Advances the internal iterator to the next element
 *                        and returns it.
 *   AS_FOREACH         - A macro for iterating over the set's elements
 */

/** Type for defining the product */
typedef struct Product_t *Product;

#endif /* AMOUNT_SET_H_ */
