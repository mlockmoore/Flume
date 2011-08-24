// math_expression.h  -- Header file for math expression types and functions
// v. 0.2.7  2010-12-26

#ifndef __math_expression_h
#define __math_expression_h

#define MAXENTRY    64
#define MAXENTRYLEN 256

// Abnormally small limits to help during testing
//#define MAXENTRY    3
//#define MAXENTRYLEN 20

#define MAXITEMS    256

enum {INT_NUM, HEX_NUM, BIN_NUM, REAL_NUM, SCI_NUM};

#define MAX_OPER 128
enum {OPER_UNKNOWN, CONST, OPER_STORE, OPER_ADD, OPER_SUB, OPER_MULT, 
      OPER_DIV, OPER_MOD, OPER_REMAINDER, OPER_ROUND,
      OPER_AND, OPER_OR, OPER_XOR, OPER_NOT,
      OPER_POW, OPER_LN, OPER_LOG10, OPER_SIGN, 
      OPER_DEG2RAD, OPER_RAD2DEG, OPER_DEGREES, OPER_RADIANS,
      OPER_SIN, OPER_COS, OPER_TAN, OPER_ARCSIN, OPER_ARCCOS, OPER_ARCTAN,
      OPER_PREVRES, OPER_RESULT, OPER_PERCENT, OPER_FACT, OPER_SQRT,
      OPER_INT, OPER_HEX, OPER_BIN, OPER_REAL, OPER_SCI
};

enum {PUSH_OPER, INFIX_OPER, SUFFIX_OPER, FUNC_OPER, REF_OPER};

enum {CALC_SUCCESS = 0, CALC_UNBALANCED, CALC_EXPRLENGTH, CALC_ERROR};

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define PI      3.1415926535897932384626433832795l
#define E_NUM   2.71828182845904523536l
#define DEG_PER_RAD (180.0/PI)

struct item {
    double  val;
    int     fmt;
    int     oper;
    int     lvl;
    int     used;
    int     prec;   // # of preceeding args needed for operations
    int     pow;    // power of ten (order of magnitude) for the overall value
    int     sigpow; // power of value's least-significant digit of value
                    //   (rounding to this power of ten does not appreciably
                    //    change the accuracy of the value)
};

//char Pad[256];
extern struct item ResultList[64];
extern int Result;
extern char Note_str[MAXENTRY][256];
extern int AngleUnitsAreDegrees;
extern int TrackSignificance;
extern int SayThreshold;

void clear_item(struct item* i);

void render_item(char* outstr, struct item* i);

struct operation_def {
    int oper;
    int type;
    int ignore_case;
    char name[32];
    char desc[64];
    int prec;       // number of arguments needed for operation
    double val;
};

extern struct operation_def operation_defs[MAX_OPER];

int build_expression(char* exp, struct item* eval_list);

int evaluate_expression(struct item* eval_list, int items, int resultindex); 

void print_expression(struct item* eval_list, int items);

void prepare_expression(char* raw_expr, char* clean_expr);

void group_for_precedence(char* inexpr, int len, char* outexpr);

int expr_is_unbalanced(char* expr);

void SAY(int level, const char * format, ...);

int calculate_expr(char* expr_input_str, char* expr_fixed_str, int resultnum);

#endif
