//
// Created by Noam Raveh & Carmel Davidon 24/11/2019.
//
#include "list.h"
#include "order.h"
#include "product.h"
struct orderProduct_t {
    unsigned product_id;
    double amount;
    struct OrderProduct* next_prod_in_order;
};


struct order_t {
    unsigned int order_id;
    OrderProduct order_product;
    Order next_order;
};
