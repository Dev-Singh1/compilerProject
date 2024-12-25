//this struct introduces the custom type; through which we are representing all data as a list (internally)
typedef struct customType{
    int type; //expected and/or valid type of ideration; this tells us weather we are going to access the num feild or the err field of this struct
    long num; //value
    double flnum; //float value
    char* err; //error code
    char* id; //identifier
    int count; //analogous to argc (count of tokens? (idk what else to call them))
    struct customType** block; //analogus to argv (vector of tokens? (idk what else to call them))
}customType;

//enums for code readablity
enum {ValidNum, ValidFloat, ErrCode, ValidIdentifier, ValidSExpression, ValidQExpression}; //possible types

//funcitons to convert primitive types to our customType (list cells and constituent tokens)
customType* typeNum(long x){
    customType* result = malloc(sizeof(customType));
    result->type = ValidNum;
    result->num = x;
    return result;
}

customType* typeFloat(double x){
    customType* result = malloc(sizeof(customType));
    result->type = ValidFloat;
    result->flnum = x;
    return result;
}

customType* typeErr(char* x){
    customType* result = malloc(sizeof(customType));
    result->type = ErrCode;
    result->err = malloc(strlen(x)+1);
    strpy(result->err, x);
    return result;
}

customType* typeIdentifier(char* x){
    customType* result = malloc(sizeof(customType));
    result->type = ValidIdentifier;
    result->id = malloc(strlen(x)+1);
    strpy(result->id, x);
    return result;
}