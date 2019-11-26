#include "matamazom.h"
#include "amount_set.h"
#include "set.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
//#include "product.h"
#include <math.h>

//needs to move to separate files
typedef struct Product_t {
    char* name;
    unsigned int id;
    MatamazomAmountType amountType;
    MtmProductData customData;
    double sales;
    MtmCopyData copyData;
    MtmFreeData freeData;
    MtmGetProductPrice prodPrice;
} *Product;

typedef struct order_t *Order;
typedef struct orderProduct_t *OrderProduct;

/*
struct orderProduct_t {
    unsigned int product_id;
    double amount;
    OrderProduct next_prod_in_order;
};*/


struct order_t {
    unsigned int order_id;
    AmountSet products_in_order;
    Order next_order;
};
/*
static SetElement copyOrder (SetElement prod_in_order){
    OrderProduct copy = malloc(sizeof(*copy));
    if (copy != NULL ){
        copy->product_id = ((OrderProduct)(prod_in_order))->product_id;
        copy->amount = ((OrderProduct)(prod_in_order))->amount;
        copy->next_prod_in_order = ((OrderProduct)(prod_in_order))->next_prod_in_order;
    }
    return copy;

}
static void freeOrder (SetElement prod_in_order){
    while ((OrderProduct)prod_in_order != NULL){
        OrderProduct to_delete = ((OrderProduct)(prod_in_order));
        prod_in_order = ((OrderProduct)(prod_in_order))->next_prod_in_order;
        free(to_delete);
    }
}
static int compareProductInOrder(SetElement product1, SetElement product2){
    return (int)(((OrderProduct)(product1))->product_id - ((OrderProduct)(product2))->product_id);
}
*/

static OrderProduct findProductInOrder (Order order, unsigned int product_id){
    OrderProduct ptr = order->prod_in_order;
    while (ptr != NULL){
        if (ptr->product_id == product_id){
            return ptr;
        }
        ptr = ptr->next_prod_in_order;
    }
    return ptr;
}
static bool nameValid (const char* name);
static bool checkAmountType (double amount, MatamazomAmountType type);
static double absDouble (double number);
static ASElement copyProduct(ASElement product);
static void freeProduct(ASElement product);
static int compareProduct(ASElement product1, ASElement product2);
static Product findProduct (AmountSet storage,const unsigned int id);
static Order findOrder (Set orders,const unsigned int id);
static void freeOrder(SetElement order);
static SetElement copyOrder(SetElement order);
static int compareOrder(SetElement order1, SetElement order2);


struct Matamazom_t {
    AmountSet storage;
    Set orders;
    unsigned int * number_of_orders;
};

Matamazom matamazomCreate() {
    Matamazom matamazom = malloc(sizeof(*matamazom));
    if (matamazom == NULL){
        return NULL;
    }
    //need to add NULL cases here (out of memory)
    matamazom->storage = asCreate(copyProduct,freeProduct,compareProduct);
    matamazom->orders = setCreate(copyOrder,freeOrder,compareOrder);
    *(matamazom->number_of_orders) = 0;
    return matamazom;
}

void matamazomDestroy(Matamazom matamazom){
    if (matamazom == NULL){
        return;
    }
    asDestroy(matamazom->storage);
    setDestroy(matamazom->orders);
    free(matamazom);
}

MatamazomResult mtmNewProduct(Matamazom matamazom, const unsigned int id, const char *name,
                              const double amount, const MatamazomAmountType amountType,
                              const MtmProductData customData, MtmCopyData copyData,
                              MtmFreeData freeData, MtmGetProductPrice prodPrice) {
    if (matamazom == NULL || name == NULL || customData == NULL || freeData == NULL ||
        prodPrice == NULL || copyData == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    assert(matamazom->storage != NULL);
    if (!nameValid(name)) {
        return MATAMAZOM_INVALID_NAME;
    }
    if (amount < 0 || !checkAmountType(amount, amountType)) {
        return MATAMAZOM_INVALID_AMOUNT;
    }
    Product new_prod = malloc(sizeof(*new_prod));
    if (new_prod == NULL) {
        return MATAMAZOM_OUT_OF_MEMORY;
    }
    new_prod->id = id;
    strcpy(new_prod->name, name);
    new_prod->amountType = amountType;
    new_prod->customData = new_prod->copyData(customData);
    new_prod->prodPrice = prodPrice;
    new_prod->sales = 0;
    new_prod->copyData = copyData;
    new_prod->freeData = freeData;

    AmountSetResult registration_result = asRegister(matamazom->storage, new_prod);
    assert(registration_result != AS_NULL_ARGUMENT);
    if (registration_result == AS_ITEM_ALREADY_EXISTS) {
        return MATAMAZOM_PRODUCT_ALREADY_EXIST;
    }
    if (registration_result == AS_OUT_OF_MEMORY) {
        return MATAMAZOM_OUT_OF_MEMORY;
    }
    //only positive amounts added here
    asChangeAmount(matamazom->storage, new_prod, amount);
     return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmChangeProductAmount(Matamazom matamazom, const unsigned int id, const double amount){
    if (matamazom == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Product current_product = findProduct(matamazom->storage,id);
    if (current_product == NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    AmountSetResult changing_result =  asChangeAmount(matamazom->storage,current_product,amount);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmClearProduct(Matamazom matamazom, const unsigned int id){
    if (matamazom == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Product to_delete_product = findProduct(matamazom->storage,id);
    if (to_delete_product == NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }

    to_delete_product->freeData(to_delete_product->customData); //delete inner object in the product struct
    AmountSetResult deletion_result = asDelete(matamazom->storage,to_delete_product);
    return MATAMAZOM_SUCCESS;
}


unsigned int mtmCreateNewOrder(Matamazom matamazom) {
    if (matamazom == NULL){
        return 0;
    }
    Order new_order = malloc(sizeof(*new_order));
    if (new_order == NULL) {
        return 0;
    }
    new_order->order_id = ++(*(matamazom->number_of_orders));
    SetResult creation_result = setAdd(matamazom->orders,new_order);
    if (creation_result == SET_OUT_OF_MEMORY){
        return 0;
    }
    assert (creation_result!= SET_ITEM_ALREADY_EXISTS);
    return new_order->order_id;

}

MatamazomResult mtmChangeProductAmountInOrder(Matamazom matamazom, const unsigned int orderId,
                                              const unsigned int productId, const double amount) {
    if (matamazom == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Order current_order = findOrder(matamazom->orders,orderId);
    if (current_order == NULL){
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    Product current_product_ptr = findProduct(matamazom->storage,productId);
    if (current_product_ptr == NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    if (!checkAmountType(amount,current_product_ptr->amountType)){
        return MATAMAZOM_INVALID_AMOUNT;
    }
    if (amount == 0) {
        return MATAMAZOM_SUCCESS;
    }
    OrderProduct amount_in_order = findProductInOrder(current_order, productId);

    if (amount_in_order == NULL) { //we need to add the product to the order

    }
    if (amount <= amount_in_order->amount)  { //we need to remove the product

    }




}

static bool nameValid (const char* name){
    if ((*name >= 'a' && *name<= 'z') || (*name >= 'A' && *name <= 'Z') || (*name >= '0' && *name <= '9')) {
        return true;
    }
    return false;
}
static double absDouble (double number){
    if (number < 0){
        return -number;
    }
    return number;

}
static bool checkAmountType (double amount, MatamazomAmountType type){
    bool result;
    switch (type){
        case MATAMAZOM_INTEGER_AMOUNT:
            result = absDouble(round(amount)-amount) < 0.001 ? true : false;
            break;
        case MATAMAZOM_HALF_INTEGER_AMOUNT:
            result = absDouble((round(amount*2)/2.0)-amount) < 0.001 ? true: false;
            break;
        default:
            result = true;
    }
    return result;
}

static void freeProduct(ASElement product){
    ((Product)product)->freeData(((Product)product)->customData);
    free(product);
}
static int compareProduct(ASElement product1, ASElement product2){
    return (int)(((Product)product1)->id) - (int)(((Product)(product2))->id);
}

static ASElement copyProduct(ASElement product) {
    Product copy = malloc(sizeof(*copy));
    if (copy != NULL) {
        copy->id = ((Product) (product))->id;
        strcpy(copy->name, ((Product) (product))->name);
        copy->amountType = ((Product) (product))->amountType;
        copy->customData = ((Product) (product))->copyData(((Product) (product))->customData);
        copy->prodPrice = ((Product) (product))->prodPrice;
        copy->sales = ((Product) (product))->sales;
        copy->copyData = ((Product) (product))->copyData;
        copy->freeData = ((Product) (product))->freeData;
    }
    return copy;
}

static SetElement copyOrder(SetElement order){
    AmountSet copy = asCopy(order);
    return copy;
}

static void freeOrder(SetElement order){
    asDestroy(order);
}
static int compareOrder(SetElement order1, SetElement order2){
    return (int)(((Order)order1)->order_id) - (int)(((Order)(order2))->order_id);
}
static Product findProduct (AmountSet storage,const unsigned int id){
    AS_FOREACH(Product,prod1,storage){
        if (prod1->id == id){
            return prod1;
        }
    }
    return NULL;
}

static Order findOrder (Set orders,const unsigned int id){
    AS_FOREACH(Order,curr_order,orders){
        if (curr_order->order_id == id){
            return curr_order;
        }
    }
    return NULL;
}



