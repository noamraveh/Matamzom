#include "matamazom.h"
#include "amount_set.h"
#include "list.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

typedef struct Product_t *Product;
struct Product_t {
    char* name;
    unsigned int product_id;
    MatamazomAmountType amountType;
    MtmProductData customData;
    double sales;
    MtmCopyData copyData;
    MtmFreeData freeData;
    MtmGetProductPrice prodPrice;
};

typedef struct order_t *Order;
struct order_t {
    unsigned int order_id;
    AmountSet products_in_order;
};

struct Matamazom_t {
    AmountSet storage;
    List orders;
    unsigned int number_of_orders;
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
*/
static bool nameValid (const char* name);
static bool checkAmountType (double amount, MatamazomAmountType type);
static double absDouble (double number);
static ASElement copyProduct(ASElement product);
static void freeProduct(ASElement product);
static int compareProduct(ASElement product1, ASElement product2);
static Product findProduct (AmountSet storage,const unsigned int id);
static ListElement copyOrderProducts(ListElement order);
static void freeOrderProducts (ListElement order);
//int compareOrderId (ListElement id1 , ListElement id2);
static Order findOrder(List orders, unsigned int orderId);
static Product findProductInOrder(AmountSet products_in_order, unsigned int productId);




/*static Order findOrder (Set orders,const unsigned int id);
static void freeOrder(SetElement order);
static SetElement copyOrder(SetElement order);
static int compareOrder(SetElement order1, SetElement order2);
*/

Matamazom matamazomCreate(){
    Matamazom matamazom = malloc (sizeof(*matamazom));
    if (matamazom == NULL){
        return NULL;
    }
    matamazom->storage = NULL;
    matamazom ->orders = NULL;
    matamazom->number_of_orders = 0;
    return matamazom;
}
/*
Matamazom matamazomCreate() {
    Matamazom matamazom = malloc(sizeof(*matamazom));
    if (matamazom == NULL){
        return NULL;
    }
    matamazom->storage = asCreate(copyProduct,freeProduct,compareProduct);
    if (matamazom->storage == NULL){
        return NULL;
    }
    matamazom->orders = listCreate(asCopy
    if (matamazom->orders == NULL){
        return NULL;
    }
    *(matamazom->number_of_orders) = 0;
    return matamazom;
}*/

void matamazomDestroy(Matamazom matamazom){
    if (matamazom == NULL){
        return;
    }
    asDestroy(matamazom->storage);
    listDestroy(matamazom->orders);
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
    if(matamazom->storage == NULL){
        // freeProducts didn't fill itself (needs to be checked)
        matamazom->storage = asCreate(copyProduct,freeProduct, compareProduct);
        if (matamazom->storage == NULL){
            return MATAMAZOM_OUT_OF_MEMORY;
        }
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
    new_prod->product_id = id;
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
    if (matamazom == NULL ){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    if (matamazom->storage == NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    Product current_product = findProduct(matamazom->storage,id);
    if (current_product == NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    if (!checkAmountType(amount,current_product->amountType)){
        return MATAMAZOM_INVALID_AMOUNT;
    }
    AmountSetResult changing_result = asChangeAmount(matamazom->storage,current_product,amount);
    if (changing_result == AS_INSUFFICIENT_AMOUNT){
        return MATAMAZOM_INSUFFICIENT_AMOUNT;
    }
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmClearProduct(Matamazom matamazom, const unsigned int id){
    if (matamazom == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    if (matamazom->storage == NULL){
        return MATAMAZOM_SUCCESS;
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
    // freeOrderProducts didn't fill itself (needs to be checked)
    matamazom->orders = listCreate(copyOrderProducts,freeOrderProducts);
    Order new_order = malloc(sizeof(*new_order));
    if (new_order == NULL) {
        return 0;
    }
    new_order->order_id = ++(matamazom->number_of_orders);
    new_order->products_in_order = asCreate(copyProduct, freeProduct,compareProduct);
    ListResult creation_result = listInsertLast(matamazom->orders,new_order);
    if (creation_result == LIST_NULL_ARGUMENT || creation_result == LIST_OUT_OF_MEMORY){
        return 0;
    }
    return new_order->order_id;
}

MatamazomResult mtmChangeProductAmountInOrder(Matamazom matamazom, const unsigned int orderId,
                                              const unsigned int productId, const double amount) {
    if (matamazom == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    if (matamazom->orders == NULL) {
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    Order order_ptr = findOrder(matamazom->orders, orderId);
    if (order_ptr == NULL) {
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    Product product_ptr = findProduct(matamazom->storage, productId);
    if (product_ptr == NULL) {
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    if (!checkAmountType(amount, product_ptr->amountType)) {
        return MATAMAZOM_INVALID_AMOUNT;
    }
    // Product is valid in storage, amountType is valid and order exists
    if (amount == 0) {
        return MATAMAZOM_SUCCESS;
    }
    assert(order_ptr != NULL && product_ptr != NULL); // order exists and product exists in storage
    // Product product_exists_in_order = findProductInOrder (order_ptr->products_in_order, productId);
    //if (amount > 0  && product_exists_in_order == NULL ){
    AmountSetResult registration_result = asRegister(order_ptr->products_in_order, product_ptr);
    if (registration_result == AS_ITEM_ALREADY_EXISTS) {
        AmountSetResult changing_result = asChangeAmount(order_ptr->products_in_order, product_ptr, amount);
        if (changing_result == AS_INSUFFICIENT_AMOUNT) {
            assert(order_ptr->products_in_order != NULL && product_ptr != NULL);
            AmountSetResult deletion_result = asDelete(order_ptr->products_in_order, product_ptr);
        }
        if (changing_result == AS_SUCCESS) {
            return MATAMAZOM_SUCCESS;
        }
    }
    if (registration_result == AS_SUCCESS) {
        AmountSetResult changing_result = asChangeAmount(order_ptr->products_in_order, product_ptr, amount);
    }

    return MATAMAZOM_SUCCESS;
}

    Product amount_in_order = findProductInOrder(current_order_ptr, productId);

    if (amount_in_order == NULL) { //we need to add the product to the order
        asRegister(current_order_ptr,current_product_ptr);
    }
    if (amount <= amount_in_order->)  { //we need to remove the product
        asDelete(current_order_ptr,current_product_ptr);
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
    Product prod_to_delete = product;
    prod_to_delete->freeData(prod_to_delete->customData);
    free(prod_to_delete);
}

static int compareProduct(ASElement product1, ASElement product2){
    Product prod1 = product1;
    Product prod2 = product2;
    return (int)(prod1->product_id) - (int)(prod2->product_id);
}
static ASElement copyProduct(ASElement product) {
    Product copy = malloc(sizeof(*copy));
    Product prod_to_be_copied = product;
    if (copy != NULL) {
        copy->product_id = prod_to_be_copied->product_id;
        strcpy(copy->name, prod_to_be_copied->name);
        copy->amountType = prod_to_be_copied->amountType;
        copy->customData = prod_to_be_copied->copyData(prod_to_be_copied->customData);
        copy->prodPrice =prod_to_be_copied->prodPrice;
        copy->sales = prod_to_be_copied->sales;
        copy->copyData = prod_to_be_copied->copyData;
        copy->freeData = prod_to_be_copied->freeData;
    }
    return copy;
}
int compareOrderId (ListElement id1 , ListElement id2){
    Order order1 = id1;
    Order order2 = id2;
    return (int)(order2->order_id) - (int)(order1->order_id);
}

static ListElement copyOrderProducts(ListElement order){
    Order order_copy = order;
    Order copy = malloc(sizeof(*copy));
    if(copy == NULL){
        return copy;
    }
    copy->order_id = order_copy->order_id;
    copy->products_in_order = asCopy(order_copy->products_in_order);
    return copy;
}
static void freeOrderProducts (ListElement order){
    Order order_to_delete = order;
    asDestroy(order_to_delete->products_in_order);
}
static Order findOrder(List orders, unsigned int orderId){
    if (orders == NULL){
        return NULL;
    }
    LIST_FOREACH(Order, curr_order, orders){
        if (curr_order->order_id == orderId){
            return curr_order;
        }
        return NULL;
    }

}

static Product findProduct (AmountSet storage ,const unsigned int id){
    AS_FOREACH(Product,prod1,storage){
        if (prod1->product_id == id){
            return prod1;
        }
    }
    return NULL;
}
static Product findProductInOrder(AmountSet products_in_order, unsigned int productId){
    AS_FOREACH(Product, curr_prod, products_in_order){
        if (curr_prod->product_id == productId){
            return curr_prod;
        }
    }return NULL;
}
/*
static Order findOrder (Set orders,const unsigned int id){
    SET_FOREACH(Order,curr_order,orders){
        if (curr_order->order_id == id){
            return curr_order;
        }
    }
    return NULL;
}
*/


