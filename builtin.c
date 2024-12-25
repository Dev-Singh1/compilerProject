#include "types.c"

//builtin functions
customType* head(customType* lst){ //equivalent to car
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
customType* tail(customType* lst){ //equivalent to cdr
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
customType* list(customType* lst){ // takes the given list and converts it to a qexpr
    lst->type = ValidQExpression;
    return lst;

}
customType* evalQexpr(customType* lst){
    if(lst->count!=1){blockDel(lst);return typeErr("Too many arguments passed to eval!");}
    if(lst->type!=ValidQExpression){blockDel(lst);return typeErr("eval can only iderate on QExpressions");}

    //take the first (and only) argument change its type to sexpr then call our evaluation functions on it
    customType* result=popAndDel(lst,0);
    result->type=ValidSExpression;
    return sexprEvalHelper(result);
}
customType* join(customType* lst){
    for(int i=0;i<lst->count;i++){
        if(lst->block[i]->type!=ValidQExpression){blockDel(lst);return typeErr("join can only iderate on QExpressions");}
    }
    customType* result = pop(lst,0); //pop the first element

    //then join the rest of the elements to it
    while(lst->count){
        result=joinHelper(result, pop(lst,0));
    }
    blockDel(lst);
    return result;
}
customType* joinHelper(customType* x, customType* y){
    //for each block in y, add it to x then delete y and return x
    while(y->count){
        x = concatinate(x,pop(y,0));
    }
    blockDel(y);
    return x;
}
customType* cons(customType* x, customType* y){
    //ensure that y is Qexpr
    if(y->type!=ValidQExpression){blockDel(x); blockDel(y); return typeErr("cons joins a value to a qexpr, you must pass a qexpr");}
    
    //create a new qexpr
    customType* result;
    result->type = ValidQExpression;

    //x is the first element of result
    result->block[0]=x;

    //join all elements of y to result and return it
    return joinHelper(result,y);
}
int len(customType* lst){
    return lst->count;
}
customType* init(customType* lst){
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
