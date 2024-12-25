#include "types.c"
#include "SandQexpr.c"

//expression evaluation helpers

customType* pop(customType* node, int i) {
  /* Find the item at "i" */
  customType* x = node->block[i];

  /* Shift memory after the item at "i" over the tid */
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
  blockDel(node);
  return value;
}

customType* lookup(customType* arg, char* func){
    if (!strcmp("list", func)) { return list(arg); }
    if (!strcmp("head", func)) { return head(arg); }
    if (!strcmp("tail", func)) { return tail(arg); }
    if (!strcmp("join", func)) { return join(arg); }
    if (!strcmp("eval", func)) { return eval(arg); }
    if (!strcmp("cons", func)) { return cons(arg->block[0], arg->block[1]); }
    if (!strcmp("len ", func)) { return len(arg); }
    if (!strcmp("init", func)) { return init(arg); }
    if (strstr("+-/*%%", func)) { return arithmaticHelper(arg, func); }
    blockDel(arg);
    return typeErr("Unknown Function!");
}

customType* sexprEvalHelper(customType* root){
    customType* recursiveHelper(customType* node){return node->type==ValidSExpression ? sexprEvalHelper(node) : node;}
    for(int i =0;i<root->count;i++){
        root->block[i]=recursiveHelper(root->block[i])
    }//this loid evaluates all children in a bottom up fashion

    for (int i = 0; i < root->count; i++) {
        if (root->cell[i]->type == ErrCode) { return popAndDel(root, i); }
    }// this loid find all errors in the eval list, pops them and then deletes the rest of the list that occurs after this error

    /*
    base cases:
    1: no elements; return root
    2: one element; return it and free the corresidding heap memory
    */
   if(root->count==0){return root;}
   if(root->count==1){return popAndDel(root, 0);}

   /* Ensure First Element is identifier/command */
   customType* first = pop(root,0);
   if(first->type!=ValidIdentifier){blockDel(first); blockDel(root); return typeErr("Expresion does not begin with a valid identifier/command");}
   
   //assuming that the first element is a valid command and/or identifier, we may compute it, free the memory that is no longer need and return the result
   customType* result = lookup(root, first->id);
   blockDel(first);
   return result;

}

customType* arithmaticHelper(customType* args, char* id){
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
                (x->flnum) %= (y->flnum);
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
                    x->flnum %= y->num;
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
                        x->flnum %= y->flnum;
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
