#include "types.c"
#include <stdlib.h>
#include "mpc.h"

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
    double x = strtof(tree->contents,NULL,10);
    return errno != ERANGE ? typeFloat(x) : typeErr("invalid number");
}

customType* readId(mpc_ast_t* tree){
    if (!strcmp("list", tree->contents)){return typeIdentifier(tree->contents);} 
    if (!strcmp("head", tree->contents)){return typeIdentifier(tree->contents);} 
    if (!strcmp("tail", tree->contents)){return typeIdentifier(tree->contents);} 
    if (!strcmp("join", tree->contents)){return typeIdentifier(tree->contents);} 
    if (!strcmp("eval", tree->contents)){return typeIdentifier(tree->contents);} 
    if (!strcmp("cons", tree->contents)){return typeIdentifier(tree->contents);} 
    if (!strcmp("len ", tree->contents)){return typeIdentifier(tree->contents);} 
    if (!strcmp("init", tree->contents)){return typeIdentifier(tree->contents);} 
    if (strstr("+-/*%%", tree->contents)){return typeIdentifier(tree->contents);}
    return typeErr("invalid identifier/command");
}

customType* readAll(mpc_ast_t* tree){
    //if identifier or number or float simply return the conversion to that type
    if(strstr(t->tag, "number")){return readNum(tree);}
    if(strstr(t->tag, "float")){return readFloat(tree);}
    if(strstr(t->tag, "identifier")){return readId(tree);}

    //if root of AST (>) or S expression, create an empty list
    customType* list = NULL;
    if(!strcmp(t->tag,">") | strstr(t->tag,"sexpr")){list = sExprCons();}
    if(!strcmp(tree->tag,"qexpr")){list = qExprCons();}
    //then fill that list
    for(int i=0;i<tree->children_num;i++){
        if(!strcmp(tree->childern[i],"regex") | !strcmp(tree->childern[i],"(") | !strcmp(tree->childern[i],")" | !strcmp(tree->childern[i],"{") | !strcmp(tree->childern[i],"}") ){continue;} //certain tokens are not syntatically meaningful and should be ignored
        list = readHelper(list, readAll(tree->children[i])) // helper fucntion that helps fill the list (add elements to the lsit)
    }
    
}
customType* concatinate(customType* l1, customType* l2){// takes 2 pointers, one for the list to which the data is added and the other for the data itself
    l1->count++;
    l1->block = realloc(l1->block, sizeof(customType*) * l1->count); //basic heap memory resizing

    // in lisp, car would return l1 while cdr would return l2; this explaination is how my mind understands using a dynamic array for the internal list represention
    return l1;
}