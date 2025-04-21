#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "mpc.h"
//apparently, according to official documentation, readline and addhistory are function that dont exist on windows 32 bit so i have to define an edgecase in which I make my own if I am compiling on/for a 32 bit windows system
#ifdef _WIN32

static char buffer[2048];

//readline and add_history function definition for WIN32 systems
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <stdbool.h>
#include <editline/history.h>
#endif

/*forward declarations*/

struct customType;
struct env;
typedef struct customType customType;
typedef struct env env;
env* newEnv(void);
customType* def(env* e, customType* arg);
void delEnv(env* e);
customType* blkCopy(customType* target);
env* envCopy(env* e);
customType* sexprEvalHelper(env* e, customType* root);
customType* callFunction(env* e, customType* f, customType* args);
customType* joinHelper(env* e, customType* x, customType* y);
customType* concatinate(customType* l1, customType* l2);
customType* readAll(mpc_ast_t* tree);
customType* myDivide(env* e, customType* a);
void envAdd(env* e, customType* x, customType* y);
customType* defStruct(env* e, customType* arg);
customType* envAddHelper(env* e, customType* arg, char* f);
void extendedPrintf(customType* s);






//enums for code readablity
enum {ValidNum, ValidFloat, ErrCode, ValidIdentifier, ValidSExpression, ValidQExpression, ValidFunction, ValidString, UserDefinedType}; //possible types

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
    char* str; //string

    //function types
    funcPtr func; //function pointer (if null we use the fields below to identify and execute user defined funtions)
    env* e;// pointer to funciton enviorment
    customType* parameters; // pointer to list of function params
    customType* functionBody; //pointer to expression that represents the function body

    //struct entries
    customType* entrys; 
    
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
    customType* result = malloc(sizeof(customType)); //allocate memory for the object
    //set its type and value
    result->type = ValidNum;
    result->num = x;
    return result; //return the created object
}

customType* typeFloat(double x){
    customType* result = malloc(sizeof(customType));//allocate memory for the object
    //set its type and value
    result->type = ValidFloat;
    result->flnum = x;
    return result; //return the created object
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
    customType* result = malloc(sizeof(customType));//allocate memory for the object
    //set its type and value
    result->type = ValidIdentifier;
    result->id = malloc(strlen(x)+1);
    strcpy(result->id, x);
    return result;//return the created object
}

customType* typeFunction(funcPtr f){
    customType* result = malloc(sizeof(customType));//allocate memory for the object
    //set its type and value
    result->type = ValidFunction;
    result->func = f;
    return result;//return the created object
}
//constructor for user defined functions
customType* typeLambda(customType* params, customType* body){
    customType* result = malloc(sizeof(customType));//allocate memory for the object
    //set its type and value(s)
    result->type = ValidFunction;
    result->e = newEnv();
    result->parameters = params;
    result->functionBody = body;
    result->func = NULL;
    return result;//return the created object

} 

//constructor for user defined data types
customType* typeUser(char* name, customType* values){
    customType* result = malloc(sizeof(customType));
    result->type=UserDefinedType;
    result->e=newEnv();
    result->id=name;
    result->entrys=def(result->e,values);
    return result;
}

customType* typeStr(char* str){
    customType* result = malloc(sizeof(customType*));//allocate memory for the object
    //set its type and value
    result->type=ValidString;
    //using malloc and strcpy to set the value of result->str to be the desired stirng, because it would otherwise be a pointer to that string
    result->str = malloc(strlen(str)+1);
    strcpy(result->str,str);
    return result;//return the created object

}

//s and q expr helper functions

//sExpr constructor
customType* blockCons(void){ //creates an empty list block and returns a pointer to it
    customType* result = malloc(sizeof(customType));//allocate memory for the object
    //set its type and value(s)
    result->type = ValidSExpression;
    result->count = 0;
    result->block = NULL;
    return result;//return the created object
}

//qExpr consturctor
customType* qExprCons(void){ //creates an empty list block and returns a pointer to it
    customType* result = malloc(sizeof(customType));//allocate memory for the object
    //set its type and value(s)
    result->type = ValidQExpression;
    result->count = 0;
    result->block = NULL;
    return result;//return the created object
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
        case ValidString: free(x->str); break;
        case ValidIdentifier: free(x->id);break;
        //the above 2 cases find and free the apridriate fields of the object pointed to by the given pointer
        
	//WIP
	//case UserDefinedType: clean(x);break;

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
    customType* result = malloc(sizeof(customType)); //allocate memory for the copy
    result->type=target->type; //equate types
    //copy appropriate value(s) based on type
    switch (target->type){
        case ValidFunction:
            if(target->func!=NULL){
                result->func = target->func;
                break;
            }else{
                result->func = NULL;
                result->e = envCopy(target->e);
                result->parameters =blkCopy(target->parameters);
                result->functionBody = blkCopy(target->functionBody);
                break;
            }
        case ValidNum: result->num = target->num;break;
        case ValidString: result->str=malloc(strlen(target->str)+1); strcpy(result->str,target->str);break;
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
    return result; //return the copy
}

//enviorment helper functions
env* newEnv(void){
    env* e = malloc(sizeof(env)); //allocate memory for the eviorment object
    //set its value(s)
    e->parent=NULL;//pointer to parnet env
    e->count=0;//count of number of identifier stored in the enviornment
    e->ids=NULL;//pointer to list of names
    e->values=NULL; //pointer to list of values   
    return e; //return the enviorment object
}

void delEnv(env* e){
    //in the given enviornment, loop over all variables and identifiers stored in the enviornmnet and free them
    for(int i=0;i<e->count;i++){
        free(e->ids[i]);
        blockDel(e->values[i]);
    }
    //free the pointers as well
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

    e->values[e->count-1] = blkCopy(y);
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
//function to create a copy of the provided enviorment
env* envCopy(env* e){
    env* result = malloc(sizeof(env)); //allocate space for the copy
    
    result->parent = e->parent; //equate parent pointers
    result->count = e->count; //equate counts
    //allocate space for copies of all values stored in this enviornment
    result->ids = malloc(sizeof(char*)*result->count); 
    result->values = malloc(sizeof(customType*)*result->count);
    //loop over all values in the enviornment and use the value copying functuion to make a copy of the value and add it the the env copy
    for(int i=0;i<e->count;i++){
        result->ids[i] = malloc(strlen(e->ids[i])+1);
        strcpy(result->ids[i],e->ids[i]);
        result->values[i] = blkCopy(e->values[i]);
    }
    return result;//return env copy
}


//expression evaluation helpers

customType* pop(customType* node, int i) {
  /* Find the item at "i" */
  customType* x = node->block[i];

  /* Shift memory after the item at "i" over the top */
  memmove(&node->block[i], &node->block[i+1],
    sizeof(customType*) * (node->count-i-1));

  /* Decrease the count of items in the list */
  node->count--;

  /* Reallocate the memory used */
  node->block = realloc(x->block, sizeof(customType*) * x->count);
  return x;
}

customType* popAndDel(customType* node, int i) {
  customType* value = pop(node, i);
  blockDel(node);//after poping the desired element from the given list delete the rest of the list
  return value;
}



//helper for recursive evaluation of s expr lists
customType* recursiveHelper(env* e, customType* node){
    if(node->type == ValidIdentifier){
        customType* x = envGet(e,node);
        blockDel(node);
        return x;
    }
    return node->type==ValidSExpression ? sexprEvalHelper(e,node) : node;
}

customType* sexprEvalHelper(env* e, customType* root){
    
    for(int i =0;i<root->count;i++){
        root->block[i]=recursiveHelper(e, root->block[i]);
    }//this loop evaluates all children in a bottom up fashion

    for (int i = 0; i < root->count; i++) {
        if (root->block[i]->type == ErrCode) { return popAndDel(root, i); }
    }// this loop find all errors in the eval list, pops them and then deletes the rest of the list that occurs after this error

    /*
    base cases:
    1: no elements; return root
    2: one element; return it and free the corresidding heap memory
    */
   if(root->count==0){return root;}
   if(root->count==1){return popAndDel(root, 0);}

   /* Ensure First Element is identifier/command */
   customType* first = pop(root,0);
   if(first->type!=ValidFunction){blockDel(first); blockDel(root); return typeErr("Expresion does not begin with a valid identifier/command");}
   
   //assuming that the first element is a valid command and/or identifier, we may compute it, free the memory that is no longer need and return the result
   customType* result = callFunction(e, first, root);
   blockDel(first);
   return result;

}

customType* arithmaticHelper(env* e, customType* args, char* id){
    //ensure all arguments are ints and/or floats 
    for(int i=0; i<args->count;i++){
        if(args->block[i]->type != ValidNum && args->block[i]->type != ValidFloat){
            blockDel(args);
            return typeErr("Not A Number Error");
        }
    }

    //pop the first element into a local variable for evalutation
    customType* x = pop(args, 0);

    //if there are no arguments left and a substract identifier was supplied, treat it as a unary negation (multiply by negative one)
    if (!(strcmp(id, "-")) && args->count == 0) {
        if(x->type==ValidFloat){
            x->flnum=-x->flnum;
        }
        if(x->type==ValidNum){
            x->num=-x->num;
        }
    }

    //while there are still elements remaining keep a running tally of sorts which results from succesively applying the identifier to the arguments
    while(args->count>0){
        customType* y= pop(args, 0);
        if(x->type==ValidFloat && y->type==ValidFloat){
            if (!strcmp(id, "+")) { x->flnum += y->flnum; }
            if (!strcmp(id, "-")) { x->flnum -= y->flnum; }
            if (!strcmp(id, "*")) { x->flnum *= y->flnum; }
            if (!strcmp(id, "/")) {
                if (y->flnum == 0) {
                    blockDel(x); blockDel(y);
                    x = typeErr("Division By Zero!"); break;
                }
                x->flnum /= y->flnum;
            }
            if (!strcmp(id, "%%")) {
                if (y->flnum == 0) {
                    blockDel(x); blockDel(y);
                    x = typeErr("Division By Zero!"); break;
                }
                x->flnum = fmod(x->flnum,y->flnum);
            }
        }else{
            if(x->type==ValidFloat && y->type==ValidNum){
                if (!strcmp(id, "+")) { x->flnum += y->num; }
                if (!strcmp(id, "-")) { x->flnum -= y->num; }
                if (!strcmp(id, "*")) { x->flnum *= y->num; }
                if (!strcmp(id, "/")) {
                    if (y->num == 0) {
                        blockDel(x); blockDel(y);
                        x = typeErr("Division By Zero!"); break;
                    }
                    x->flnum /= y->num;
                }
                if (!strcmp(id, "%%")) {
                    if (y->num == 0) {
                        blockDel(x); blockDel(y);
                        x = typeErr("Division By Zero!"); break;
                    }
                    x->flnum = fmod(x->flnum,(double)y->num);
                }
            }else{
                if(x->type==ValidNum && y->type==ValidFloat){
                    x=typeFloat((double)x->num);
                    if (!strcmp(id, "+")) { x->flnum += y->flnum; }
                    if (!strcmp(id, "-")) { x->flnum -= y->flnum; }
                    if (!strcmp(id, "*")) { x->flnum *= y->flnum; }
                    if (!strcmp(id, "/")) {
                        if (y->flnum == 0) {
                            blockDel(x); blockDel(y);
                            x = typeErr("Division By Zero!"); break;
                        }
                        x->flnum /= y->flnum;
                    }
                    if (!strcmp(id, "%%")) {
                        if (y->flnum == 0) {
                            blockDel(x); blockDel(y);
                            x = typeErr("Division By Zero!"); break;
                        }
                        x->flnum = fmod(x->flnum,y->flnum);
                    }

                }else{
                    if(x->type==ValidNum && y->type==ValidNum){
                        if (!strcmp(id, "+")) { x->num += y->num; }
                        if (!strcmp(id, "-")) { x->num -= y->num; }
                        if (!strcmp(id, "*")) { x->num *= y->num; }
                        if (!strcmp(id, "/")) {
                            if (y->num == 0) {
                                blockDel(x); blockDel(y);
                                x = typeErr("Division By Zero!"); break;
                            }
                            x->num /= y->num;
                        }
                        if (!strcmp(id, "%%")) {
                            if (y->num == 0) {
                                blockDel(x); blockDel(y);
                                x = typeErr("Division By Zero!"); break;
                            }
                            x->num %= y->num;
                        }
                    }
                }
            }
        }
    }

    return x;
           
}


//builtin functions
void show(env* e, customType* lst){
   lst->str[strlen(lst->str)-1]='\0';
   char* result = malloc(strlen(lst->str+1)+1);
   strcpy(result,lst->str+1);
   result = mpcf_unescape(result);
   printf("%s",result);
   free(result);
   blockDel(lst); 
}

customType* head(env* e, customType* lst){ //equivalent to car
    if(lst->type==ValidString){char x=lst->str[0];blockDel(lst);return typeStr(&x);}
    //base cases that causes errors
    if(lst->count!=1){blockDel(lst); return typeErr("Too many arguments given to head function");}
    if(lst->type != ValidQExpression){blockDel(lst); return typeErr("head function can only iderate on Qexpressions!");}
    if(lst->block[0]->count==0){blockDel(lst); return typeErr("Empty list passed to head!");}

    //take the first (and only) argument
    customType* result = popAndDel(lst,0);

    //delete all but the first element from that argument, then return it
    while(result->count>1){blockDel(pop(result,1));}
    return result;
}
customType* tail(env* e, customType* lst){ //equivalent to cdr
    if(lst->type==ValidString){char* x=malloc(sizeof(lst->str));strncpy(x,lst->str+1,strlen(lst->str)-1);x[strlen(lst->str)-1]='\0';blockDel(lst);return typeStr(x);}
    //base cases that causes errors
    if(lst->count!=1){blockDel(lst);return typeErr("Too many arguments passed to tail!");}
    if(lst->type!=ValidQExpression){blockDel(lst);return typeErr("tail can only iderate on QExpressions");}
    if(lst->block[0]->count==0){blockDel(lst);return typeErr("Empty list passed to tail!");}

    //take the first (and only) argument
    customType* result = popAndDel(lst,0);

    //delete the first element from that argument, then return it
    blockDel(pop(result,0));
    return result;
}
customType* list(env* e, customType* lst){ // takes the given list and converts it to a qexpr
    lst->type = ValidQExpression;
    return lst;

}
//evaluation helper for all expressions that are not s exprs
customType* evalQexpr(env* e, customType* lst){
    if(lst->count!=1){blockDel(lst);return typeErr("Too many arguments passed to eval!");}
    if(lst->type!=ValidQExpression){blockDel(lst);return typeErr("eval can only iderate on QExpressions");}

    //take the first (and only) argument change its type to sexpr then call our evaluation functions on it
    customType* result=popAndDel(lst,0);
    result->type=ValidSExpression;
    return sexprEvalHelper(e,result);
}
customType* join(env* e, customType* lst){
    if(lst->block[0]->type==ValidQExpression){
        for(int i=0;i<lst->count;i++){
            if(lst->block[i]->type!=ValidQExpression){
                
                blockDel(lst);
                return typeErr("join can only iderate on QExpressions/Strings");
            }
        }
    }else{
        for(int i=0;i<lst->count;i++){
            if(lst->block[i]->type!=ValidString){
                
                blockDel(lst);
                return typeErr("join can only operate on QExpressions/String");
            }
        }
    }
    customType* result = pop(lst,0); //pop the first element

    //then join the rest of the elements to it
    while(lst->count){
        result=joinHelper(e,result, pop(lst,0));
    }
    blockDel(lst);
    return result;
}
customType* joinHelper(env* e, customType* x, customType* y){
    //for each block in y, add it to x then delete y and return x
    while(y->count){
        x = concatinate(x,pop(y,0));
    }
    blockDel(y);
    return x;
}
customType* cons(env* e, customType* x, customType* y){
    //ensure that y is Qexpr
    if(y->type!=ValidQExpression){blockDel(x); blockDel(y); return typeErr("cons joins a value to a qexpr, you must pass a qexpr");}
    
    //create a new qexpr
    customType* result=qExprCons();
    result->type = ValidQExpression;

    //x is the first element of result
    result->block[0]=x;

    //join all elements of y to result and return it
    return joinHelper(e,result,y);
}
int len(env* e, customType* lst){
    return lst->count;
}
//WIP
//customType* read(customType* s){
    //if(s->type!=ValidString){blockDel(s);return typeErr("excpected string");}
    //mpc_result_t ast;
    //mpc_parse("input",s->str,Program, &ast);
    //return readAll(ast.output);
//}
customType* init(env* e, customType* lst){
    //base cases that causes errors
    if(lst->count!=1){blockDel(lst);return typeErr("Too many arguments passed to init!");}
    if(lst->type!=ValidQExpression){blockDel(lst);return typeErr("init can only iderate on QExpressions");}
    if(lst->block[0]->count==0){blockDel(lst);return typeErr("Empty list passed to init!");}

    //take the first (and only) argument
    customType* result = popAndDel(lst,0);

    //delete the last element from that argument, then return it
    blockDel(pop(result,result->count-1));
    return result;
}
//arithmatic wrappers
customType* add(env* e, customType* a) {
  return arithmaticHelper(e, a, "+");
}

customType* sub(env* e, customType* a) {
  return arithmaticHelper(e, a, "-");
}

customType* mul(env* e, customType* a) {
  return arithmaticHelper(e, a, "*");
}

customType* myDivide(env* e, customType* a) {
  return arithmaticHelper(e, a, "/");
}
customType* mod(env* e, customType* a) {
  return arithmaticHelper(e, a, "%%");
}
//comparison operator builtins
customType* geq(env *e, customType* args){
    if(args->count!=2){blockDel(args);return typeErr("incorrect number of arguments");}
    customType* first = args->block[0];
    customType* second = args->block[1];
    //switch case to match types and return appropriate values
    int x;
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:x = first->num >= second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: x = first->num >= second->flnum; blockDel(first); blockDel(second); blockDel(args);  return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: x = first->flnum >= second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: x = first->flnum >= second->flnum; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect arguements");
    }
}
customType* gt(env *e, customType* args){
    if(args->count!=2){blockDel(args);return typeErr("incorrect number of arguments");}
    customType* first = args->block[0];
    customType* second = args->block[1];
    //switch case to match types and return appropriate values
    int x;
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:x = first->num > second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: x = first->num > second->flnum; blockDel(first); blockDel(second); blockDel(args);  return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: x = first->flnum > second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: x = first->flnum > second->flnum; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect arguements");
    }
}
customType* leq(env *e, customType* args){
    if(args->count!=2){blockDel(args);return typeErr("incorrect number of arguments");}
    customType* first = args->block[0];
    customType* second = args->block[1];
    //switch case to match types and return appropriate values
    int x;
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:x = first->num <= second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: x = first->num <= second->flnum; blockDel(first); blockDel(second); blockDel(args);  return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: x = first->flnum <= second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: x = first->flnum <= second->flnum; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect arguements");
    }
}
customType* lt(env *e, customType* args){
    if(args->count!=2){blockDel(args);return typeErr("incorrect number of arguments");}
    customType* first = args->block[0];
    customType* second = args->block[1];
    //switch case to match types and return appropriate values
    int x;
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:x = first->num < second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: x = first->num < second->flnum; blockDel(first); blockDel(second); blockDel(args);  return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: x = first->flnum < second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: x = first->flnum < second->flnum; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect arguements");
    }
}

customType* eq(env* e, customType* first, customType* second){
    //switch case to match types and return appropriate values
    int x;
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:x = first->num == second->num; blockDel(first); blockDel(second);  return typeNum(x);
                case ValidFloat: x = first->num == second->flnum; blockDel(first); blockDel(second);  return typeNum(x);
                default: blockDel(first); blockDel(second); return typeNum(0);
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: x = first->flnum == second->num; blockDel(first); blockDel(second); return typeNum(x);
                case ValidFloat: x = first->flnum == second->flnum; blockDel(first); blockDel(second); return typeNum(x);
                default: blockDel(first); blockDel(second); return typeNum(0);
            }break;
        case ErrCode:
            switch(second->type){
                case ErrCode: x = (strcmp(first->err,second->err)==0); blockDel(first); blockDel(second); return typeNum(x); 
                default: blockDel(first); blockDel(second); return typeNum(0);
            }break;
        case ValidIdentifier:
            switch(second->type){
                case ValidIdentifier: x = (strcmp(first->id,second->id)==0); blockDel(first); blockDel(second); return typeNum(x); 
                default: blockDel(first); blockDel(second); return typeErr("inccrrect argumetns");
            }break;
        case ValidString:
            switch(second->type){
                case ValidString: x = (strcmp(first->str,second->str)==0); blockDel(first); blockDel(second); return typeNum(x); 
                default: blockDel(first); blockDel(second); return typeNum(0);
            }break;
        case ValidFunction:
            switch(second->type){
                case ValidFunction: 
                    if(first->func || second->func){
                        blockDel(first); blockDel(second); return typeNum(first->func==second->func);

                    }else{
                        blockDel(first); blockDel(second); return typeNum(eq(e, first->parameters, second->parameters)&&eq(e,first->functionBody,second->functionBody));
                    }
                default: blockDel(first); blockDel(second); return typeNum(0);
            }break;
        // compare lists
        case ValidQExpression:
        case ValidSExpression:
            if(first->count != second->count){blockDel(first); blockDel(second); return typeNum(0);}
            for (int i = 0; i < first->count; i++){
                if(!(eq(e,first->block[i],second->block[i]))){blockDel(first); blockDel(second); return typeNum(0);}
            }
            blockDel(first); blockDel(second); return typeNum(1);
        break;
            
        default: blockDel(first); blockDel(second); return typeNum(0);
    }
}
customType* neq(env* e, customType* args){
    if(args->count!=2){return typeErr("incorrect number of arguments");}
    customType* answer = eq(e,args->block[0],args->block[1]);
    blockDel(args);
    switch(answer->type){
        case ErrCode: return answer;
        default: return typeNum(!(answer->num));
    }
}// neq is the same as not(eq)/!eq

customType* notFunction(env* e, customType* args ){
    if(args->count!=1){
        return typeErr("too few arguments");
    }
    else if(args->block[0]->type!=ValidNum){
        return typeErr("valid conditional required");
    }
    //switch case to match types and return appropriate values
    switch(args->block[0]->num){
        case 0: blockDel(args); return typeNum(1);
        case 1: blockDel(args); return typeNum(0);
    }
    return blockCons();

}

customType* andFunction(env* e, customType* args){
    if(args->count!=2){
        return typeErr("too few arguments");
    }
    else if(args->block[0]->type!=ValidNum&&args->block[1]->type!=ValidNum){
        return typeErr("valid conditional required");
    }
    //operates on bools (represented internally by numbers 0 and 1)
    int first=args->block[0]->num;
    int second=args->block[1]->num;
    int answer = first && second;
    blockDel(args);
    return typeNum(answer);
}

customType* orFunction(env* e, customType* args){
    if(args->count!=2){
        return typeErr("too few arguments");
    }
    else if(args->block[0]->type!=ValidNum&&args->block[1]->type!=ValidNum){
        return typeErr("valid conditional required");
    }
    //operates on bools (represented internally by numbers 0 and 1)
    int first=args->block[0]->num;
    int second=args->block[1]->num;
    int answer = first || second;
    blockDel(args);
    return typeNum(answer);

}






//builtin function for defining and declaring
customType* def(env* e, customType* arg){
    if(arg->type!=ValidQExpression){
        return typeErr("def passed incorrect type!");
    }
    // the first argument passed to def is a list of names to define
    customType* vars = arg->block[0];

    for(int i=0;i<vars->count;i++){
        if(vars->block[i]->type!=ValidIdentifier){
            return typeErr("can only assign values to identifiers!");
        }
    }

    //ensure that number of provided names matches the number of provided values
    if(vars->count!=arg->count-1){
        return typeErr("incorrect number of identifies and values");
    }

    //add all name value pairs to the enviornment
    for(int i=0;i<vars->count;i++){
        envAdd(e,vars->block[i],arg->block[i+1]);
    }

    blockDel(arg);
    return blockCons();
}
//WIP
//customType* defStruct(env* e, customType* arg){
    //if(arg->type!=ValidQExpression){
        //return typeErr("defStruct passed incorrect type!");
    //}
    // the first argument passed to defStruct is the name of the struct to define
    //char* name = arg->block[0];
    // the second argument passed to defStruct is the list of identifiers to define
    //customType* vars = arg->block[1];

    //for(int i=0;i<vars->count;i++){
        //if(vars->block[i]->type!=ValidIdentifier){
            //return typeErr("can only assign values to identifiers!");
        //}
    //}

    //ensure that number of provided names matches the number of provided values
    //if(vars->count!=arg->count-1){
        //return typeErr("incorrect number of identifies and values");
    //}
    //customType* result = typeStr("");
    //customType* temp = blockCons();
    //add all name value pairs to the result list;
    //for(int i=0;i<vars->count;i++){
        //temp=cons(e,typeStr(vars->block[i]->id),arg->block[i+1]);
        //result=cons(e,result,temp);

    //}

    //blockDel(arg);
    //blockDel(temp);
    //return list(e,result);
//}
//END_WIP
//builtin function for printing all names in an enviorment
customType* printCurrentEnv(env* e){
    for(int i=0;i<e->count;i++){
	switch(e->values[i]->type){
	    case ValidNum:
        	printf("%s : %ld",e->ids[i],e->values[i]->num);
		break;
            case ValidFloat:
        	printf("%s : %f",e->ids[i],e->values[i]->flnum);
		break;

	}
    }
    return blockCons();
}

customType* global(env* e, customType* arg){return envAddHelper(e,arg,"static");}
customType* let(env* e, customType* arg){return envAddHelper(e,arg,"let");}
//helper that differentiates between adding to current local scope or global scope and call the appropraite env add functions
customType* envAddHelper(env* e, customType* arg, char* f){

    if(arg->block[0]->type!=ValidQExpression){
        blockDel(arg); return typeErr("incorrect type passed");
    }
    customType* idens = arg->block[0];
    for(int i = 0; i < idens->count;i++){
        if(idens->block[i]->type!=ValidIdentifier){
            blockDel(arg); return typeErr("cannot bind non identifier");
        }
    }
    for(int i=0;i<idens->count;i++){
        if(!strcmp(f,"static")){globalEnvAdd(e,idens->block[i],arg->block[i+1]);}
        if(!strcmp(f,"let")){envAdd(e,idens->block[i],arg->block[i+1]);}
    }
    blockDel(arg);
    return blockCons();
}



//builtin lambda function for funtional programming compatibility (anonymous functions)
//arg is a list of 2 arguments: the params and the body
customType* lambda(env* e, customType* arg){
    //error checking cases
    if(arg->count!=2){return typeErr("incorrect number of arguments");}
    if(arg->block[0]->type!=ValidQExpression){return typeErr("incorrect type of arguments");}
    if(arg->block[0]->type!=ValidQExpression){return typeErr("incorrect type of arguments");}

    //ensure that the paraments are identifiers (we do not allow primitive type literals to constitute the type signature of the lambda function)
    for(int i=0;i<arg->block[0]->count;i++){
        if(arg->block[0]->block[i]->type!=ValidQExpression){
            return typeErr("cannot bind non identifier");
        }
    }

    //use lamda consturctor to return a pointer to the user defined function
    customType* params = pop(arg,0);
    customType* body = pop(arg,0);
    blockDel(arg);
    return typeLambda(params,body);
    
}

//evaluate calls to user defined functions
customType* callFunction(env* e, customType* f, customType* args){
    if(f->func){return f->func(e,args);} //if builtin, simply call it

    //otherwise check the argument count and add parameters to the function enviorment and set the current enviorment as the parent enviorment of the funciton enviorment
    while(args->count){
        if(f->parameters->count == 0){blockDel(args); return typeErr("too many arguments passed");}
        
        customType* name = pop(f->parameters,0);
        if(!strcmp(name->id,"'&'")){ //if & is found, enter edge case of varible arguments
            if(f->parameters->count!=1){
                blockDel(args); return typeErr("incorrect function format");
            }
            customType* optionalArgument = pop(f->parameters,0);
            envAdd(f->e,optionalArgument,list(e,args));
            blockDel(name); blockDel(optionalArgument);
            break;
        }
        customType* value = pop(args->parameters,0);
        
        envAdd(f->e,name,value);
        blockDel(name);blockDel(value);
    }
    blockDel(args);

    if(f->parameters->count > 0 && !strcmp(f->parameters->block[0]->id,"'&'")){
        if(f->parameters->count != 2){
            return typeErr("incorrect function format");
        }
        blockDel(pop(f->parameters,0));
        customType* name = pop(f->parameters,0);
        customType* value = qExprCons();
        envAdd(f->e,name,value);
        blockDel(name);blockDel(value);
    }

    if(f->parameters->count == 0){
        f->e->parent=e;
        return evalQexpr(f->e,concatinate(blockCons(), blkCopy(f->functionBody)));

    }else{
        return blkCopy(f);
    }
    


}


//builtin if 
customType* ifFunction(env* e, customType* args){
    if(args->count!=3){
        return typeErr("too few arguments");
    }
    else if(args->block[0]->type!=ValidNum){
        return typeErr("valid conditional required");
    }
    else if(args->block[1]->type!=ValidQExpression || args->block[2]->type!=ValidQExpression){
        return typeErr("evaluable exprssions required");

    }
    customType* ans;
    switch(args->block[0]->num){
        case 1: ans=recursiveHelper(e,pop(args,1)); blockDel(args); return ans;
        case 0: ans=recursiveHelper(e,pop(args,2)); blockDel(args); return ans;;
    }
    return blockCons();
}
//builtin print for the language

customType* printBuiltin(env* e, customType* args){
    for(int i=0;i<args->count;i++){
        extendedPrintf(args->block[i]); putchar(' ');
    }
    putchar('\n');
    blockDel(args);

    return blockCons();
}

//builtin error command for the language
customType* errorBuiltin(env* e, customType* args){
    if(args->count!=1){blockDel(args);return typeErr("requires ONE string to return as an error");}
    if(args->block[0]->type!=ValidString){blockDel(args);return typeErr("requires one STRING to return as an error");}
    customType* error = typeErr(args->block[0]->str);
    blockDel(args);
    return error;
}



//functions that read the AST and convert it into this format

/*
If the given node is tagged as a number or identifier or float, then we use our constructors to return an customType* directly for those types. 
If the given node is the root, or an sexpresion, then we create an empty S-Expression customType* and slowly add each valid sub-expression contained in the tree.
*/

customType* readNum(mpc_ast_t* tree){
    errno=0;
    long x = strtol(tree->contents,NULL,10);
    return errno != ERANGE ? typeNum(x) : typeErr("invalid number");
}

customType* readFloat(mpc_ast_t* tree){
    errno=0;
    double x = strtof(tree->contents,NULL);
    return errno != ERANGE ? typeFloat(x) : typeErr("invalid number");
}

customType* readId(mpc_ast_t* tree){
    if (strcmp("list", tree->contents)||strcmp("head", tree->contents)||strcmp("tail", tree->contents)||strcmp("join", tree->contents)||strcmp("cons", tree->contents)||strcmp("init", tree->contents)||strcmp("len", tree->contents)||strcmp("def", tree->contents)||strcmp("static", tree->contents)||strcmp("let", tree->contents)||strcmp("if", tree->contents)||strcmp(">=", tree->contents)||strcmp(">", tree->contents)||strcmp("<=", tree->contents)||strcmp("<", tree->contents)||strcmp("join", tree->contents)||strcmp("==", tree->contents)||strcmp("!=", tree->contents)||strcmp("eval", tree->contents)||strcmp("cons", tree->contents)||strcmp("len", tree->contents) || strcmp("init", tree->contents) || !strstr("+-/*%%", tree->contents)){
        return typeErr("invalid identifier/command");
    }
    
    return typeIdentifier(tree->contents);
}

customType* readStr(mpc_ast_t* tree){
// Cut off the final quote character 
  tree->contents[strlen(tree->contents)-1] = '\0';
  //Copy the string missing out the first quote character 
  char* inputStr = malloc(strlen(tree->contents+1)+1);
  strcpy(inputStr, tree->contents+1);

  inputStr=mpcf_unescape(inputStr);
  customType* result = typeStr(inputStr);
  free(inputStr);
  return result;
}

customType* readAll(mpc_ast_t* tree){
    //if identifier or number or float simply return the conversion to that type
    if(strstr(tree->tag, "number")){return readNum(tree);}
    if(strstr(tree->tag, "float")){return readFloat(tree);}
    if(strstr(tree->tag,"string")){return readStr(tree);}
    if(strstr(tree->tag, "identifier")){return readId(tree);}
    

    //if root of AST (>) or S expression, create an empty list
    customType* list = NULL;
    if(!strcmp(tree->tag,">") || strstr(tree->tag,"sexpr")){list = blockCons();}
    if(!strcmp(tree->tag,"qexpr")){list = qExprCons();}
    //then fill that list
    for(int i=0;i<tree->children_num;i++){
        if(!strcmp(tree->children[i]->tag,"regex") || !strcmp(tree->children[i]->contents,"(") || !strcmp(tree->children[i]->contents,")") || !strcmp(tree->children[i]->contents,"{") || !strcmp(tree->children[i]->contents,"}") || strstr(tree->children[i]->tag,"comment") ){continue;} //certain tokens are not syntatically meaningful and should be ignored
        list = concatinate(list, readAll(tree->children[i])); // helper fucntion that helps fill the list (add elements to the lsit)
    }
    return blockCons();
    
}
customType* concatinate(customType* l1, customType* l2){// takes 2 pointers, one for the list to which the data is added and the other for the data itself
    l1->count++;
    l1->block = realloc(l1->block, sizeof(customType*) * l1->count); //basic heap memory resizing

    // in lisp, car would return l1 while cdr would return l2; this explaination is how my mind understands using a dynamic array for the internal list represention
    return l1;
}
//need to add further error handling for file wrappers
FILE* fopenwrapper(customType* f){
    return fopen(f->block[0]->str,f->block[1]->str);

}
int fclosewrapper(FILE* f){
    return fclose(f);

}
char* fgetswrapper(FILE* f){ 
    char buff[2048];
    return fgets(buff,sizeof(buff),f);
}
int fputswrapper(FILE* f,customType* data){
    return fputs(data->block[0]->str,f);

}

//we must now create our own print function(s) to handle the custom type
void exprPrint(customType* s, char open, char close){
    putchar(open);
    for(int i = 0; i<s->count;i++){
        extendedPrintf(s->block[i]);
        //skip spacer if at last element; for output readabliity
        if(i!=s->count-1){
            putchar(' ');
        }
    }
    putchar(close);
}
void stringHelper(customType* s){
    char* result= malloc(sizeof(s->str)+1);
    strcpy(result,s->str);
    result=mpcf_escape(result);
    printf("\"%s\"",result);
    free(result);
}
//void testStringHelper(customType* s){
    //char* result= malloc(sizeof(s->str)+1);
    //strcpy(result,s->str);
    //result=mpcf_escape(result);
    //return result;
//}
void extendedPrintf(customType* s){
    switch (s->type){
        case ValidNum: printf("%li", s->num); break;
        case ValidFloat: printf("%f",s->flnum); break;
        case ErrCode: printf("Error: %s",s->err); break;
        case ValidString: stringHelper(s); break;
        case ValidIdentifier: printf("%s",s->id); break;
        case ValidFunction:
            if(s->func !=NULL){
                printf("<Builtin Function>"); break;
            }else{
                printf("<User Defined Function>"); break;
            }
        case ValidSExpression: exprPrint(s,'(',')'); break;
        case ValidQExpression: exprPrint(s,'{','}'); break;
    }
}
//char* testPrintf(customType* s){
    //char* result = malloc(sizeof(float));
    //switch (s->type){
        //case ValidNum: return ("%li", s->num); break;
        //case ValidFloat: gcvt(s->flnum,5,result); return result; break;
        //case ErrCode: return ("%s",s->err); break;
        //case ValidString: testStringHelper(s); break;
        //case ValidIdentifier: return ("%s",s->id); break;
        //case ValidFunction:
            //if(s->func !=NULL){
                //return ("<Builtin Function>"); break;
            //}else{
                //return ("<User Defined Function>"); break;
            //}
        //case ValidSExpression: exprPrint(s,'(',')'); break;
        //case ValidQExpression: exprPrint(s,'{','}'); break;
    //}
    //return result;
//}
void extendedPrintln(customType* s){extendedPrintf(s); putchar('\n');}

//forward declaration of parser pointers
mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Program;

customType* load(env* e, customType* args){
    if(args->count!=1){blockDel(args);return typeErr("too many arguments");}
    if(args->block[0]->type!=ValidString){blockDel(args);return typeErr("need a string of the file name");}
    mpc_result_t result;
    if(mpc_parse_contents(args->block[0]->str,Program,&result)){
        //read contents
        customType* exprs = readAll(result.output);
        mpc_ast_delete(result.output);

        //loop over and eval each expression
        while(exprs->count){
            customType* x = evalQexpr(e,pop(exprs,0));
            //if error, print error
            if(x->type==ErrCode){extendedPrintln(x);}
            blockDel(x);
        }
        //delete the expr and args then return defualt
        blockDel(exprs);
        blockDel(args);
        return blockCons();

    }else{
        //Get Parse Error as String 
        char* err_msg = mpc_err_string(result.error);
        mpc_err_delete(result.error);

        //Create new error message using it 
        customType* err = typeErr("Could not load Library %s", err_msg);
        free(err_msg);
        blockDel(args);

        // Cleanup and return error 
        return err;
    }
}

//register builtins into the enviorment
void builtinFunctionAddHelper(env* e, char* name, funcPtr f){
    customType* x = typeIdentifier(name);
    customType* y = typeFunction(f);
    envAdd(e,x,y);
    blockDel(x);blockDel(y); 
}
void builtinFunctionAdd(env* e){
    builtinFunctionAddHelper(e, "list", list);
    builtinFunctionAddHelper(e, "head", head);
    builtinFunctionAddHelper(e, "tail", tail);
    builtinFunctionAddHelper(e, "eval", evalQexpr);
    builtinFunctionAddHelper(e, "join", join);
    //builtinFunctionAddHelper(e, "cons", cons);
    builtinFunctionAddHelper(e, "init", init);
    //builtinFunctionAddHelper(e, "len", len);
    builtinFunctionAddHelper(e, "def", def);
    builtinFunctionAddHelper(e, "/", myDivide);
    builtinFunctionAddHelper(e, "*", mul);
    builtinFunctionAddHelper(e, "+", add);
    builtinFunctionAddHelper(e, "-", sub);
    builtinFunctionAddHelper(e, "%%", mod);
    builtinFunctionAddHelper(e, "\\", lambda); //using backslash to represent lambda
    builtinFunctionAddHelper(e, "static", global);
    builtinFunctionAddHelper(e, "let", let);
    builtinFunctionAddHelper(e, "if", ifFunction);
    //builtinFunctionAddHelper(e, "==", eq);
    builtinFunctionAddHelper(e, "!=", neq);
    builtinFunctionAddHelper(e, ">=", geq);
    builtinFunctionAddHelper(e, ">", gt);
    builtinFunctionAddHelper(e, "<=", leq);
    builtinFunctionAddHelper(e, "<", lt);
    builtinFunctionAddHelper(e, "load", load);
    builtinFunctionAddHelper(e, "print", printBuiltin);
    builtinFunctionAddHelper(e, "error", errorBuiltin);
    //builtinFunctionAddHelper(e,"show",show);
    //builtinFunctionAddHelper(e,"read",read);
}

int main(int argc, char** argv){
    /* Create Some Parsers */
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* String   = mpc_new("string");
    mpc_parser_t* Float   = mpc_new("float");
    mpc_parser_t* Identifier = mpc_new("identifier");
    mpc_parser_t* Sexpr     = mpc_new("sexpr");
    //Q-expression = quotes expreesions; this allows us to escapce certain string which will be return/printed as-is wihtout evalutation
    mpc_parser_t* Qexpr     = mpc_new("qexpr");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Comment   = mpc_new("comment");
    mpc_parser_t* Program    = mpc_new("program");

    /* Define them with the following Language */
    //the definition of identifier in this context includes, not just arithmatic identifier, but funciton calls, conditionals, loops, etc. as well
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                    \
            number   : /-?[0-9]+/;                           \
            string   : /\\\"(\\\\\\\\.||[^\\\"])*\\\"/ ;               \
            float    : /-?[0-9]+\\.?[0-9]+/;                   \
            identifier : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;  \
            sexpr   : '(' <expr>* ')';                        \
            qexpr   : '{' <expr>* '}';                        \
            expr     : <number> | <float> | <sexpr> | <qexpr> | <comment> | <string> | <identifier> ;\
            comment  : /;[^\\r\\n]*/ ;                       \
            program    : /^/ <expr>* /$/ ;                   \
	",
        Number, String, Float, Identifier, Sexpr, Qexpr, Expr, Comment, Program);
    
    env* e = newEnv();
    builtinFunctionAdd(e);
    //if filename(s) supplied, loop overthem and evaluate
    if(argc>=2){
        for(int i=1;i<argc;i++){ //i starts at 1 to skip the name of this programm
            customType* argument = concatinate(blockCons(),typeStr(argv[i]));
            customType* x = load(e,argument);
            //if error, print error
            if(x->type==ErrCode){
                extendedPrintln(x);
            }
            blockDel(x);

        }
// if no file name provided, enter into repl 
    }else{
        while(true){
            char* input = readline(">");
            add_history(input);

            mpc_result_t ast;
            if(mpc_parse("<stdin>",input,Program,&ast)){
                // if successfull, print the result of evaluating the ast
                customType* output = recursiveHelper(e,readAll(ast.output));
                extendedPrintf(output);
                blockDel(output);
                mpc_ast_delete(ast.output);
            }else{
                printf("%s:\n", mpc_err_string(ast.error));
                mpc_err_delete(ast.error);
            }
            free(input);
        }
    }
    delEnv(e);
    /* Undefine and Delete our Parsers */
    mpc_cleanup(9,Number,String,Float,Identifier,Sexpr,Qexpr,Expr,Comment,Program);
    return 0;
}
