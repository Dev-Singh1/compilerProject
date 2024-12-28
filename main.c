#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"
#include "types.c"
#include "readAST.c"
#include "printResult.c"
#include "eval.c"

//apparently, according to official documentation, readline and addhistory are function that dont exist on windows 32 bit so i have to define an edgecase in which I make my own if I am compling on/for a 32 bit windows system
#ifdef _WIN32

static char buffer[2048];

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
#include <editline/history.h>
#endif


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
    builtinFunctionAddHelper(e, "cons", cons);
    builtinFunctionAddHelper(e, "init", init);
    builtinFunctionAddHelper(e, "len", len);
    builtinFunctionAddHelper(e, "def", def);
    builtinFunctionAddHelper(e, "/", div);
    builtinFunctionAddHelper(e, "*", mul);
    builtinFunctionAddHelper(e, "+", add);
    builtinFunctionAddHelper(e, "-", sub);
    builtinFunctionAddHelper(e, "%%", mod);
    builtinFunctionAddHelper(e, "\\", lambda); //using backslash to represent lambda
    builtinFunctionAddHelper(e, "static", global);
    builtinFunctionAddHelper(e, "let", let);
}

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
            identifier : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;                \
            sexpr   : '(' <expr>* ')' \
            qexpr   : '{' <expr>* '}' \
            expr     : <number> | <float> | '<sexpr>' ;\
            program    : /^/ <identifier> <expr>+ /$/ ;         \
        ",
        Number, Float, Identifier, Sexpr, Qexpr, Expr, Program);
    
    env* e = newEnv();
    builtinFunctionAdd(e);

    while(true){
        char* input = readline(">");
        add_history(input);

        mpc_result_t ast;
        if(mpc_parse("<stdin>",input,Program,&ast)){
            // if successfull, print the result of evaluating the ast
            customType* output = recursiveHelper(e,readAll(ast.output));
            extendedPrintf(output);
            blockDel(output)
            mpc_ast_delete(ast.output);
        }else{
            mpc_ast_print(ast.error);
            mpc_ast_delete(ast.error);
        }
        free(input);
    }
    delEnv(e);
    /* Undefine and Delete our Parsers */
    mpc_cleanup(7,Number,FLoat,Identifier,Sexpr,Qexpr,Expr,Program);
    return 0
}