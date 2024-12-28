#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
/*forward declarations*/

struct customType;
struct env;
typedef struct customType customType;
typedef struct env env;

//enums for code readablity
enum {ValidNum, ValidFloat, ErrCode, ValidIdentifier, ValidSExpression, ValidQExpression, ValidFunction}; //possible types

//The following typedef allows us to have a 'function pointer' type; this is done to accomodate variable definitions and lookups
typedef customType*(*funcPtr)(env*, customType*);

//this struct introduces the custom type; through which we are representing all data as a list (internally)
struct customType{
    //primitive types
    int type; //expected and/or valid type of ideration; this tells us weather we are going to access the num feild or the err field of this struct
    long num; //value
    double flnum; //float value
    char* err; //error code
    char* id; //identifier

    //function types
    funcPtr func; //function pointer (if null we use the fields below to identify and execute user defined funtions)
    env* e;// pointer to funciton enviorment
    customType* parameters; // pointer to list of function params
    customType* functionBody; //pointer to expression that represents the function body

    //generic expression type
    int count; //analogous to argc (count of tokens? (idk what else to call them))
    customType** block; //analogus to argv (vector of tokens? (idk what else to call them))
};

//this struct introduces an enviorment type that is used to keep track of scopes and symbol tables
struct env{
    env* parent; //parent enviorment (for inheritance/dynamic scoping)
    int count; //number of entries in the symbol table for this enviorment/scope
    char** ids; //pointer to string (char*); it acts as a list of identifier names
    customType** values; //pointer to pointer to a custom type; it acts as a list of pointers where each pointer points to an object of type customType and contains the relavent value
};

//funcitons to convert primitive types to our customType (list cells and constituent tokens)
customType* typeNum(long x){
    customType* result = malloc(sizeof(customType));
    result->type = ValidNum;
    result->num = x;
    return result;
}

customType* typeFloat(double x){
    customType* result = malloc(sizeof(customType));
    result->type = ValidFloat;
    result->flnum = x;
    return result;
}
// err function now uses a variable argument list
customType* typeErr(char* x, ...){
    customType* result = malloc(sizeof(customType));
    result->type = ErrCode;

    // create a va list
    va_list va;
    va_start(va, x);


    result->err = malloc(512); //max error message size

    vsnprintf(result->err, 511, x, va); // print the error
    
    //realloc memory and clean the va list
    result->err=realloc(result->err, strlen(result->err)+1);
    va_end(va);


    return result;
}

customType* typeIdentifier(char* x){
    customType* result = malloc(sizeof(customType));
    result->type = ValidIdentifier;
    result->id = malloc(strlen(x)+1);
    strpy(result->id, x);
    return result;
}

customType* typeFunction(funcPtr f){
    customType* result = malloc(sizeof(customType));
    result->type = ValidFunction;
    result->func = f;
    return result;
}
//constructor for user defined functions
customType* typeLambda(customType* params, customType* body){
    customType* result = malloc(sizeof(customType));
    result->type = ValidFunction;
    result->e = newEnv();
    result->parameters = params;
    result->functionBody = body;
    result->func = NULL;
    return result;

} 

//s and q expr helper functions

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


//recursive destructor
void blockDel(customType* x){ //deletes list block and returns nothing
    switch (x->type){
        case ValidNum: break; //no need to call free when it is an int
        case ValidFloat: break; //similarly, ther is no need to deallocate heap memory when it is a float
        case ValidFunction:
            if(x->func != NULL){
                break;
            }else{
                delEnv(x->e);
                blockDel(x->parameters);
                blockDel(x->functionBody);
                break;
            }
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

//helper function that makes a copy of the provided target
customType* blkCopy(customType* target){
    customType* result = malloc(sizeof(customType));
    result->type=target->type;
    switch (target->type){
        case ValidFunction:
            if(target->func!=NULL){
                result->func = target->func;
                break;
            }else{
                result->func = NULL;
                result->e = blkCopy(target->e);
                result->parameters =blkCopy(target->parameters);
                result->functionBody = blkCopy(target->functionBody);
                break;
            }
        case ValidNum: result->num = target->num;break;
        case ValidFloat: result->flnum = target->flnum;break;
        case ErrCode: result->err = malloc(strlen(target->err)+1); strcpy(result->err, target->err);break;
        case ValidIdentifier: result->id = malloc(strlen(target->id)+1); strcpy(result->id, target->id);break;
        case ValidSExpression:
        case ValidQExpression:
            result->count = target->count;
            result->block = malloc(sizeof(customType*) * result->count);
            for(int i=0;i<result->count;i++){
                result->block[i]=blkCopy(target->block[i]);
            }
            break;
    }
    return result;
}

//enviorment helper functions
env* newEnv(void){
    env* e = malloc(sizeof(env));
    e->parent=NULL;
    e->count=0;
    e->ids=NULL;
    e->values=NULL;    
    return e;
}

env* delEnv(env* e){
    for(int i=0;i<e->count;i++){
        free(e->ids[i]);
        blockDel(e->values[i]);
    }
    free(e->ids);
    free(e->values);
    free(e);
}

customType* envGet(env* e, customType* x){
    for(int i=0;i<e->count;i++){ // loop over all items in the enviorment
        if(!strcmp(e->ids[i],x->id)){ // if the desired identifier is located
            return blkCopy(e->values[i]); // return a copy of it (instead of return a pointer because that pointer would be referencing something inside the enviorment and we don't want that)
        }
    }
    if(e->parent){
        return envGet(e->parent,x); //if a parent of this enviorment exists then search for the value there otherwise we know that the target is not present in the current scope
    }else{
        return typeErr("404 error %s not found", x->id); // return err if not found
    }
}

void envAdd(env* e, customType* x, customType* y){
    for(int i=0;i<e->count;i++){ //loop over all items, if identifier found overwite its vale
        if(!strcmp(e->ids[i],x->id)){
            blockDel(e->values[i]);
            e->values[i] = blkCopy(y);
        }
    }
    //otherwise realloc memory and add the new identifier to the enviorment
    e->count++;
    e->values = realloc(e->values, sizeof(customType*) * e->count);
    e->ids = realloc(e->ids, sizeof(char*) * e->count);

    e->values[e->count-1] = lval_copy(y);
    e->ids[e->count-1] = malloc(strlen(x->id)+1);
    strcpy(e->ids[e->count-1], x->id);

}
//a loop may or may not be more efficient; im leaving it like this for now
void globalEnvAdd(env* e, customType* name, customType* value){
    if(e->parent){
        globalEnvAdd(e->parent,name,value);
    }else{
        envAdd(e,name,value);
    }
}

env* envCopy(env* e){
    env* result = malloc(sizeof(env));
    result->parent = e->parent;
    result->count = e->count;
    result->ids = malloc(sizeof(char*)*result->count);
    result->values = malloc(sizeof(customType*)*result->count);
    for(int i=0;i<e->count;i++){
        result->ids[i] = malloc(strlen(e->ids[i])+1);
        strcpy(result->ids[i],e->ids[i]);
        result->values[i] = blkCopy(e->values[i]);
    }
    return result;
}
