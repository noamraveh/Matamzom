//
// Created by Noam Raveh & Carmel David on 18/11/2019.
//

#include "amount_set.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

struct AmountSet_t {
    ASElement* FirstNode;
    int size;
    ASElement* Iterator;


};
