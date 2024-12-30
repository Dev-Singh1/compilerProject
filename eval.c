#include "types.c"
#include <stdio.h>
#include <stdlib.h>
#include "readAST.c"

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
  blockDel(node);
  return value;
}



customType* lookup(env* e, customType* arg, char* func){
    if (!strcmp("list", func)) { return list(e,arg); }
    if (!strcmp("head", func)) { return head(e,arg); }
    if (!strcmp("tail", func)) { return tail(e,arg); }
    if (!strcmp("join", func)) { return join(e,arg); }
    if (!strcmp("eval", func)) { return evalQexpr(e,arg); }
    if (!strcmp("cons", func)) { return cons(e,arg->block[0], arg->block[1]); }
    if (!strcmp("len", func)) { return len(e,arg); }
    if (!strcmp("init", func)) { return init(e,arg); }
    if (!strcmp("def", func)) { return def(e,arg); }
    if (!strcmp("printCurrentEnv", func)) { return printCurrentEnv(e); }
    if (strstr("+-/*%%", func)) { return arithmaticHelper(e,arg, func); }
    //exit functionality
    if(!strcmp("exit",func)){printf("BYE :)"); exit(0);}
    blockDel(arg);
    return typeErr("Unknown Function!");
}

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
   customType* result = callFunction(e, f, root);
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
customType* head(env* e, customType* lst){ //equivalent to car
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
customType* evalQexpr(env* e, customType* lst){
    if(lst->count!=1){blockDel(lst);return typeErr("Too many arguments passed to eval!");}
    if(lst->type!=ValidQExpression){blockDel(lst);return typeErr("eval can only iderate on QExpressions");}

    //take the first (and only) argument change its type to sexpr then call our evaluation functions on it
    customType* result=popAndDel(lst,0);
    result->type=ValidSExpression;
    return sexprEvalHelper(e,result);
}
customType* join(env* e, customType* lst){
    for(int i=0;i<lst->count;i++){
        if(lst->block[i]->type!=ValidQExpression){blockDel(lst);return typeErr("join can only iderate on QExpressions");}
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
    customType* result;
    result->type = ValidQExpression;

    //x is the first element of result
    result->block[0]=x;

    //join all elements of y to result and return it
    return joinHelper(e,result,y);
}
int len(env* e, customType* lst){
    return lst->count;
}
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
customType* add(env* e, customType* a) {
  return arithmaticHelper(e, a, "+");
}

customType* sub(env* e, customType* a) {
  return arithmaticHelper(e, a, "-");
}

customType* mul(env* e, customType* a) {
  return arithmaticHelper(e, a, "*");
}

customType* div(env* e, customType* a) {
  return arithmaticHelper(e, a, "/");
}
customType* mod(env* e, customType* a) {
  return arithmaticHelper(e, a, "%%");
}
//comparison operator builtins
customType* geq(env e* customType* args){
    if(args->count!=2){blockDel(args);return typeErr("incorrect number of arguments");}
    customType* first = args->block[0];
    customType* second = args->block[1];
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:int x = first->num >= second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: int x = first->num >= second->flnum; blockDel(first); blockDel(second); blockDel(args);  return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: int x = first->flnum >= second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: int x = first->flnum >= second->flnum; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        default: blockDel(first); blockDel(second); blockDel(args); reutrn typeErr("incorrect arguements");
    }
}
customType* gt(env e* customType* args){
    if(args->count!=2){blockDel(args);return typeErr("incorrect number of arguments");}
    customType* first = args->block[0];
    customType* second = args->block[1];
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:int x = first->num > second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: int x = first->num > second->flnum; blockDel(first); blockDel(second); blockDel(args);  return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: int x = first->flnum > second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: int x = first->flnum > second->flnum; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        default: blockDel(first); blockDel(second); blockDel(args); reutrn typeErr("incorrect arguements");
    }
}
customType* leq(env e* customType* args){
    if(args->count!=2){blockDel(args);return typeErr("incorrect number of arguments");}
    customType* first = args->block[0];
    customType* second = args->block[1];
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:int x = first->num <= second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: int x = first->num <= second->flnum; blockDel(first); blockDel(second); blockDel(args);  return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: int x = first->flnum <= second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: int x = first->flnum <= second->flnum; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        default: blockDel(first); blockDel(second); blockDel(args); reutrn typeErr("incorrect arguements");
    }
}
customType* lt(env e* customType* args){
    if(args->count!=2){blockDel(args);return typeErr("incorrect number of arguments");}
    customType* first = args->block[0];
    customType* second = args->block[1];
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:int x = first->num < second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: int x = first->num < second->flnum; blockDel(first); blockDel(second); blockDel(args);  return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: int x = first->flnum < second->num; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                case ValidFloat: int x = first->flnum < second->flnum; blockDel(first); blockDel(second); blockDel(args); return typeNum(x);
                default: blockDel(first); blockDel(second); blockDel(args); return typeErr("incorrect argumetns");
            }break;
        default: blockDel(first); blockDel(second); blockDel(args); reutrn typeErr("incorrect arguements");
    }
}

customType* eq(env* e, customType* first, customType* second){
    switch(first->type){
        case ValidNum:
            switch(second->type){
                case ValidNum:int x = first->num == second->num; blockDel(first); blockDel(second);  return typeNum(x);
                case ValidFloat: int x = first->num == second->flnum; blockDel(first); blockDel(second);  return typeNum(x);
                default: blockDel(first); blockDel(second); return typeNum(0);
            }break;
        case ValidFloat:
            switch(second->type){
                case ValidNum: int x = first->flnum == second->num; blockDel(first); blockDel(second); return typeNum(x);
                case ValidFloat: int x = first->flnum == second->flnum; blockDel(first); blockDel(second); return typeNum(x);
                default: blockDel(first); blockDel(second); return typeNum(0);
            }break;
        case ErrCode:
            switch(second->type){
                case ErrCode: int x = (strcmp(first->err,second->err)==0); blockDel(first); blockDel(second); return typeNum(x); 
                default: blockDel(first); blockDel(second); return typeNum(0);
            }break;
        case ValidIdentifier:
            switch(second->type){
                case ValidIdentifier: int x = (strcmp(first->id,second->id)==0); blockDel(first); blockDel(second); return typeNum(x); 
                default: blockDel(first); blockDel(second); return typeErr("inccrrect argumetns");
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
                if(!(eq(first->block[i],second->block[i]))){blockDel(first); blockDel(second); return typeNum(0);}
            }
            blockDel(first); blockDel(second); return typeNum(1);
        break;
            
        default: blockDel(first); blockDel(second); return typeNum(0);
    }
}
customType* neq(env* e, customType* args){
    if(args->count!=2){return typeErr("incorrect number of arguments");}
    customType* answer = eq(e,args-block[0],args->block[1]);
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
    switch(args->block[0]->num){
        case 0: blockDel(args); return typeNum(1);
        case 1: blockDel(args); return typeNum(0);
    }

}

customType* andFunction(env* e, customType* args){
    if(args->count!=2){
        return typeErr("too few arguments");
    }
    else if(args->block[0]->type!=ValidNum&&args->block[1]->type!=ValidNum){
        return typeErr("valid conditional required");
    }
    int first=args->block[0]->num;
    int second=arg->block[1]->num;
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
    int first=args->block[0]->num;
    int second=arg->block[1]->num;
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
        envAdd(e,vars->block[i]->id,arg->block[i+1]);
    }

    blockDel(arg);
    return blockCons();
}

//builtin function for printing all names in an enviorment
customType* printCurrentEnv(env* e){
    for(int i=0;i<e->count;i++){
        printf("%s : %d",e->ids[i],e->values[i]);
    }
    return blockCons();
}

customType* global(env* e, customType* arg){return envAddHelper(e,arg,"static");}
customType* let(env* e, customType* arg){return envAddHelper(e,arg,"let");}

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
    while(arg->count){
        if(f->parameters->count == 0){blockDel(arg); return typeErr("too many arguments passed");}
        
        customType* name = pop(f->parameters,0);
        if(!strcmp(name->id,'&')){ //if & is found, enter edge case of varible arguments
            if(f->parameters->count!=1){
                blockDel(arg); return typeErr("incorrect function format");
            }
            customType* optionalArgument = pop(f->parameters,0);
            envAdd(f->e,optionalArgument,list(e,args));
            blockDel(name); blockDel(optionalArgument);
            break;
        }
        customType* value = pop(a->parameters,0);
        
        envAdd(f->e,name,value);
        blockDel(name);blockDel(value);
    }
    blockDel(args);

    if(f->parameters->count > 0 && !strcmp(f->parameters->block[0]->id,'&')){
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
    switch(arg->block[0]->num){
        case 1: ans=recursiveHelper(e,pop(arg,1)); blockDel(args); return ans;
        case 0: ans=recursiveHelper(e,pop(arg,2)); blockDel(args); return ans;;
    }
}