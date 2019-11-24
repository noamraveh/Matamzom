//
// Created by Noam Raveh & Carmel David on 18/11/2019.
//

#include "product.h"
#include "matamazom.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>


struct Products_t {
    char* name;
    unsigned int id;
    MatamazomAmountType amountType;
    MtmProductData customData;
    double sales;
    MtmCopyData copyData;
    MtmFreeData freeData;
    MtmGetProductPrice prodPrice;
};


