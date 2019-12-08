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

typedef struct Product_t {
    char *name;
    unsigned int product_id;
    MatamazomAmountType amountType;
    MtmProductData customData;
    double sales;
    MtmCopyData copyData;
    MtmFreeData freeData;
    MtmGetProductPrice prodPrice;
} *Product;

typedef struct Order_t {
    unsigned int order_id;
    AmountSet products_in_order;
} *Order;

struct Matamazom_t {
    AmountSet storage;
    List orders;
    unsigned int number_of_orders;
};

static bool nameIsValid(const char *name);

static bool checkAmountType(double amount, MatamazomAmountType type);


static ASElement copyProduct(ASElement product);

static void freeProduct(ASElement product);

static int compareProduct(ASElement product1, ASElement product2);

static Product findProduct(AmountSet storage, const unsigned int id);

static ListElement copyOrder(ListElement order);

static void freeOrder(ListElement order);

static Order findOrder(List orders, unsigned int orderId);

static Product findProductInOrder(AmountSet products_in_order,
                                  unsigned int productId);

Matamazom matamazomCreate() {
    Matamazom matamazom = malloc(sizeof(*matamazom));
    if (matamazom == NULL) {
        return NULL;
    }
    matamazom->storage = NULL;
    matamazom->orders = NULL;
    matamazom->number_of_orders = 0;
    return matamazom;
}

void matamazomDestroy(Matamazom matamazom) {
    if (matamazom == NULL) {
        return;
    }
    asDestroy(matamazom->storage);
    listDestroy(matamazom->orders);
    free(matamazom);
}

MatamazomResult
mtmNewProduct(Matamazom matamazom, const unsigned int id, const char *name,
              const double amount, const MatamazomAmountType amountType,
              const MtmProductData customData, MtmCopyData copyData,
              MtmFreeData freeData, MtmGetProductPrice prodPrice) {
    if (matamazom == NULL || name == NULL || customData == NULL ||
        freeData == NULL ||
        prodPrice == NULL || copyData == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    if (matamazom->storage == NULL) {
        matamazom->storage = asCreate(copyProduct, freeProduct, compareProduct);
        if (matamazom->storage == NULL) {
            return MATAMAZOM_OUT_OF_MEMORY;
        }
    }
    assert(matamazom->storage != NULL);
    if (!nameIsValid(name)) {
        return MATAMAZOM_INVALID_NAME;
    }
    if (amount < 0 || !checkAmountType(amount, amountType)) {
        return MATAMAZOM_INVALID_AMOUNT;
    }
    Product new_product = malloc(sizeof(*new_product));
    if (new_product == NULL) {
        return MATAMAZOM_OUT_OF_MEMORY;
    }
    new_product->product_id = id;
    int str_len = strlen(name);
    new_product->name = malloc(sizeof(const char *) * str_len + 1);
    if (new_product->name == NULL) {
        return MATAMAZOM_OUT_OF_MEMORY;
    }
    strcpy(new_product->name, name);
    new_product->amountType = amountType;
    new_product->prodPrice = prodPrice;
    new_product->sales = 0;
    new_product->copyData = copyData;
    new_product->freeData = freeData;
    new_product->customData = copyData(customData);

    AmountSetResult registration_result = asRegister(matamazom->storage,
                                                     new_product);
    assert(registration_result != AS_NULL_ARGUMENT);
    freeProduct(new_product);

    Product registered_product = findProduct(matamazom->storage, id);

    if (registration_result == AS_ITEM_ALREADY_EXISTS) {
        return MATAMAZOM_PRODUCT_ALREADY_EXIST;
    }
    if (registration_result == AS_OUT_OF_MEMORY) {
        return MATAMAZOM_OUT_OF_MEMORY;
    }
    asChangeAmount(matamazom->storage, registered_product, amount);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmChangeProductAmount(Matamazom matamazom,
                                       const unsigned int id,
                                       const double amount) {
    if (matamazom == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    if (matamazom->storage == NULL) { // checks if the storage is empty
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    Product product_in_storage = findProduct(matamazom->storage, id);
    if (product_in_storage == NULL) {
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    if (!checkAmountType(amount, product_in_storage->amountType)) {
        return MATAMAZOM_INVALID_AMOUNT;
    }
    AmountSetResult changing_result = asChangeAmount(matamazom->storage,
                                                     product_in_storage,
                                                     amount);
    if (changing_result == AS_INSUFFICIENT_AMOUNT) {
        return MATAMAZOM_INSUFFICIENT_AMOUNT;
    }
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmClearProduct(Matamazom matamazom, const unsigned int id) {
    if (matamazom == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    if (matamazom->storage == NULL) { // the storage hasn't been initialized
        return MATAMAZOM_SUCCESS;
    }
    Product product_to_delete = findProduct(matamazom->storage, id);
    if (product_to_delete == NULL) {
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    LIST_FOREACH(Order, order, matamazom->orders) {
        AS_FOREACH(Product, product, order->products_in_order) {
            if (product->product_id == id) {
                asDelete(order->products_in_order, product);
                break;
            }
        }
    }
    //delete inner object in the product struct
    assert (matamazom->storage != NULL && product_to_delete != NULL);
    asDelete(matamazom->storage, product_to_delete);
    return MATAMAZOM_SUCCESS;
}

unsigned int mtmCreateNewOrder(Matamazom matamazom) {
    if (matamazom == NULL) {
        return 0;
    }
    if (matamazom->orders == NULL) {
        matamazom->orders = listCreate(copyOrder, freeOrder);
        if (matamazom->orders == NULL) {
            return 0;
        }
    }
    Order new_order = malloc(sizeof(*new_order));
    if (new_order == NULL) {
        return 0;
    }
    new_order->order_id = ++(matamazom->number_of_orders);
    new_order->products_in_order = NULL;
    ListResult creation_result = listInsertLast(matamazom->orders, new_order);
    // inserts the order in the end

    if (creation_result == LIST_NULL_ARGUMENT
        || creation_result == LIST_OUT_OF_MEMORY) {
        return 0;
    }
    unsigned int given_id = new_order->order_id;
    free(new_order);
    return given_id;
}

MatamazomResult mtmChangeProductAmountInOrder(Matamazom matamazom,
                                              const unsigned int orderId,
                                              const unsigned int productId,
                                              const double amount) {
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
    if (order_ptr->products_in_order == NULL) {
        order_ptr->products_in_order = asCreate(copyProduct, freeProduct,
                                                compareProduct);
    }
    AmountSetResult registration_result = asRegister(
            order_ptr->products_in_order, product_ptr);

    if (registration_result ==
        AS_ITEM_ALREADY_EXISTS) { // the product was already in the order
        AmountSetResult changing_result = asChangeAmount(
                order_ptr->products_in_order, product_ptr, amount);
        double updated_amount = 0;
        asGetAmount(order_ptr->products_in_order, product_ptr, &updated_amount);
        if (changing_result ==
            AS_INSUFFICIENT_AMOUNT || updated_amount == 0) {
            // if the amount to decrease was larger/equal than the amount in order
            assert(order_ptr->products_in_order != NULL && product_ptr != NULL);
            asDelete(order_ptr->products_in_order, product_ptr);
        }
        if (changing_result ==
            AS_SUCCESS) {
            // amount to increase was added the the amount of the product
            return MATAMAZOM_SUCCESS;
        }
    }
    if (registration_result ==
        AS_SUCCESS) { // item that was not in the order was added
        asChangeAmount(order_ptr->products_in_order, product_ptr,
                       amount); // * removed variable name
    }
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmShipOrder(Matamazom matamazom, const unsigned int orderId) {
    if (matamazom == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Order current_order = findOrder(matamazom->orders, orderId);
    if (current_order == NULL) {
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    double amount_in_order;
    double amount_in_storage;
    //check all products for insufficient amount
    AS_FOREACH(Product, prod_in_order,
               current_order->products_in_order) {
        Product product_in_storage = findProduct(matamazom->storage,
                                                 prod_in_order->product_id);
        asGetAmount(current_order->products_in_order, prod_in_order,
                    &amount_in_order);
        asGetAmount(matamazom->storage, product_in_storage, &amount_in_storage);
        if (amount_in_order > amount_in_storage) {
            return MATAMAZOM_INSUFFICIENT_AMOUNT;
        }
    }
    //update amount and sales for each product of the order in the storage
    AS_FOREACH(Product, prod_in_order,
               current_order->products_in_order) {
        Product product_in_storage = findProduct(matamazom->storage,
                                                 prod_in_order->product_id);
        asGetAmount(current_order->products_in_order, prod_in_order,
                    &amount_in_order);
        product_in_storage->sales += prod_in_order->prodPrice(
                prod_in_order->customData,
                amount_in_order);
        asChangeAmount(matamazom->storage, product_in_storage,
                       amount_in_order * -1);
    }

    //update order iterator to point to current order for deletion
    LIST_FOREACH(Order, searching_order, matamazom->orders) {
        if (searching_order->order_id == current_order->order_id) {
            break;
        };
    }
    listRemoveCurrent(matamazom->orders);
    return MATAMAZOM_SUCCESS;

}

MatamazomResult
mtmCancelOrder(Matamazom matamazom, const unsigned int orderId) {
    if (matamazom == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Order current_order = findOrder(matamazom->orders, orderId);
    if (current_order == NULL) {
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    //update order iterator to point to current order for deletion
    LIST_FOREACH(Order, searching_order, matamazom->orders) {
        if (searching_order->order_id == current_order->order_id) {
            break;
        }
    }
    listRemoveCurrent(matamazom->orders);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmPrintInventory(Matamazom matamazom, FILE *output) {
    if (matamazom == NULL || output == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    fprintf(output, "Inventory Status:\n");
    if (matamazom->storage == NULL) {
        return MATAMAZOM_SUCCESS;
    }
    AS_FOREACH(Product, product, matamazom->storage) {
        double product_amount;
        asGetAmount(matamazom->storage, product, &product_amount);
        double product_price = product->prodPrice(product->customData,
                                                  1);
        mtmPrintProductDetails(product->name, product->product_id,
                               product_amount, product_price, output);
    }
    return MATAMAZOM_SUCCESS;
}

MatamazomResult
mtmPrintOrder(Matamazom matamazom, const unsigned int orderId, FILE *output) {
    if (matamazom == NULL || output == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Order curr_order = findOrder(matamazom->orders, orderId);
    if (curr_order == NULL) {
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    double order_sum = 0;
    mtmPrintOrderHeading(curr_order->order_id, output);
    AS_FOREACH(Product, product, curr_order->products_in_order) {
        Product product_in_order = findProductInOrder(
                curr_order->products_in_order, product->product_id);
        double product_amount_in_order;
        asGetAmount(curr_order->products_in_order, product,
                    &product_amount_in_order);
        double total_product_price = product_in_order->prodPrice(
                product_in_order->customData, product_amount_in_order);
        product_in_order->prodPrice(product_in_order->customData,
                                    product_amount_in_order);
        mtmPrintProductDetails(product->name, product->product_id,
                               product_amount_in_order, total_product_price,
                               output);
        order_sum += total_product_price;
    }
    mtmPrintOrderSummary(order_sum, output);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmPrintBestSelling(Matamazom matamazom, FILE *output) {
    if (matamazom == NULL || output == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    fprintf(output, "Best Selling Product:\n");
    double best_seller_sales = 0;
    int best_seller_id = -1;
    //find id of best seller
    AS_FOREACH(Product, product, matamazom->storage) {
        if (product->sales > best_seller_sales) {
            best_seller_id = (int) product->product_id;
            best_seller_sales = product->sales;
            continue;
        }
        if (product->sales == best_seller_sales) {
            best_seller_id = (best_seller_id < product->product_id)
                             ? (int) best_seller_id : (int) product->product_id;
        }
    }
    if (best_seller_sales == 0) {
        fprintf(output, "none\n");
        return MATAMAZOM_SUCCESS;
    }
    Product best_seller_ptr = findProduct(matamazom->storage, best_seller_id);
    mtmPrintIncomeLine(best_seller_ptr->name, best_seller_ptr->product_id,
                       best_seller_sales, output);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult
mtmPrintFiltered(Matamazom matamazom, MtmFilterProduct customFilter,
                 FILE *output) {
    if (matamazom == NULL || customFilter == NULL || output == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }

    AS_FOREACH(Product, curr_product, matamazom->storage) {
        double product_amount;
        asGetAmount(matamazom->storage, curr_product, &product_amount);
        double product_price = (double) curr_product->prodPrice(
                curr_product->customData,
                product_amount / product_amount);
        if (customFilter(curr_product->product_id, curr_product->name,
                         product_amount, curr_product->customData)) {
            mtmPrintProductDetails(curr_product->name, curr_product->product_id,
                                   product_amount, product_price, output);
        }
    }
    return MATAMAZOM_SUCCESS;
}


static bool nameIsValid(const char *name) {
    if ((*name >= 'a' && *name <= 'z') || (*name >= 'A' && *name <= 'Z') ||
        (*name >= '0' && *name <= '9')) {
        return true;
    }
    return false;
}



static bool checkAmountType(double amount, MatamazomAmountType type) {
    bool result;
    double floored = floor(amount);
    switch (type) {
        case MATAMAZOM_INTEGER_AMOUNT:
            if ((amount - floored > 0.001) && (amount - floored < 0.999)) {
                result = false;
                break;
            }
        case MATAMAZOM_HALF_INTEGER_AMOUNT:
            if ((amount - floored > 0.001 && amount - floored < 0.499) ||
                (amount - floored > 0.501 && amount - floored < 0.999)) {
                result = false;
                break;
            }
        default:
            result = true;
    }
    return result;
}

static void freeProduct(ASElement product) {
    Product prod_to_delete = product;
    free(prod_to_delete->name);
    prod_to_delete->freeData(prod_to_delete->customData);
    free(prod_to_delete);
}

static int compareProduct(ASElement product1, ASElement product2) {
    Product prod1 = product1;
    Product prod2 = product2;
    return (int) (prod1->product_id) - (int) (prod2->product_id);
}

static ASElement copyProduct(ASElement product) {
    Product copy = malloc(sizeof(*copy));
    Product prod_to_be_copied = product;
    if (copy != NULL) {
        copy->product_id = prod_to_be_copied->product_id;
        int str_len = strlen(prod_to_be_copied->name);
        char *name = malloc(sizeof(char) * str_len + 1);
        if (name == NULL) {
            return NULL;
        }
        strcpy(name, prod_to_be_copied->name);
        copy->name = name;
        copy->amountType = prod_to_be_copied->amountType;
        copy->prodPrice = prod_to_be_copied->prodPrice;
        copy->sales = prod_to_be_copied->sales;
        copy->copyData = prod_to_be_copied->copyData;
        copy->freeData = prod_to_be_copied->freeData;
        copy->customData = prod_to_be_copied->copyData(
                prod_to_be_copied->customData);
    }
    return copy;
}

static ListElement copyOrder(ListElement order) {
    Order order_copy = order;
    Order copy = malloc(sizeof(*copy));
    if (copy == NULL) {
        return copy;
    }
    copy->order_id = order_copy->order_id;
    copy->products_in_order = asCopy(order_copy->products_in_order);
    return copy;
}

static void freeOrder(ListElement order) {
    Order order_to_delete = order;
    if (order_to_delete != NULL) {
        asDestroy(order_to_delete->products_in_order);
    }
    free(order_to_delete);
}

static Order findOrder(List orders, unsigned int orderId) {
    if (orders == NULL) {
        return NULL;
    }
    LIST_FOREACH(Order, tmp_product, orders) {
        if (tmp_product->order_id == orderId) {
            return tmp_product;
        }
    }
    return NULL;
}

static Product findProduct(AmountSet storage, const unsigned int id) {
    if (storage == NULL) {
        return NULL;
    }
    AS_FOREACH(Product, tmp_order, storage) {
        if (tmp_order->product_id == id) {
            return tmp_order;
        }
    }
    return NULL;
}

static Product
findProductInOrder(AmountSet products_in_order, unsigned int productId) {
    AS_FOREACH(Product, tmp_product, products_in_order) {
        if (tmp_product->product_id == productId) {
            return tmp_product;
        }
    }
    return NULL;
}


