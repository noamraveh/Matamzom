//
// Created by Noam Raveh & Carmel David on 18/11/2019.
//

#include "product.h"
#include "matamazom.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>


struct Product_t {
    char* prod_name;
    unsigned int id;
    MatamazomAmountType prod_data;
    enum amount_type;
    double sales;
};

