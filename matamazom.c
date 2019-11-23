#include "matamazom.h"
#include "amount_set.h"
#include "list.h"
//#include "product.h"
//#include "order.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

struct Matamazom_t {
    AmountSet storage;
    List orders;
};

Matamazom matamzomCreate() {
    Matamazom warehouse = malloc(sizeof(*warehouse));
    if (warehouse == NULL){
        return NULL;
    }
    warehouse->storage = NULL;
    warehouse->orders = NULL;
    return warehouse;
}

void matamazomDestroy(Matamazom matamazom){

}