#include "matamazom.h"
#include "amount_set.h"
#include "matamazom_print.h"
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

static bool nameValid (const char* name);
static bool checkAmountType (double amount, MatamazomAmountType type);
static double absDouble (double number);
static ASElement copyProduct(ASElement product);
static void freeProduct(ASElement product);
static int compareProduct(ASElement product1, ASElement product2);
static Product findProduct (AmountSet storage,const unsigned int id);
static ListElement copyOrderProducts(ListElement order);
static void freeOrderProducts (ListElement order);
static Order findOrder(List orders, unsigned int orderId);
static Product findProductInOrder(AmountSet products_in_order, unsigned int productId);

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
    if (matamazom->storage == NULL){ // checks if the storage is empty
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    Product product_in_storage = findProduct(matamazom->storage,id);
    if (product_in_storage == NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    if (!checkAmountType(amount,product_in_storage->amountType)){
        return MATAMAZOM_INVALID_AMOUNT;
    }
    AmountSetResult changing_result = asChangeAmount(matamazom->storage,product_in_storage,amount);
    if (changing_result == AS_INSUFFICIENT_AMOUNT){
        return MATAMAZOM_INSUFFICIENT_AMOUNT;
    }
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmClearProduct(Matamazom matamazom, const unsigned int id){
    if (matamazom == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    if (matamazom->storage == NULL){ // the storage hasn't been initialized - no products in storage
        return MATAMAZOM_SUCCESS;
    }
    Product product_to_delete = findProduct(matamazom->storage,id);
    if (product_to_delete == NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    LIST_FOREACH(Order,order,matamazom->orders){
        AS_FOREACH(Product,product,order->products_in_order){
            if (product->product_id == id){
                asDelete(order->products_in_order,product);
                break;
            }
        }
    }
    product_to_delete->freeData(product_to_delete->customData); //delete inner object in the product struct
    assert (matamazom->storage != NULL && product_to_delete != NULL);
    AmountSetResult deletion_result = asDelete(matamazom->storage,product_to_delete);
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
    // again freeProduct didn't fill itself (needs to be checked)
    new_order->products_in_order = asCreate(copyProduct,freeProduct,compareProduct);
    ListResult creation_result = listInsertLast(matamazom->orders,new_order); // inserts the order in the end
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
    // Product exists in storage, amountType is valid and order exists
    if (amount == 0) {
        return MATAMAZOM_SUCCESS;
    }
    assert(order_ptr != NULL && product_ptr != NULL);
    // registering the product to the order

    AmountSetResult registration_result = asRegister(order_ptr->products_in_order, product_ptr);

    if (registration_result == AS_ITEM_ALREADY_EXISTS) { // the product was already in the order
        AmountSetResult changing_result = asChangeAmount(order_ptr->products_in_order, product_ptr, amount);
        if (changing_result == AS_INSUFFICIENT_AMOUNT) { // if the amount to decrease was larger than the amount in order
            assert(order_ptr->products_in_order != NULL && product_ptr != NULL);
            asDelete(order_ptr->products_in_order, product_ptr); // * removed variable name
        }
        if (changing_result == AS_SUCCESS) { // amount to increase was added the the amount of the product
            return MATAMAZOM_SUCCESS;
        }
    }
    if (registration_result == AS_SUCCESS) { // item that was not in the order was added
        asChangeAmount(order_ptr->products_in_order, product_ptr, amount); // * removed variable name
    }
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmShipOrder(Matamazom matamazom, const unsigned int orderId){
    if (matamazom == NULL ){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Order current_order = findOrder(matamazom->orders,orderId);
    if (current_order == NULL){
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    double amount_in_order;
    double amount_in_storage;
    //check all products for insufficient amount
    AS_FOREACH(Product,current_prod_in_order,current_order->products_in_order){
        Product product_in_storage = findProduct(matamazom->storage,current_prod_in_order->product_id);
        asGetAmount(current_order->products_in_order,current_prod_in_order,&amount_in_order);
        asGetAmount(matamazom->storage,product_in_storage,&amount_in_storage);
        if (amount_in_order>amount_in_storage){
            return MATAMAZOM_INSUFFICIENT_AMOUNT;
        }
    }
    //update amount and sales for each product of the order in the storage
    AS_FOREACH(Product,current_prod_in_order,current_order->products_in_order){
        Product product_in_storage = findProduct(matamazom->storage,current_prod_in_order->product_id);
        asGetAmount(current_order->products_in_order,current_prod_in_order,&amount_in_order);
        asChangeAmount(matamazom->storage,product_in_storage,amount_in_order);
        product_in_storage->sales += current_prod_in_order->prodPrice(current_prod_in_order->customData,amount_in_order);
    }

    //update order iterator to point to current order for deletion
    LIST_FOREACH(Order,searching_order,matamazom->orders){
        if(searching_order->order_id == current_order->order_id){
            break;
        };
    }
    listRemoveCurrent(matamazom->orders);
    return MATAMAZOM_SUCCESS;

}
MatamazomResult mtmCancelOrder(Matamazom matamazom, const unsigned int orderId){
    if (matamazom == NULL ){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Order current_order = findOrder(matamazom->orders,orderId);
    if (current_order == NULL){
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    //update order iterator to point to current order for deletion
    LIST_FOREACH(Order,searching_order,matamazom->orders){
        if(searching_order->order_id == current_order->order_id){
            break;
        }
    }
    listRemoveCurrent(matamazom->orders);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmPrintInventory(Matamazom matamazom, FILE *output){
    if (matamazom == NULL || output == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    fprintf(output,"Inventory Status:\n");
    if (matamazom->storage == NULL){
        fclose(output);
        return MATAMAZOM_SUCCESS;
    }
    AS_FOREACH(Product,product,matamazom->storage){
        double product_amount;
        asGetAmount(matamazom->storage,product,&product_amount);
        double product_price = product->prodPrice(product->customData,product_amount);
        mtmPrintProductDetails(product->name,product->product_id,product_amount,product_price,output);
    }
    fclose(output);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmPrintOrder(Matamazom matamazom, const unsigned int orderId, FILE *output){
    if (matamazom == NULL || output == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
// need to add case of no orders
    Order curr_order = findOrder(matamazom->orders,orderId);
    double order_sum=0;
    AS_FOREACH(Product ,product,curr_order->products_in_order){
        Product product_in_order = findProductInOrder(curr_order->products_in_order,product->product_id);
        double product_amount_in_order;
        asGetAmount(curr_order->products_in_order,curr_order,&product_amount_in_order);
        double total_product_price = product_in_order->prodPrice(product_in_order->customData,product_amount_in_order);
        product_in_order->prodPrice(product_in_order->customData,product_amount_in_order);

        mtmPrintOrderHeading(curr_order->order_id,output);
        mtmPrintProductDetails(product->name,product->product_id,product_amount_in_order,total_product_price,output);
        order_sum +=total_product_price;
    }
    mtmPrintOrderSummary(order_sum,output);
    fclose(output);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmPrintBestSelling(Matamazom matamazom, FILE *output){
    if (matamazom == NULL || output == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }

    double best_seller_sales = 0;
    int best_seller_id = -1;
    //find id of best seller
    AS_FOREACH(Product,product,matamazom->storage){
        if (product->sales > best_seller_sales) {
            best_seller_id = (int) product->product_id;
            continue;
        }
        if (product->sales == best_seller_sales){
            best_seller_id = (best_seller_id < product->product_id) ? (int)best_seller_id : (int)product->product_id;
        }
    }
    Product best_seller_ptr = findProductInOrder(matamazom->storage,best_seller_id);
    fprintf(output,"Best Selling Product:\n");
    if (best_seller_id == -1){
        fprintf(output,"none");
        fclose(output);
        return MATAMAZOM_SUCCESS;
    }
    mtmPrintIncomeLine(best_seller_ptr->name,best_seller_id,best_seller_sales,output);
    fclose(output);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmPrintFiltered(Matamazom matamazom, MtmFilterProduct customFilter, FILE *output){
    if (matamazom == NULL || output == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }

    AS_FOREACH(Product,curr_product,matamazom->storage){
        double product_amount;
        asGetAmount(matamazom->storage,curr_product,&product_amount);
        double product_price = curr_product->prodPrice(curr_product->customData,product_amount);
        if(customFilter(curr_product->product_id,curr_product->name,product_amount,curr_product->customData)){
            mtmPrintProductDetails(curr_product->name,curr_product->product_id,product_amount,product_price,output);
        }
    }
    return MATAMAZOM_SUCCESS;
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
static Order findOrder(List orders, unsigned int orderId) {
    if (orders == NULL) {
        return NULL;
    }
    LIST_FOREACH(Order, curr_order, orders) {
        if (curr_order->order_id == orderId) {
            return curr_order;
        }
    }
    return NULL;
}
static Product findProduct (AmountSet storage ,const unsigned int id){
    if (storage == NULL){
        return NULL;
    }
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


