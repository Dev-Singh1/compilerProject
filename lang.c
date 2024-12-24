#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

static char input[2048]

//this struct introduces symbol expression; i.e we are starting to represent all data as a list (at least internally)
typedef struct extendedType{
    int type; //expected and/or valid type of operation; this tells us weather we are going to access the num feild or the err field of this struct
    long num; //value
    double flNum; //float value
    char* err; //error code
    char* op //operator
    int count; //analogous to argc (count of tokens? (idk what else to call them))
    struct extendedType** block; //analogus to argv (vector of tokens? (idk what else to call them))
}extendedType;

//enums for code readablity
enum {ValidNum, ValidFloat, ErrCode, ValidOperator, ValidSExpression}; //possible types

enum {DivByZero, InvalidOperator, InvalidNumber};//possible errors

//funcitons to convert primitive types to our extended types (list cells and constituent tokens)
extendedType* typeNum(long x){
    extendedType* result = malloc(sizeof(extendedType));
    result->type = ValidNum;
    result->num = x;
    return result;
}

extendedType* typeFloat(double x){
    extendedType* result = malloc(sizeof(extendedType));
    result->type = ValidFloat;
    result->flnum = x;
    return result;
}

extendedType* typeErr(char* x){
    extendedType* result = malloc(sizeof(extendedType));
    result->type = ErrCode;
    result->err = malloc(strlen(x)+1);
    strpy(result->err, x)
    return result;
}

extendedType* typeOperator(char* x){
    extendedType* result = malloc(sizeof(extendedType));
    result->type = ValidOperator;
    result->op = malloc(strlen(x)+1);
    strpy(result->op, x)
    return result;
}

//block constructor
extendedType* blockCons(void){ //creates an empty list block and returns a pointer to it
    extendedType* result = malloc(sizeof(extendedType));
    result->type = ValidSExpression;
    result->count = 0;
    result->block = NULL;
    return result;
}

//block destructor
void blockDel(extendedType* x){ //deletes list block and returns nothing
    switch (x->type){
        case ValidNum: break; //no need to call free when it is an int
        case ValidFloat: break; //similarly, ther is no need to deallocate heap memory when it is a float
        case ErrCode: free(x->err);break;
        case ValidOperator: free(x->op);break;
        //the above 2 cases find and free the apropriate fields of the object pointed to by the given pointer
        
        //the next case uses a loop to free an entire list (all blocks reachable from the given pointer are freed)
        case ValidSExpression:
            for(int i=0;i<x->count;i++){
                blockDel(v->block[i]);
            }
            //also freeing the pointers
            free(v->block);
            break;
    }
    //free the struct itself
    free(x);
}

//now that we have interanlly changed the representation of data as a dynamic list of symbols (operators and numbers and floats)
//we must also add new functions that read the AST and convert it into this format

/*
If the given node is tagged as a number or operator or float, then we use our constructors to return an extendedType* directly for those types. 
If the given node is the root, or an sexpresion, then we create an empty S-Expression extendedType* and slowly add each valid sub-expression contained in the tree.
*/

extendedType* readNum(mpc_ast_t* tree){
    errno=0;
    long x = strtol(t->contents,NULL,10);
    return errno != ERANGE ? typeNum(x) : typeErr("invalid number");
}

extendedType* readFloat(mpc_ast_t* tree){
    errno=0;
    double x = strtof(t->contents,NULL,10);
    return errno != ERANGE ? typeFloat(x) : typeErr("invalid number");
}

extendedType* readAll(mpc_ast_t* tree){
    //if operator or number or float simply return the conversion to that type
    if(strstr(t->tag, "number")){return readNum(tree);}
    if(strstr(t->tag, "float")){return readFloat(tree);}
    
    //if root of AST (>) or S expression, create an empty list
    extendedType* list = NULL;
    if(!strcmp(t->tag,">") | strstr(t->tag,"sexpr")){list = blockCons();}

    //then fill that list
    for(int i=0;i<tree->children_num;i++){
        if(!strcmp(tree->childern[i],"regex") | !strcmp(tree->childern[i],"(") | !strcmp(tree->childern[i],")") ){continue;} //certain tokens are not syntatically meaningful and should be ignored
        list = readHelper(list, readAll(tree->children[i])) // helper fucntion that helps fill the list (add elements to the lsit)
    }
    
}
extendedType* readHelper(extendedType* l1, extendedType* l2){// takes 2 pointers, one for the list to which the data is added and the other for the data itself
    l1->count++;
    l1->block = realloc(l1->block, sizeof(extendedType*) * l1->count); //basic heap memory resizing

    // in lisp, car would return l1 while cdr would return l2; this explaination is how my mind understands using a dynamic array for the internal list represention
    return l1;
}

//we must now modify printf/create our own print function(s) to handle the extended type
void exprPrint(extendedType* s, char open, char close){
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
void extendedPrintf(extendedType* s){
    switch (s->type){
        case ValidNum: printf("%li", s->num); break;
        case ValidFloat: printf("%d",s->flnum); break;
        case ErrCode: printf("Error: %s",s->err); break;
        case ValidOperator: printf("%s",s->op); break;
        case ValidSExpression: exprPrint(s,'(',')'); break;
    }
}
void extendedPrintln(extendedType* s){extendedPrintf(s); putchar('\n');}

//expression evaluation helpers, pop and popAndDel

extendedType* pop(extendedType* node, int i) {
  /* Find the item at "i" */
  extendedType* x = node->block[i];

  /* Shift memory after the item at "i" over the top */
  memmove(&node->block[i], &node->block[i+1],
    sizeof(extendedType*) * (node->count-i-1));

  /* Decrease the count of items in the list */
  node->count--;

  /* Reallocate the memory used */
  node->block = realloc(v->cell, sizeof(extendedType*) * v->count);
  return x;
}

extendedType* popAndDel(extendedType* node, int i) {
  extendedType* value = pop(node, i);
  blockDel(node);
  return value;
}

//the eval helpers have been modified to operate on S-expression rather than prefix arithmatic
extendedType* sexprEvalHelper(extendedType* root){
    extendedType* recursiveHelper(extendedType* node){return node->type==ValidSExpression ? sexprEvalHelper(node) : node;}
    for(int i =0;i<root->count;i++){
        root->block[i]=recursiveHelper(root->block[i])
    }//this loop evaluates all children in a bottom up fashion

    for (int i = 0; i < root->count; i++) {
        if (root->cell[i]->type == ErrCode) { return popAndDel(root, i); }
    }// this loop find all errors in the eval list, pops them and then deletes the rest of the list that occurs after this error

    /*
    base cases:
    1: no elements; return root
    2: one element; return it and free the corresopding heap memory
    */
   if(root->count==0){return root;}
   if(root->count==1){return popAndDel(root, 0)}

   /* Ensure First Element is operator/command */
   extendedType* first = pop(root,0);
   if(first->type!=ValidOperator){blockDel(first); blockDel(root); return typeErr("Expresion does not begin with a valid operator/command");}
   
   //assuming that the first element is a valid command and/or operator, we may compute it, free the memory that is no longer need and return the result
   extendedType* result = computationHelper(root, first->op);
   blockDel(first);
   return result;

}

long computationHelper(extendedType* args, char* operator){
    //ensure all arguments are ints and floats (as only numerical operations have been defined thus far)
    for(int i=0; i<args->count;i++){
        if(args->block[i]->type != ValidNum && args->block[i]->type != ValidFloat){
            blockDel(args);
            return typeErr("Not A Number Error");
        }
    }

    //pop the first element into a local variable for evalutation
    extendedType* x = pop(args, 0);

    //if there are no arguments left and a substract operator was supplied, treat it as a unary negation (multiply by negative one)
    if (!(strcmp(op, "-")) && args->count == 0) {
        if(x->type==ValidFloat){
            x->flnum=-x->flnum;
        }
        if(x->type==ValidNum){
            x->num=-x->num;
        }
    }

    //while there are still elements remaining keep a running tally of sorts which results from succesively applying the operator to the arguments
    while(args->count>0){
        extendedType* y= pop(args, 0);
        if(x->type==ValidFloat && y->type==ValidFloat){
            if (!strcmp(op, "+")) { x->flnum += y->flnum; }
            if (!strcmp(op, "-")) { x->flnum -= y->flnum; }
            if (!strcmp(op, "*")) { x->flnum *= y->flnum; }
            if (!strcmp(op, "/")) {
                if (y->flnum == 0) {
                    blockDel(x); blockDel(y);
                    x = typeErr("Division By Zero!"); break;
                }
                x->flnum /= y->flnum;
            }
            if (!strcmp(op, "%%")) {
                if (y->flnum == 0) {
                    blockDel(x); blockDel(y);
                    x = typeErr("Division By Zero!"); break;
                }
                x->flnum %= y->flnum;
            }
        }else{
            if(x->type==ValidFloat && y->type==ValidNum){
                if (!strcmp(op, "+")) { x->flnum += y->num; }
                if (!strcmp(op, "-")) { x->flnum -= y->num; }
                if (!strcmp(op, "*")) { x->flnum *= y->num; }
                if (!strcmp(op, "/")) {
                    if (y->num == 0) {
                        blockDel(x); blockDel(y);
                        x = typeErr("Division By Zero!"); break;
                    }
                    x->flnum /= y->num;
                }
                if (!strcmp(op, "%%")) {
                    if (y->num == 0) {
                        blockDel(x); blockDel(y);
                        x = typeErr("Division By Zero!"); break;
                    }
                    x->flnum %= y->num;
                }
            }else{
                if(x->type==ValidNum && y->type==ValidFloat){
                    x=typeFloat((double)x->num);
                    if (!strcmp(op, "+")) { x->flnum += y->flnum; }
                    if (!strcmp(op, "-")) { x->flnum -= y->flnum; }
                    if (!strcmp(op, "*")) { x->flnum *= y->flnum; }
                    if (!strcmp(op, "/")) {
                        if (y->flnum == 0) {
                            blockDel(x); blockDel(y);
                            x = typeErr("Division By Zero!"); break;
                        }
                        x->flnum /= y->flnum;
                    }
                    if (!strcmp(op, "%%")) {
                        if (y->flnum == 0) {
                            blockDel(x); blockDel(y);
                            x = typeErr("Division By Zero!"); break;
                        }
                        x->flnum %= y->flnum;
                    }

                }else{
                    if(x->type==ValidNum && y->type==ValidNum){
                        if (!strcmp(op, "+")) { x->num += y->num; }
                        if (!strcmp(op, "-")) { x->num -= y->num; }
                        if (!strcmp(op, "*")) { x->num *= y->num; }
                        if (!strcmp(op, "/")) {
                            if (y->num == 0) {
                                blockDel(x); blockDel(y);
                                x = typeErr("Division By Zero!"); break;
                            }
                            x->num /= y->num;
                        }
                        if (!strcmp(op, "%%")) {
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

    
           
}

int main(int argc, char** argv){
    /* Create Some Parsers */
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Float   = mpc_new("float");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Program    = mpc_new("program");

    /* Define them with the following Language */
    //the definition of operator in this context includes, not just arithmatic operator, but funciton calls, conditionals, loops, etc. as well
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                     \
            number   : /-?[0-9]+/;                           \
            float    : /-?[0-9]+.?[0-9]+/ \
            operator : '+' | '-' | '*' | '/' | '%%';                \
            sexpr   : '(' <expr>* ')'
            expr     : <number> | <float> | '<sexpr>' ;\
            program    : /^/ <operator> <expr>+ /$/ ;         \
        ",
        Number, Float, Operator, Sexpr, Expr, Program);

    while(true){
        printf(">");

        fget(input, 2048, stdin);
        
        mpc_result_t ast;
        if(mpc_parse("<stdin>",input,Program,&ast)){
            // if successfull, print the result of evaluating the ast
            extendedType output = recursiveEvalHelper(ast.output);
            extendedPrintf(output);
            mpc_ast_delete(ast.output);
        }else{
            mpc_ast_print(ast.error);
            mpc_ast_delete(ast.error);
        }
    }
    /* Undefine and Delete our Parsers */
    mpc_cleanup(6,Number,FLoat,Operator,Sexpr,Expr,Program);
    return 0
}
