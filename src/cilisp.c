#include "cilisp.h"
#include "math.h"

#define RED             "\033[31m"
#define RESET_COLOR     "\033[0m"

// yyerror:
// Something went so wrong that the whole program should crash.
// You should basically never call this unless an allocation fails.
// (see the "yyerror("Memory allocation failed!")" calls and do the same.
// This is basically printf, but red, with "\nERROR: " prepended, "\n" appended,
// and an "exit(1);" at the end to crash the program.
// It's called "yyerror" instead of "error" so the parser will use it for errors too.
void yyerror(char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 255, format, args);

    printf(RED "\nERROR: %s\nExiting...\n" RESET_COLOR, buffer);
    fflush(stdout);

    va_end (args);
    exit(1);
}

// warning:
// Something went mildly wrong (on the user-input level, probably)
// Let the user know what happened and what you're doing about it.
// Then, move on. No big deal, they can enter more inputs. ¯\_(ツ)_/¯
// You should use this pretty often:
//      too many arguments, let them know and ignore the extra
//      too few arguments, let them know and return NAN
//      invalid arguments, let them know and return NAN
//      many more uses to be added as we progress...
// This is basically printf, but red, and with "\nWARNING: " prepended and "\n" appended.
void warning(char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 255, format, args);

    printf(RED "WARNING: %s\n" RESET_COLOR, buffer);
    fflush(stdout);

    va_end (args);
}

FUNC_TYPE resolveFunc(char *funcName)
{
    // Array of string values for function names.
    // Must be in sync with members of the FUNC_TYPE enum in order for resolveFunc to work.
    // For example, funcNames[NEG_FUNC] should be "neg"
    char *funcNames[] = {
            "neg",
            "abs",
            "add",
            "sub",
            "mult",
            "div",
            "remainder",
            "exp",
            "exp2",
            "pow",
            "log",
            "sqrt",
            "cbrt",
            "hypot",
            "max",
            "min",
            "equal",
            "less",
            "greater",
            "rand",
            "read",
            "print",
            // TODO complete the array - DONE
            // the empty string below must remain the last element
            ""
    };
    int i = 0;
    while (funcNames[i][0] != '\0')
    {
        if (strcmp(funcNames[i], funcName) == 0)
        {
            return i;
        }
        i++;
    }
    return CUSTOM_FUNC;
}

NUM_TYPE resolveType(char *type)
{
    char *types[] = {
            "int",
            "double"
    };

    if (strcmp(types[0], type) == 0)
    {
        return INT_TYPE;
    }
    else if (strcmp(types[1], type) == 0)
    {
        return DOUBLE_TYPE;
    }

    return NO_TYPE;
}

AST_NODE *createNumberNode(double value, NUM_TYPE type)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->type = NUM_NODE_TYPE;
    node->data.number.value = value;
    node->data.number.type = type;

    // TODO complete the function - DONE

    // Populate "node", the AST_NODE * created above with the argument data.
    // node is a generic AST_NODE, don't forget to specify it is of type NUMBER_NODE

    return node;
}


AST_NODE *createFunctionNode(FUNC_TYPE func, AST_NODE *opList)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->type = FUNC_NODE_TYPE;
    node->data.function.func = func;
    node->data.function.opList = opList;

    while (opList != NULL)
    {
        opList->parent = node;
        opList = opList->next;
    }

    // TODO complete the function - DONE
    // Populate the allocated AST_NODE *node's data

    return node;
}

AST_NODE *createCondNode(AST_NODE *cond, AST_NODE *_true, AST_NODE *_false)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->type = CONDITIONAL_NODE_TYPE;

    node->data.condition.condition = cond;
    node->data.condition._true = _true;
    node->data.condition._false = _false;

    node->data.condition.condition->parent = node;
    node->data.condition._true->parent = node;
    node->data.condition._false->parent = node;

    return node;
}

AST_NODE *createScopeNode(SYMBOL_TABLE_NODE *symbolTable, AST_NODE *scopeList)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    scopeList->parent = node;
    scopeList->symbolTable = symbolTable;

    node->type = SCOPE_NODE_TYPE;
    node->data.scope.child = scopeList;

    while (symbolTable != NULL)
    {
        symbolTable->value->parent = scopeList;
        symbolTable = symbolTable->next;
    }

    return node;
}

SYMBOL_TABLE_NODE *createSymbolNode_T(char *type, char *id, AST_NODE *val)
{
    SYMBOL_TABLE_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(SYMBOL_TABLE_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->id = calloc(sizeof(char), strlen(id)+1);
    strcpy(node->id, id);
    node->value = val;
    node->type = resolveType(type);

    if (val->data.number.type == DOUBLE_TYPE & node->type == INT_TYPE)
    {
        warning("Precision loss on int cast from %f to %d", val->data.number.value, (int) node->value->data.number.value);
    }

    if (node->value->type == NUM_NODE_TYPE)
    {
        node->value->data.number.type = node->type;
    }

    return node;
}

SYMBOL_TABLE_NODE *createSymbolNode_I(char *id, AST_NODE *val)
{
    SYMBOL_TABLE_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(SYMBOL_TABLE_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->id = calloc(sizeof(char), strlen(id)+1);
    strcpy(node->id, id);
    node->value = val;
    node->type = NO_TYPE;


    return node;
}

AST_NODE *createSymbolNode_U(char *id)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->data.symbol.id = id;
    node->type = SYM_NODE_TYPE;

    return node;
}

SYMBOL_TABLE_NODE *storeSymbolTableNode(SYMBOL_TABLE_NODE *newSymbol, SYMBOL_TABLE_NODE *symbolList)
{
    //References first STN for later reassignment
    SYMBOL_TABLE_NODE *hold = symbolList;

    while (symbolList != NULL)
    {
        if (strcmp(newSymbol->id, symbolList->id) == 0)
        {
            warning("The symbol \"%s\" already exists within the scope. Value remains unchanged.", newSymbol->id);
            return newSymbol;
        }
        symbolList = symbolList->next;
    }

    symbolList = hold;
    newSymbol->next = symbolList;

    return newSymbol;
}

AST_NODE *addExpressionToList(AST_NODE *newExpr, AST_NODE *exprList)
{
    // TODO complete the function
    // at newExpr to the exprList as the head. return the resulting list's head.

    newExpr->next = exprList;

    return newExpr;
}

RET_VAL evalNegFunc(AST_NODE *node)
{
    RET_VAL val;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);
    val.value = -val.value;

    if(node->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalAbsFunc(AST_NODE *node)
{
    RET_VAL val;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);

    if (val.type == INT_TYPE) {
        val.value = abs((int) val.value);
    }
    else {
        val.value = fabs(val.value);
    }


    if(node->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalAddFunc(AST_NODE *node)
{
    RET_VAL val, temp;
    node = node->data.function.opList;
    val.value = 0; //Initialized to 0
    val.type = INT_TYPE;

    if(node == NULL) {
        warning("Not enough parameters. Returning 0");
        return ZERO_RET_VAL;
    }

    for (int i = 0; node != NULL; i++)
    {
        temp = eval(node);
        val.value += temp.value;
        val.type = val.type || temp.type;
        node = node->next;
    }

    return val;
}

RET_VAL evalSubFunc(AST_NODE *node)
{
    RET_VAL val, val2;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);

    if(node->next == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    node = node->next;
    val2 = eval(node);
    val.type = val.type || val2.type;
    val.value -= val2.value;

    if(node->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalMultFunc(AST_NODE *node)
{
    RET_VAL val, temp;
    node = node->data.function.opList;
    val.value = 1; //Initialized value to 1 so that it doesn't return 0
    val.type = INT_TYPE;

    if(node == NULL) {
        warning("Not enough parameters. Returning 1");
        return val;
    }

    for (int i = 0; node != NULL; i++)
    {
        temp = eval(node);
        val.value *= temp.value;
        val.type = val.type || temp.type;
        node = node->next;
    }

    return val;
}

RET_VAL evalDivFunc(AST_NODE *node)
{
    RET_VAL val, temp;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);

    if(node->next == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    temp = eval(node->next);
    val.type = val.type || temp.type;
    if (val.type == INT_TYPE) {
        val.value = (int) val.value / (int) temp.value;
    }
    else
    {
        val.value /= temp.value;
    }

    if(node->next->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalRemFunc(AST_NODE *node)
{
    RET_VAL val, temp;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);

    if(node->next == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    temp = eval(node->next);
    val.type = val.type || temp.type;
    val.value = fmod(val.value, temp.value);

    if (val.value < 0)
    {
        val.value += fabs(temp.value);
    }

    if(node->next->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalExpFunc(AST_NODE *node)
{
    RET_VAL val;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);
    val.type = DOUBLE_TYPE;
    val.value = (double) exp(val.value);

    if(node->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalExp2Func(AST_NODE *node)
{
    RET_VAL val;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);

    if(val.value < 0)
    {
        val.type = DOUBLE_TYPE;
        val.value = (double) pow(2, val.value);
    }
    else
    {
        val.value = pow(2, val.value);
    }

    if(node->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalMinFunc(AST_NODE *node)
{
    RET_VAL min, temp;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    min = eval(node);
    node = node->next;

    while (node != NULL)
    {
        temp = eval(node);
        if (temp.value < min.value)
        {
            min.value = temp.value;
            min.type = temp.type;
        }
        node = node->next;
    }

    return min;
}

RET_VAL evalMaxFunc(AST_NODE *node)
{
    RET_VAL max, temp;
    node = node->data.function.opList;

    if (node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    max = eval(node);
    node = node->next;

    while (node != NULL)
    {
        temp = eval(node);
        if (temp.value > max.value)
        {
            max.value = temp.value;
            max.type = temp.type;
        }
        node = node->next;
    }

    return max;
}

RET_VAL evalHypotFunc(AST_NODE *node)
{
    RET_VAL val, temp;
    node = node->data.function.opList;


    if (node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return ZERO_RET_VAL;
    }

    val.type = DOUBLE_TYPE;
    val.value = 0;

    for (int i = 0; node != NULL; i++)
    {
        temp = eval(node);
        val.value += pow(temp.value, 2);
        node = node->next;
    }

    val.value = sqrt(val.value);

    return val;
}

RET_VAL evalCbrtFunc(AST_NODE *node)
{
    RET_VAL val;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);
    val.type = DOUBLE_TYPE;
    val.value = cbrt(val.value);

    if(node->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalSqrtFunc(AST_NODE *node)
{
    RET_VAL val;
    node = node->data.function.opList;

    if(node == NULL) {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);
    val.type = DOUBLE_TYPE;
    val.value = sqrt(val.value);

    if(node->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalLogFunc(AST_NODE *node)
{
    RET_VAL val;
    node = node->data.function.opList;

    val = eval(node);
    val.type = DOUBLE_TYPE;
    val.value = log(val.value);

    if(node->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalPowFunc(AST_NODE *node)
{
    RET_VAL val, temp;
    node = node->data.function.opList;

    if(node == NULL)
    {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);

    if(node->next == NULL)
    {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    temp = eval(node->next);
    val.type = val.type || temp.type;
    val.value = pow(val.value, temp.value);

    if(node->next->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalLessFunc(AST_NODE *node)
{
    RET_VAL val, temp;
    node = node->data.function.opList;

    if(node == NULL)
    {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);

    if(node->next == NULL)
    {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    temp = eval(node->next);
    val.value = (val.value < temp.value) ? (val.value = 1) : (val.value = 0);

    if(node->next->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalGreaterFunc(AST_NODE *node)
{
    RET_VAL val, temp;
    node = node->data.function.opList;

    if(node == NULL)
    {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);

    if(node->next == NULL)
    {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    temp = eval(node->next);
    val.value = (val.value > temp.value) ? (val.value = 1) : (val.value = 0);

    if(node->next->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalEqualFunc(AST_NODE *node)
{
    RET_VAL val, temp;
    node = node->data.function.opList;

    if(node == NULL)
    {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    val = eval(node);

    if(node->next == NULL)
    {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    temp = eval(node->next);
    val.value = (val.value == temp.value) ? (val.value = 1) : (val.value = 0);

    if(node->next->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return val;
}

RET_VAL evalPrintFunc(AST_NODE *node)
{
    node = node->data.function.opList;

    if(node == NULL)
    {
        warning("Not enough parameters. Returning NAN");
        return NAN_RET_VAL;
    }

    printRetVal(node->data.number);

    if(node->next != NULL) {
        warning("Extra parameters ignored.");
    }

    return node->data.number;
}

RET_VAL evalRandFunc()
{
    return (RET_VAL){DOUBLE_TYPE, (((double) rand() / (RAND_MAX)))};
}

RET_VAL evalReadFunc()
{
    int offset; // Number of characters read by sscanf
    double value;
    char buffer[64];
    printf("read :: ");
    fscanf(read_target, "%[^\n]\n", buffer);

    if (strcmp(buffer, "0") == 0) { return ZERO_RET_VAL; }
    printf("%s\n", buffer);

    if (sscanf(buffer, "%lf%n", &value, &offset) != 1) {
        if (offset != strlen(buffer - 1))
        {
            warning("Invalid read entry! NAN returned!");
            return NAN_RET_VAL;
        }
        warning("Invalid read entry! NAN returned!");
        return NAN_RET_VAL;
    }

    RET_VAL val = {NO_TYPE, value};

    int intVal = trunc(val.value);
    if (val.value == intVal) {
        val.type = INT_TYPE;
    } else {
        val.type = DOUBLE_TYPE;
    }

    return val;
}

RET_VAL evalConditionalFunc(AST_NODE *node)
{
    return (eval(node->data.condition.condition).value == 0) ?
           eval(node->data.condition._false) :
           eval(node->data.condition._true);
}
RET_VAL evalCustomFunc(AST_NODE *node)
{
    RET_VAL val = (RET_VAL){INT_TYPE, 1};
    //Initial Framework Attempt. Commented out to prevent errors
    /*int i = 0; //i is counter to store list head of stack
    AST_NODE *temp = node;
    STACK_NODE *stack, *stackHead;
    SYMBOL_TABLE_NODE *arg = node->symbolTable->value->argTable;
    node = node->data.function.opList;

    //Initialize stackNode
    size_t stackSize = sizeof(STACK_NODE);
    if ((stack = calloc(stackSize, 1)) == NULL)
    { yyerror("Memory allocation failed!"); exit(1); }

    //Allocate space for and store values on stack
    while (node != NULL)
    {
        stack->value = node->data.number;
        if (node->next != NULL)
        {
            if ((stack->next = calloc(stackSize, 1)) == NULL)
            { yyerror("Memory allocation failed!"); exit(1); }
            if (!i) { stackHead = stack; }
            stack = stack->next;
        }
        node = node->next;
        i++;
    }

    stack = stackHead; //Reset stack address
    node = temp->symbolTable->value; //Reset node address

    while (node->argTable != NULL && stack != NULL)
    {
        node->argTable->value->data.number = stack->value;
        node->argTable = node->argTable->next;
        stack = stack->next;
    }

    node->argTable = arg; //Reset argTable address

    val = eval(temp->symbolTable->value);

    if (temp->symbolTable->type == INT_TYPE && val.type == DOUBLE_TYPE)
    {
        warning("Precision loss on int cast from %.3lf to %d", val.value, (int) val.value);
        val.type = INT_TYPE;
        val.value = trunc(val.value);
    }
*/
    return val;
}

RET_VAL evalFuncNode(AST_NODE *node)
{
    RET_VAL val;

    if (!node || node == NULL)
    {
        yyerror("NULL AST_NODE passed into evalFuncNode!");
        return NAN_RET_VAL; // unreachable but kills a clang-tidy warning
    }

    switch (node->data.function.func)
    {
        case NEG_FUNC:
            val = evalNegFunc(node);
            break;
        case ABS_FUNC:
            val = evalAbsFunc(node);
            break;
        case ADD_FUNC:
            val = evalAddFunc(node);
            break;
        case SUB_FUNC:
            val = evalSubFunc(node);
            break;
        case MULT_FUNC:
            val = evalMultFunc(node);
            break;
        case DIV_FUNC:
            val = evalDivFunc(node);
            break;
        case REM_FUNC:
            val = evalRemFunc(node);
            break;
        case EXP_FUNC:
            val = evalExpFunc(node);
            break;
        case EXP2_FUNC:
            val = evalExp2Func(node);
            break;
        case POW_FUNC:
            val = evalPowFunc(node);
            break;
        case LOG_FUNC:
            val = evalLogFunc(node);
            break;
        case SQRT_FUNC:
            val = evalSqrtFunc(node);
            break;
        case CBRT_FUNC:
            val = evalCbrtFunc(node);
            break;
        case HYPOT_FUNC:
            val = evalHypotFunc(node);
            break;
        case MAX_FUNC:
            val = evalMaxFunc(node);
            break;
        case MIN_FUNC:
            val = evalMinFunc(node);
            break;
        case EQUAL_FUNC:
            val = evalEqualFunc(node);
            break;
        case LESS_FUNC:
            val = evalLessFunc(node);
            break;
        case GREATER_FUNC:
            val = evalGreaterFunc(node);
            break;
        case RAND_FUNC:
            val = evalRandFunc();
            break;
        case READ_FUNC:
            val = evalReadFunc();
            break;
        case PRINT_FUNC:
            val = evalPrintFunc(node);
            break;
        case CUSTOM_FUNC:
            val = evalCustomFunc(node);
        default:
            break;
    }

    // TODO complete the function - DONE
    // HINT:
    // the helper functions that it calls will need to be defined above it
    // because they are not declared in the .h file (and should not be)

    return val;
}


RET_VAL evalNumNode(AST_NODE *node)
{
    RET_VAL val;
    val = node->data.number;

    // TODO complete the function - DONE

    return val;
}

RET_VAL evalScopeNode(AST_NODE *node)
{
    RET_VAL val;
    val = eval(node->data.scope.child);

    return val;
}

RET_VAL evalSymbolNode(AST_NODE *node)
{
    RET_VAL val;

    if (!node)
    {
        yyerror("NULL ast node passed into eval!");
        return NAN_RET_VAL;
    }

    AST_NODE *evalNode = node;
    SYMBOL_TABLE_NODE *sTN;

    while (evalNode != NULL)
    {
        sTN = evalNode->symbolTable;
        while (sTN != NULL)
        {
            if (strcmp(node->data.symbol.id, sTN->id) == 0)
            {
                val = eval(sTN->value);
                if (sTN->value->type != NUM_NODE_TYPE)
                {
                    freeNode(sTN->value);
                    sTN->value = createNumberNode(val.value, sTN->type);
                    val.type = sTN->type;
                }
                return val;
            }
            sTN = sTN->next;
        }
        evalNode = evalNode->parent;
    }

    warning(">>> Symbol \"%s\" not found. Returning NAN.", node->data.symbol.id);
    return NAN_RET_VAL;
}

RET_VAL eval(AST_NODE *node)
{
    RET_VAL val;

    if (!node)
    {
        yyerror("NULL ast node passed into eval!");
        return NAN_RET_VAL;
    }

    switch (node->type)
    {
        case FUNC_NODE_TYPE:
            val = evalFuncNode(node);
            break;
        case NUM_NODE_TYPE:
            val = evalNumNode(node);
            break;
        case SCOPE_NODE_TYPE:
            val = evalScopeNode(node);
            break;
        case SYM_NODE_TYPE:
            val = evalSymbolNode(node);
            break;
        case CONDITIONAL_NODE_TYPE:
            val = evalConditionalFunc(node);
            break;
    }

    return val;
}

// prints the type and value of a RET_VAL
void printRetVal(RET_VAL val)
{
    switch (val.type)
    {
        case INT_TYPE:
            printf("Integer : %.lf\n", val.value);
            break;
        case DOUBLE_TYPE:
            printf("Double : %lf\n", val.value);
            break;
        default:
            printf("No Type : %lf\n", val.value);
            break;
    }
}

void freeFuncNode(AST_NODE *node)
{
    freeNode(node->data.function.opList);
}

void freeScopeNode(AST_NODE *node)
{
    freeNode(node->data.scope.child);
}

void freeSymbolTableNode(AST_NODE *node)
{
    if (!node)
    {
        return;
    }

    free(node->data.symbol.id);
    freeSymbolTableNode(node->next);
    free(node);
}

void freeCondNode(AST_NODE *node)
{
    freeNode(node->data.condition.condition);
    freeNode(node->data.condition._false);
    freeNode(node->data.condition._true);
}

/*
 * NOT FUNCTIONING
 * Framework for evaluation
 */
void freeStackNode(AST_NODE *node)
{
    if (!node) {
        return;
    }
    free(node->next);
    free(node);
}

void freeNode(AST_NODE *node)
{
    if (!node)
    {
        return;
    }

    // TODO complete the function

    // look through the AST_NODE struct, decide what
    // referenced data should have freeNode called on it
    // (hint: it might be part of an s_expr_list, with more
    // nodes following it in the list)

    // if this node is FUNC_TYPE, it might have some operands
    // to free as well (but this should probably be done in
    // a call to another function, named something like
    // freeFunctionNode)

    // and, finally,
    freeNode(node->next);

    switch (node->type)
    {
        case FUNC_NODE_TYPE:
            freeFuncNode(node);
            break;
        case CONDITIONAL_NODE_TYPE:
            freeCondNode(node);
            break;
        case SYM_NODE_TYPE:
            freeSymbolTableNode(node);
            break;
        case SCOPE_NODE_TYPE:
            freeScopeNode(node);
            break;
        case NUM_NODE_TYPE:
        default:
            break;
    }

    free(node);
}