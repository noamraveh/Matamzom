#include "matamazom.h"
#include "amount_set.h"
#include "list.h"
//#include "product.h"
//#include "order.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

static bool nameValid (const char* name);
static bool checkAmountType (double amount, MatamazomAmountType type);
static double absDouble (double number);
static ASElement copyProduct(ASElement product);
static void freeProduct(ASElement product);
static int compareProduct(ASElement product1, ASElement product2);

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

struct Matamazom_t {
    AmountSet storage;
    List orders;
};

Matamazom matamzomCreate() {
    Matamazom matamazom = malloc(sizeof(*matamazom));
    if (matamazom == NULL){
        return NULL;
    }
    matamazom->storage = asCreate(copyProduct,freeProduct,compareProduct);
    //need to be created when we reach the order part of the project
   // matamazom->orders = listCreate(copyOrder, freeOrder);
    return matamazom;
}

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

    AmountSetResult result = asRegister(matamazom->storage, new_prod);
    assert(result != AS_NULL_ARGUMENT);
    if (result == AS_ITEM_ALREADY_EXISTS) {
        return MATAMAZOM_PRODUCT_ALREADY_EXIST;
    }
    if (result == AS_OUT_OF_MEMORY) {
        return MATAMAZOM_OUT_OF_MEMORY;
    }
    //only positive amounts added here
    asChangeAmount(matamazom->storage, new_prod, amount);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmChangeProductAmount(Matamazom matamazom, const unsigned int id, const double amount){
    if (matamazom == NULL || )
}

MatamazomResult mtmClearProduct(Matamazom matamazom, const unsigned int id){
   // AmountSetResult cleared = asDelete(matamazom->storage,);
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
        copy->id = ((Product)(product))->id;
        strcpy(copy->name,((Product)(product))->name);
        copy->amountType = ((Product)(product))->amountType;
        copy->customData = ((Product)(product))->copyData(((Product)(product))->customData);
        copy->prodPrice = ((Product)(product))->prodPrice;
        copy->sales = ((Product)(product))->sales;
        copy->copyData = ((Product)(product))->copyData;
        copy->freeData = ((Product)(product))->freeData;
    }
    return copy;
}