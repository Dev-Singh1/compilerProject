#include "types.c"


//we must now create our own print function(s) to handle the custom type
void exprPrint(customType* s, char iden, char close){
    putchar(iden);
    for(int i = 0; i<s->count;i++){
        extendedPrintf(s->block[i]);
        //skip spacer if at last element; for output readabliity
        if(i!=s->count-1){
            putchar(' ');
        }
    }
    putchar(close);
}
void extendedPrintf(customType* s){
    switch (s->type){
        case ValidNum: printf("%li", s->num); break;
        case ValidFloat: printf("%d",s->flnum); break;
        case ErrCode: printf("Error: %s",s->err); break;
        case ValidIdentifier: printf("%s",s->id); break;
        case ValidFunction: printf('<function>'); break;
        case ValidSExpression: exprPrint(s,'(',')'); break;
        case ValidQExpression: exprPrint(s,'{','}'); break;
    }
}
void extendedPrintln(customType* s){extendedPrintf(s); putchar('\n');}
