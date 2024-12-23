#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

static char input[2048]

//this struct allows 'error' to become a valid type in of itself for error handling purposes
typedef struct extendedType{
    int type; //expected and/or valid type of operation; this tells us weather we are going to access the num feild or the err field of this struct
    long num; //value
    double flNum; //float value
    char* err; //error code
    char* op //operator
    int count; //analogous to argc (count of tokens?)
    struct extendedType** block; //analogus to argv (vector of tokens?)
}extendedType;

//enums for code readablity
enum {ValidNum, ValidFloat, ErrCode}; //possible types

enum {DivByZero, InvalidOperator, InvalidNumber};//possible errors

//funcitons to convert primitive types to our extended types
extendedType typeNum(long x){
    extendedType result;
    result.type = ValidNum;
    result.num = x;
    return result;
}

extendedType typeFloat(double x){
    extendedType result;
    result.type = ValidFloat;
    result.flnum = x;
    return result;
}

extendedType typeErr(int x){
    extendedType result;
    result.type = ErrCode;
    result.err = x;
    return result;
}

//we must now modify printf/create our own print function to handle the extended type
void extendedPrintf(extendedType s){
    switch (s.type){
        case ValidNum: printf("%li", s.num); break;
        case ValidFloat: printf("%d",s); break;
        case ErrCode:
            switch (s.err){
                case DivByZero: printf("Error: Division By Zero!");break;
                case InvalidOperator: printf("Error: Invalid Operator!");break;
                case InvalidNumber: printf("Error: Invalid Number!");break;
            }break;
    }
}

extendedType recursiveEvalHelper(mpc_ast_t* tree){
    //base case (for int):
    if(strstr(tree->tag, "number")){ // if the token tag contains 'number' in its derivation, then we know it has no further derviations in the grammer and can simply return it as a terminal
        errno = 0;
        long value = strtol(tree->contents, NULL, 10);
        //if the creation of the value variable did not create valid value, the ternay operator in the return statement will output the enumerated error
        return errno != ERANGE ? typeNum(value) : typeErr(InvalidNumber);
    }
    //base case (for float):
    if(strstr(tree->tag, "float")){ //if the token tag contains 'float'
        errno=0;
        double value = strtof(tree->contnets, NULL);
        //if the creation of the value variable did not create valid value, the ternay operator in the return statement will output the enumerated error
        return errno != ERANGE ? typeFloat(value) : typeErr(InvalidNumber)
    }
    //all other cases
    char* operator=tree->children[1]->contents; // we select the second child as the operator becasue the first is always '('
    extendedType ans = recursiceEvalHelper(tree->children[2]); //recursive call
    int i = 3;// start at 3 because 0,1,2 have already been checked
    while(strstr(tree->children[1]->tag, "expr")){
        ans = arithmaticHelper(ans, op, eval(tree->childern[i]))
        i++;
    }// call a helper function and use a lookup table to evalute the arithmatic expressions
    return ans;
    
}

long arithmaticHelper(extendedType firstArgument, char operator, extendedType secondArgument){
    //base case of encountering an error
    if(firstArgument.type == ErrCode){
        return firstArgument;
    }
    if(firstArgument.type == ErrCode){
      return secondArgument;
    }
    if(!strcmp(operator,"*")){
        return firstArgument * secondArgument;

    }else{
        if(!strcmp(operator,"/")){
            //ternary operator that handles divide by zero errors
            return secondArgument==0 ? typeErr(DivByZero) : typeNum(firstArgument / secondArgument);

        }else{
            if(!strcmp(operator,"+")){
                return firstArgument + secondArgument;

            }else{
                if(!strcmp(operator,"-")){
                    return firstArgument - secondArgument;

                }else{
                    if(!strcmp(operator,"%%")){
                        return secondArgument==0 ? typeErr(DivByZero) : typeNum(firstArgument % secondArgument);
                    }
                }
            }

        }

    }
    return typeErr(InvalidOperator);
           
}

int main(int argc, char** argv){
    /* Create Some Parsers */
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Float   = mpc_new("float");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Program    = mpc_new("program");

    /* Define them with the following Language */
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
