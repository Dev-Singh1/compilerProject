#include <stdio.h>
#include "mpc.h"

static char input[2048]

long recursiveEvalHelper(mpc_ast_t* tree){
    //base case:
    if(strstr(tree->tag, "number")){ // if the token tag contains 'number' in its derivation, then we know it has no further derviations in the grammer and can simply return it as a terminal
        return atoi(tree->contents);
    }
    else{
        operator=tree->children[1]->contents; // we select the second child as the operator becasue the first is always '('
        long ans = recursiceEvalHelper(tree->children[2]); //recursive call
        int i = 3;// start at 3 because 0,1,2 have already been checked
        while(strstr(tree->children[1]->tag, "expr")){
            ans = arithmaticHelper(ans, op, eval(tree->childern[i]))
            i++;
        }// call a helper function and use a lookup table to evalute the arithmatic expressions
        return ans;
    }
    return -1;
}

long arithmaticHelper(long firstArgument, char operator, long secondArgument){
    if(strcmp(operator,"*")){
        return firstArgument * secondArgument;

    }else{
        if(strcmp(operator,"/")){
            return firstArgument / secondArgument;

        }else{
            if(strcmp(operator,"+")){
                return firstArgument + secondArgument;

            }else{
                if(strcmp(operator,"-")){
                    return firstArgument - secondArgument;

                }
            }

        }

    }
    return 0;
           
}

int main(int argc, char** argv){
    /* Create Some Parsers */
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lispy    = mpc_new("program");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                     \
            number   : /-?[0-9]+/ ;                           \
            operator : '+' | '-' | '*' | '/' ;                \
            expr     : <number> | '(' <operator> <expr>+ ')' ;\
            program    : /^/ <operator> <expr>+ /$/ ;         \
        ",
        Number, Operator, Expr, Lispy);

    while(true){
        printf(">");

        fget(input, 2048, stdin);
        
        mpc_result_t ast;
        if(mpc_parse("<stdin>",input,Program,&ast)){
            // if successfull, print the result of evaluating the ast
            long output = recursiveEvalHelper(ast.output);
            printf("output: %li\n",output);
            mpc_ast_delete(ast.output);
        }else{
            mpc_ast_print(ast.error);
            mpc_ast_delete(ast.error);
        }
    }
    /* Undefine and Delete our Parsers */
    mpc_cleanup(4,Number,Operator,Expr,Program);
    return 0
}