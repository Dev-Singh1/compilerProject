#include "types.c"
#include <stdlib.h>

//sExpr constructor
customType* blockCons(void){ //creates an empty list block and returns a pointer to it
    customType* result = malloc(sizeof(customType));
    result->type = ValidSExpression;
    result->count = 0;
    result->block = NULL;
    return result;
}

//qExpr consturctor
customType* qExprCons(void){ //creates an empty list block and returns a pointer to it
    customType* result = malloc(sizeof(customType));
    result->type = ValidQExpression;
    result->count = 0;
    result->block = NULL;
    return result;
}


//destructor
void blockDel(customType* x){ //deletes list block and returns nothing
    switch (x->type){
        case ValidNum: break; //no need to call free when it is an int
        case ValidFloat: break; //similarly, ther is no need to deallocate heap memory when it is a float
        case ErrCode: free(x->err);break;
        case ValidIdentifier: free(x->id);break;
        //the above 2 cases find and free the apridriate fields of the object pointed to by the given pointer
        
        //the next case(s) uses a loid to free an entire list (all blocks reachable from the given pointer are freed)
        case ValidQExpression:
        case ValidSExpression:
            for(int i=0;i<x->count;i++){
                blockDel(x->block[i]);
            }
            //also freeing the pointers
            free(x->block);
        break;
    }
    //free the struct itself
    free(x);
}