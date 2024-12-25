#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"
#include "types.c"
#include "SandQexpr.c"
#include "readAST.c"
#include "printResult.c"
#include "eval.c"

static char input[2048]


int main(int argc, char** argv){
    /* Create Some Parsers */
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Float   = mpc_new("float");
    mpc_parser_t* Identifier = mpc_new("identifier");
    mpc_parser_t* Sexpr     = mpc_new("sexpr");
    //Q-expression = quotes expreesions; this allows us to escapce certain string which will be return/printed as-is wihtout evalutation
    mpc_parser_t* Qexpr     = mpc_new("qexpr");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Program    = mpc_new("program");

    /* Define them with the following Language */
    //the definition of identifier in this context includes, not just arithmatic identifier, but funciton calls, conditionals, loids, etc. as well
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                     \
            number   : /-?[0-9]+/;                           \
            float    : /-?[0-9]+.?[0-9]+/ \
            identifier : '+' | '-' | '*' | '/' | '%%' | \"list\" | \"head\" | \"tail\"                \
           | \"join\" | \"eval\" | \"cons\" | \"len\" | \"init\";                \
            sexpr   : '(' <expr>* ')' \
            qexpr   : '{' <expr>* '}' \
            expr     : <number> | <float> | '<sexpr>' ;\
            program    : /^/ <identifier> <expr>+ /$/ ;         \
        ",
        Number, Float, Identifier, Sexpr, Qexpr, Expr, Program);

    while(true){
        printf(">");

        fget(input, 2048, stdin);
        
        mpc_result_t ast;
        if(mpc_parse("<stdin>",input,Program,&ast)){
            // if successfull, print the result of evaluating the ast
            customType output = recursiveEvalHelper(ast.output);
            extendedPrintf(output);
            mpc_ast_delete(ast.output);
        }else{
            mpc_ast_print(ast.error);
            mpc_ast_delete(ast.error);
        }
    }
    /* Undefine and Delete our Parsers */
    mpc_cleanup(7,Number,FLoat,Identifier,Sexpr,Qexpr,Expr,Program);
    return 0
}