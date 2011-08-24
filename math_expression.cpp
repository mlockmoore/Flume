// math_expression.cpp  -- Compiles a text math expression into a
//                       computation list.
// v. 0.2.7  2010-12-27
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include "math_expression.h"

// UNCOMMENT OR EXTERNALLY DEFINE THIS TO BUILD STANDALONE 
// TEST APP FOR MATH EXPRESSION ENGINE
//#define STANDALONE 1

enum {STD_INPUT, RPN_INPUT};
int InputMode = STD_INPUT;

// Controls the overall verbosity of the descriptive statements
int SayThreshold = 3;

// Uncomment this to enable descriptive statements
#define THINKOUTLOUD 1


//char Pad[256];
struct item ResultList[MAXENTRY];
char Note_str[MAXENTRY][256];
int Result = 0;

int AngleUnitsAreDegrees = 0;
int TrackSignificance = 0;

struct operation_def operation_defs[MAX_OPER] = {
    {OPER_UNKNOWN,  0,          0, "?",         "??", 0, 0.0},
    {CONST,         PUSH_OPER,  1, "pi",        "PUSH CONSTANT pi", 31, PI},
    {CONST,         PUSH_OPER,  0, "e",         "PUSH CONSTANT e", 20, E_NUM},
    {OPER_STORE,    INFIX_OPER, 0, "STORE",     "STORE RESULT", 1, 0.0},
    {OPER_ADD,      INFIX_OPER, 0, "+",         "ADD", 2, 0.0},  
    {OPER_SUB,      INFIX_OPER, 0, "-",         "SUBTRACT", 2, 0.0},   
    {OPER_MULT,     INFIX_OPER, 0, "*",         "MULTIPLY", 2, 0.0},   
    {OPER_MULT,     INFIX_OPER, 1, "x",         "MULTIPLY", 2, 0.0},   
    {OPER_MULT,     INFIX_OPER, 1, "of",        "MULTIPLY", 2, 0.0},   
    {OPER_DIV,      INFIX_OPER, 0, "/",         "DIVIDE", 2},  
    {OPER_MOD,      INFIX_OPER, 1, "mod",       "MODULUS", 2},  
    {OPER_AND,      INFIX_OPER, 0, "&",         "AND bit-wise", 2, 0.0},  
    {OPER_AND,      INFIX_OPER, 1, "and",       "AND bit-wise", 2, 0.0},  
    {OPER_OR,       INFIX_OPER, 0, "|",         "OR bit-wise", 2, 0.0},  
    {OPER_OR,       INFIX_OPER, 1, "or",        "OR bit-wise", 2, 0.0},  
    {OPER_XOR,      INFIX_OPER, 1, "xor",       "XOR bit-wise", 2, 0.0},  
    {OPER_POW,      INFIX_OPER, 0, "^",         "RAISE TO A POWER", 2, 0.0},
    {OPER_POW,      INFIX_OPER, 1, "power",     "RAISE TO A POWER", 2, 0.0},
    {OPER_SQRT,     INFIX_OPER, 1, "sqrt",      "SQUARE ROOT", 1, 0.0},
    {OPER_POW,      INFIX_OPER, 0, "^",         "RAISE TO A POWER", 2, 0.0},
    {OPER_NOT,      INFIX_OPER, 0, "~",         "NOT bit-wise (invert)", 1, 0.0}, 
    {OPER_LN,       FUNC_OPER,  1, "ln",        "NATURAL LOGARITHM", 1, 0.0},
    {OPER_LOG10,    FUNC_OPER,  1, "log",       "LOGARITHM", 1, 0.0},
    {OPER_LOG10,    FUNC_OPER,  1, "log10",     "LOGARITHM", 1, 0.0},
    {OPER_SIGN,     FUNC_OPER,  1, "sign",      "SIGN", 1, 0.0},
    {OPER_SIGN,     FUNC_OPER,  1, "sgn",       "SIGN", 1, 0.0},
    {OPER_DEG2RAD,  FUNC_OPER,  1, "deg2rad",   "CONVERT DEG->RADIANS", 1, 0.0},
    {OPER_RAD2DEG,  FUNC_OPER,  1, "rad2deg",   "CONVERT RADIANS->DEG", 1, 0.0},
    {OPER_SIN,      FUNC_OPER,  1, "sin",       "SIN", 1, 0.0},
    {OPER_COS,      FUNC_OPER,  1, "cos",       "COS", 1, 0.0},
    {OPER_TAN,      FUNC_OPER,  1, "tan",       "TAN", 1, 0.0},
    {OPER_ARCSIN,   FUNC_OPER,  1, "arcsin",    "ARCSIN", 1, 0.0},
    {OPER_ARCCOS,   FUNC_OPER,  1, "arccos",    "ARCCOS", 1, 0.0},
    {OPER_ARCTAN,   FUNC_OPER,  1, "arctan",    "ARCTAN", 1, 0.0},
    {OPER_PREVRES,  REF_OPER,   1, "p",         "FETCH PRIOR RELATIVE RESULT", 1, 0.0},
    {OPER_RESULT,   REF_OPER,   1, "r",         "FETCH SPECIFIC RESULT", 1, 0.0},
    {OPER_PERCENT,  SUFFIX_OPER,0, "%",         "PERCENT", 1, 0.0},
    {OPER_FACT,     SUFFIX_OPER,0, "!",         "FACTORIAL", 1, 0.0},
    {OPER_REMAINDER,FUNC_OPER,  1, "remainder", "REMAINDER", 1, 0.0},
    {OPER_ROUND,    FUNC_OPER,  1, "round",     "ROUND", 2, 0.0},
    {OPER_INT,      SUFFIX_OPER,1, "int",       "INTEGER FORMAT", 1, 0.0},
    {OPER_INT,      SUFFIX_OPER,1, "integer",   "INTEGER FORMAT", 1, 0.0},
    {OPER_HEX,      SUFFIX_OPER,1, "hex",       "HEXADECIMAL FORMAT", 1, 0.0},
    {OPER_BIN,      SUFFIX_OPER,1, "bin",       "BINARY FORMAT", 1, 0.0},
    {OPER_REAL,     SUFFIX_OPER,1, "real",      "REAL FORMAT", 1, 0.0},
    {OPER_SCI,      SUFFIX_OPER,1, "sci",       "SCIENTIFIC FORMAT", 1, 0.0},
    {-1,            0,          0, "?",         "??", 0, 0.0}
};

// Forward declaration
double fround(double a);

void SAY(int level, const char * format, ...)
{
#ifdef THINKOUTLOUD
    va_list ap;
    char buf[8192];

    if (level >= SayThreshold) {
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
    }
    if (level > 1) fflush(0);
#endif
}

void clear_item(struct item* i)
{
    i->val  = 0.0;
    i->fmt  = INT_NUM;
    i->oper = CONST;
    i->lvl  = 0;
    i->used = 0;
    i->prec = -1;
    i->pow = 0;
    i->sigpow = 0;
}

int got_integer(char* buf, struct item* i)
{
    long int n, r = 0;
    int fmt = INT_NUM;
    char extra[128] = "";
    if (buf[0] == '0' && (buf[1] == 'x' ||  buf[1] == 'X')) {
        fmt = HEX_NUM;
        if (1 == sscanf(buf, "%x", &n)) {
            r = 1;
        }
    }
    else if (buf[0] == '0' && (buf[1] == '`' || buf[1] == 'b' || buf[1] == 'B')) {
        fmt = BIN_NUM;
        n = 0;
        char* p = buf + 2;
        while (*p && (*p == '1' || *p == '0')) {
            n =  n << 1 | (*p - '0');
            p++;
        }
        if (!*p) {
            r = 1;
        } 
    }
    else {
        if (    strstr(buf, "e") || strstr(buf, "E") 
//            ||  strstr(buf, "x") || strstr(buf, "X")  //???
            ||  strstr(buf, ".")) {
            // looks like scientific or real
            r = 0;
        } 
        else if (1 == sscanf(buf, "%i%s", &n, extra)) {
            r = 1;
        }
        else {
            SAY(1, "Extra character(s) '%s' not part of an integer!\n", extra);
            r = 0;
        }
    }
    if (r == 1) {
        clear_item(i);
        i->val = (double)n;
        i->fmt = fmt;
        n = (int)fabs(n);
        i->pow = (int)floor(log10(n));
        i->sigpow = 0; 
        while ((fmt == INT_NUM)&& (n % 10) == 0) {
            i->sigpow ++;
            n = n/10;
        }
        SAY(0, "fmt is %d, after checking trailing zeros sigpow is %d\n", 
            fmt, i->sigpow);
        SAY(1, "Integer %d is of magnitude %d, significant to the 10 to the %d place\n",
                (int)i->val, i->pow, i->sigpow); fflush(0);
    }

    return r;
}

int got_real(char* buf, struct item* i)
{
    int d;
    double n;
    int ret = 0;
    int sigdigs = 0;
    int l = strlen(buf);
    int fmt = REAL_NUM;
    for (d = 0; d < l; d++) {
        if (buf[d] == 'e' || buf[d] == 'E') {
            fmt = SCI_NUM;
            if (1 == sscanf(buf, "%lf", &n)) {
                ret = 1;
                break;
            }
        }
    }
    if (ret == 0 && 1 == sscanf(buf, "%lf", &n)) {
        d = l;
        ret = 1;
    }
    
    if (ret == 1) {
        clear_item(i);
        i->val = n;
        i->fmt = fmt;
        i->pow = floor(log10(fabs(n)));
        i->sigpow = 0;

        int b = d - 1;
        sigdigs = d;
        
        int dp_found = 0;
        int c;
        for (c = 0; c < d ; c++) {
            if (buf[c] == '-') {
                sigdigs--;
            }
            else if (buf[c] == '.') {
                sigdigs--;
                dp_found = 1;
            }
        }
        if (!dp_found && (fmt == SCI_NUM)) {
            for (c = d - 1; c > 0; c--) {
                if (buf[c] == '0') {
                    sigdigs--;
                }
            }
        }
        
        i->sigpow = i->pow - sigdigs + 1;
        
        SAY(0, "real/sci number %1.11lf has power %d and is significant in the 10 to the %d place.\n",
                n, i->pow, i->sigpow); fflush(0);
    }
    return ret;
}

void render_item(char* outstr, struct item* i) {
    char fmt_str[32]        = {""};
    char scratch_str[256]   = {""};
    char *p = scratch_str;
    char *e;
    int bit;
    char bitchar[2] = "*";
    memset(scratch_str, 0, 256);
    unsigned long long int n;
    double v = i->val;
    double sig_power_of_ten = pow(10.0l, i->sigpow);
    if (TrackSignificance) {
        v = sig_power_of_ten * fround(v/sig_power_of_ten);
        SAY(1, "Render: %1.8lf rounded to nearest %1.8lf is %1.8lf\n",
            i->val, sig_power_of_ten, v);
    }
    SAY(0, "render_item() for fmt %d\n", i->fmt);
    switch(i->fmt) {
        case INT_NUM:
            sprintf(outstr, "%d", (long int)v);
            break;
        
        case BIN_NUM:
            n = (unsigned long long int)v;
            bit = 0;
            //write the bits in reverse order
            while (n > 0) {
                if (n & 1) {
                    scratch_str[bit] = '1'; 
                }
                else {
                    scratch_str[bit] = '0'; 
                }
                bit++;
                n >>= 1;
                //SAY(1, "%s\n", scratch_str);
            }
            //left-pad with 0 bits to next nibble
            while (bit < 1 || bit % 4) {
                scratch_str[bit] = '0';
                bit++;
                //SAY(1, "%s\n", scratch_str);
            }
            //now output in correct orderSAY(1, "
            sprintf(outstr, "0`");
            do {
                bit--;
                bitchar[0] = scratch_str[bit];
                strcat(outstr, bitchar);
                //SAY(1, "%s\n", outstr);
            } while (bit > 0);
            break;
            
        case HEX_NUM:
            if (fabs(v) < 256.0) {
                sprintf(outstr, "0x%02X", (long unsigned int)i->val);
            }
            else if (fabs(v) < 65536.0) {
                sprintf(outstr, "0x%04X", (long unsigned int)i->val);
            }
            else if (fabs(v) < 4294967296.0) {
                sprintf(outstr, "0x%08X", (long unsigned int)i->val);
            }
            else {
                long unsigned int uw = (long long unsigned int)v >> 32;
                long unsigned int lw = (long long unsigned int)v &  0xFFFFFFFF;
                sprintf(outstr, "0x%08X%08X", uw, lw);
            }
            break;
            
        case REAL_NUM:
            sprintf(scratch_str, "%1.8lf", v);
SAY(0, "Starting formated real num: '%s'\n", scratch_str);
            while (*p && *p != '.') {
                p++;
            }
            if (*p == '.') {
                p++;
            }
            e = scratch_str + strlen(scratch_str) - 1;
            while (e > p && *e == '0') {
                *e = '\0';
                e--;
            }
SAY(0, "Final formated real num: '%s'\n", scratch_str);
            sprintf(outstr, "%s", scratch_str);
            break;
            
        case SCI_NUM:
            sprintf(outstr, "%e", v);
            break;
    }
}

int looks_real(char* buf)
{
    char* p = buf;
    while (p && *p) {
        if (*p == '.' || *p == 'e' || *p == 'E') {
            return 1;
        }
        p++;
    }
    return 0;
}

int usable_number(char *buf, struct item* i)
{
    char scratch_str[256]       = {""};
    int itemlen = strlen(buf);
    
    if (itemlen > 0) {
//        SAY(1, "Item is '%s', looks real is %d\n", Pad, looks_real());
        if (got_integer(buf, i)) {
            SAY(1, "'%s' is an integer\n", buf);
        }
        else if (looks_real(buf) && got_real(buf, i)) {
            SAY(1, "'%s' is a real\n", buf);
        }
        else {
//            SAY(1, "'%s' does not look like a number!\n", buf);
            return 0;
        }
        render_item(scratch_str, i);
        SAY(1, "Item renders as '%s', format is %d\n", scratch_str, i->fmt);
    }
    i->used = 0;
    return itemlen;
}

int operation(char* p, struct item* i)
{
    char token_str[32]      = {""};
    char scratch_str[256]   = {""};
    int itemlen = strlen(p);
    i->used = 0;
    int found = 0;
    int op = 0;
    while (operation_defs[op].oper >= 0) {
        strcpy(token_str, p);
        char * name_str = operation_defs[op].name;
        
        SAY(0, "Checking '%s' from expr against defined operation '%s'\n", 
                token_str, name_str); fflush(0);
        found = 0;
        if (    !strncmp(name_str, token_str, strlen(name_str)) 
            &&  strlen(name_str) == strlen(token_str) ) {
            found = 1;
        }
        else if (operation_defs[op].ignore_case) {
                char *a = token_str;
                while (*a) { 
                    *a++ = tolower(*a);
                }
                SAY(0, "Checking '%s' from expr against defined operation '%s'\n", token_str, name_str);
                if (    !strncmp(name_str, token_str, strlen(name_str)) 
                    &&  strlen(name_str) == strlen(token_str) ) {
                    found = 1;
                }
        }
        if (found) {
            i->oper = operation_defs[op].oper;
            i->prec = operation_defs[op].prec;
            i->val  = operation_defs[op].val;
            i->fmt  = REAL_NUM;
            SAY(1, "OK, it is the %s operation\n", operation_defs[op].desc);
            return strlen(operation_defs[op].name);
        }
        else {
            op++;
        }
    }
    
    SAY(1, "   (not a recognized operation or function)\n");
    return 0;    
}

int have_item(char* src, char* dest)
{
    char* p = src;
    char* d = dest;
    SAY(1, "Looking at '%s'\n", p);
    while (p && *p) {
        switch (*p) {
            case '-':
                if (p > src && *(p-1) != 'e' && *(p-1) != 'E' ) {
                    *d = '\0';
                    return (d - dest);
                }
                else {
                    *d++ = *p++;
                }
                break;
                
            case ' ':
            case '=':
                *d = '\0';
                return (d - dest);
                break;
                
            case 'x':
            case 'X':
                if (*(p-1) == '0' || *(p+1) == 'o' || *(p+1) == 'O') {
                    // it's part of hex num or maybe XOR
                    *d++ = *p++;
                }
                else if (   (*(p-2) == 'H' || *(p-2) == 'h')
                         && (*(p-1) == 'E' || *(p-1) == 'e')) {
                    // it's part of "hex" formating command
                    *d++ = *p++;
                }
                else {
                    if (d == dest) {
                        *d++ = *p++;
                    }
                    *d = '\0';
                    return (d - dest);
                }
                break;
            
            case '(':
            case ')':
            case '[':
            case ']':
            case '+':
            case '*':
            case '/':
            case '^':
            case '&':
            case '|':
            case '~':
            case '!':
            case '%':
            case ',':
                if (d == dest) {
                    *d++ = *p++;
                }
                *d = '\0';
                return (d - dest);
                break;
            
            default:
                *d++ = *p++;
                break;
        }
    }
    *d = '\0';
    return (d - dest);
}

int build_rpn_expression(char* exp, struct item* eval_list)
{
    int r;
    struct item i;
    i.lvl = 0;
    int loc = 0;
    char pad[1024]  = {""};
    while (r = have_item(exp, pad)) {
        if (usable_number(pad, &i)) {
            memcpy(&eval_list[loc], &i, sizeof(struct item)); 
            SAY(1, "Compiled num %f at loc %d\n", eval_list[loc].val, loc); fflush(0);
            loc++;
        }
        
        else if (!strncmp(pad, "AS", 2) || !strncmp(pad, "as", 2)) {
            SAY(0, "Skipping word 'as'.\n");
            r = 2;
        }
        else if (operation(pad, &i)) {
            memcpy(&eval_list[loc], &i, sizeof(struct item)); 
            SAY(1, "Compiled operation %d at loc %d\n", eval_list[loc].oper, loc); fflush(0);
            loc++;
        }
        exp += r;
        while (*exp && isspace(*exp)) {
            SAY(0, "Skipping '%c'\n", *exp);
            exp++;
        }  
    }
    if (loc > 0) {
        eval_list[loc].oper = OPER_STORE;
        eval_list[loc].prec = 1;
        eval_list[loc - 1].used = 1;
        eval_list[loc].used = 1;
        loc++;
    }
    return loc;
}

int build_expression(char* exp, struct item* eval_list)
{
    struct item i;

    int loc = 0;

    int args[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int lvl = 0;

    struct item oper_list[16];
    int pending = 0;
    int r;
    char pad[1024]  = {""};
    while (r = have_item(exp, pad)) {
        if (usable_number(pad, &i)) {
            i.lvl = lvl;
            memcpy(&eval_list[loc], &i, sizeof(struct item)); 
            SAY(1, "Compiled num %f at loc %d, lvl %d, pow %d, sigpow %d\n", 
                eval_list[loc].val, loc, eval_list[loc].lvl, eval_list[loc].pow, eval_list[loc].sigpow);
            args[lvl]++;
            loc++;
            SAY(1, "There are %d args avail at lvl %d\n", args[lvl], lvl);
fflush(0);
            int needed = oper_list[pending - 1].prec;
            while (pending && oper_list[pending - 1].lvl == lvl 
                    && needed <= args[lvl]) {
                memcpy(&eval_list[loc], &oper_list[pending - 1], sizeof (struct item));
                SAY(1, "Compiled oper type %d at loc %d, lvl %d\n", 
                        eval_list[loc].oper, loc, eval_list[loc].lvl);
fflush(0);
                args[lvl] -= (needed - 1);
                pending--;
                int l = loc - 1; // most recent item perhaps not used yet
                while (needed && l >= 0) {
                    if (!eval_list[l].used) {
                        eval_list[l].used = 1;
                        SAY(1, "Marked loc %d as used\n", l);
fflush(0);
                        needed--;
                    }
                    l--;
                }
                loc++;
                needed = oper_list[pending - 1].prec;
            }
        }
        else if (pad[0] == ',') {
            SAY(0, "Skipping ,\n");
            r = 1;
        }
        else if (pad[0] == '[') {
            SAY(0, "Skipping [\n");
            r = 1;
        }
        else if (pad[0] == ']') {
            SAY(0, "Skipping ]\n");
            r = 1;
        }
        else if (pad[0] == '(') {
            lvl++;
            SAY(1, "Nesting into level %d\n", lvl);
            r = 1;
        }
        else if (pad[0] == ')') {
            // is there at least one item in previous level to promote?
            lvl--;
            SAY(1, "Unnesting out to level %d\n", lvl);
fflush(0);
            int prev = loc - 1;
            while (prev >= 0 && eval_list[prev].lvl == lvl + 1) {
                    SAY(1, "'''' Item %d is level %d, used? %d\n", prev, eval_list[prev].lvl, eval_list[prev].used);
fflush(0);
                if (!eval_list[prev].used) {
                    args[lvl]++;
                    args[lvl + 1]--;
                    eval_list[prev].lvl = lvl;
                    SAY(1, "Promoting item %d to level %d\n", prev, eval_list[prev].lvl);
fflush(0);
                }
                prev--;
            }
            int needed = oper_list[pending - 1].prec;
            while (pending && oper_list[pending - 1].lvl == lvl 
                    && needed <= args[lvl]) {
                memcpy(&eval_list[loc], &oper_list[pending - 1], sizeof (struct item));
                SAY(1, "Compiled oper type %d at loc %d\n", eval_list[loc].oper, loc);
fflush(0);
                args[lvl] -= (needed - 1);
                pending--;
                int l = loc - 1; // most recent item perhaps not used yet
                while (needed && l >= 0) {
                    if (!eval_list[l].used) {
                        eval_list[l].used = 1;
                        needed--;
                        SAY(1, "Marked loc %d as used, %d more needed\n", l, needed);
fflush(0);
                    }
                    l--;
                }
                loc++;
                needed = oper_list[pending - 1].prec;
            }
            r = 1;
        }
        else if (!strncmp(pad, "AS", 2) || !strncmp(pad, "as", 2)) {
            SAY(0, "Skipping word 'as'.\n");
            r = 2;
        }
        else if (operation(pad, &i)) {
            SAY(1, "Func/oper type is %d\n", i.oper);
            i.lvl = lvl;
            if (i.oper == CONST) {
                memcpy(&eval_list[loc], &i, sizeof(struct item)); 
                SAY(1, "Compiled num %f at loc %d\n", eval_list[loc].val, loc);
fflush(0);
                args[lvl]++;
                loc++;
                SAY(1, "There are %d args avail at lvl %d\n", args[lvl], lvl);
                int needed = oper_list[pending - 1].prec;
                while (pending && oper_list[pending - 1].lvl == lvl 
                        && needed <= args[lvl]) {
                    memcpy(&eval_list[loc], &oper_list[pending - 1], sizeof (struct item));
                    SAY(1, "Compiled oper type %d at loc %d\n", eval_list[loc].oper, loc);
fflush(0);
                    args[lvl] -= (needed - 1);
                    pending--;
                    int l = loc - 1; // most recent item perhaps not used yet
                    while (needed && l >= 0) {
                        if (!eval_list[l].used) {
                            eval_list[l].used = 1;
                        SAY(1, "Marked loc %d as used\n", l);
fflush(0);
                            needed--;
                        }
                        l--;
                    }
                    loc++;
                    needed = oper_list[pending - 1].prec;
                }
            }
            else if (i.oper == OPER_PERCENT || i.oper == OPER_FACT
                     || (i.oper >= OPER_INT && i.oper <= OPER_SCI)) {
                SAY(1, "There are %d args avail at lvl %d\n", args[lvl], lvl);
                if (args[lvl] > 0) {
                    memcpy(&eval_list[loc], &i, sizeof(struct item)); 
                    SAY(1, "Compiled postfix operator %s at loc %d\n", pad, loc);
fflush(0);
                    if (!eval_list[loc - 1].used) {
                        eval_list[loc - 1].used = 1;
                        SAY(1, "Marked loc %d as used\n", loc - 1);
                    }
                    loc++;
                }
                else {
                    SAY(3, "Operator %s has no value on which to work!", pad);
                    return 0;
                }
            }
            else {
                memcpy(&oper_list[pending], &i, sizeof(struct item));
                SAY(1, " - - - Deferring oper type %d at level %d\n", i.oper, i.lvl);
fflush(0);
                pending++;
            }                
        }
        else {
            SAY(3, "'%s' not recognized! Cannot continue.\n", pad);
            return 0;
        }
        exp += r;
        while (*exp && isspace(*exp)) {
            SAY(0, "Skipping '%c'\n", *exp);
            exp++;
        }  
fflush(0);        
       
    }
   
    if (loc > 0) {
        eval_list[loc].oper = OPER_STORE;
        eval_list[loc].prec = 1;
        eval_list[loc - 1].used = 1;
        eval_list[loc].used = 1;
        loc++;
    }

    return loc;
}

double fround(double a) 
{   
    return floor(a + 0.5l);
}

int sig_digits(double a) 
{   char nstr[16];
    int power = floor(log10(fabs(a)));
    double mant = fabs(a)/pow(10.0l, power);
    
    sprintf(nstr, "%1.13lf", mant);
    printf("For num %1.11lf, power is %d, mant is %s\n", 
            a, power, nstr);
    int s, d;
    for (s = d = 0; d < 15; d++) {
        if (nstr[d] != '0' && nstr[d] != '.') {
            s = d;
        }
        //printf("      char '%c', s is %d\n", nstr[d], s);
    }
    printf("   there are %d sig digits\n", s);
    return s;
    
}

void determine_fmt(struct item* a)
{
    int fmt = a->fmt;
    if (fabs(a->val - fround(a->val)) < 0.000000000001l) {
        if (a->fmt >= REAL_NUM) {
            a->fmt = INT_NUM;
        }
    }
   else {
        if (a->fmt < REAL_NUM) {
            a->fmt = REAL_NUM;
        }
    } 
}

void determine_fmt(struct item* a, struct item* b)
{
    int fmt = MAX(a->fmt, b->fmt);
    if (    fabs(a->val) < 100000
        &&  fmt >= REAL_NUM 
        && (fabs(a->val - fround(a->val)) < 0.000000000001l)) 
    {
        fmt = INT_NUM;
        if (a->fmt == BIN_NUM || b->fmt == BIN_NUM) {
            fmt = BIN_NUM;
        }
        else if (a->fmt == HEX_NUM || b->fmt == HEX_NUM) {
            fmt = HEX_NUM;
        }
    } 
    a->fmt = fmt;
}

int evaluate_expression(struct item* eval_list, int items, int result) 
{
    int r, i, offset;
    struct item calc_list[64];
    int num = 0;

    int bitlen = 32;
    int bitmask = 0xFFFFFFFF;

    char scratch_str[256] = {""};

    SAY(1, "Evaluation:\n");
    for(r = 0; r < items; r++) {
        SAY(1, "step %d: ", r);
        if (eval_list[r].oper == CONST) {
            calc_list[num].val    = eval_list[r].val;
            calc_list[num].fmt    = eval_list[r].fmt;
            calc_list[num].prec   = eval_list[r].prec;
            calc_list[num].pow    = eval_list[r].pow;
            calc_list[num].sigpow = eval_list[r].sigpow;
            num++;
            render_item(scratch_str, &eval_list[r]);
            SAY(1, "pushed num %s, calc stack depth is %d\n", scratch_str, num);
        }
        else {
            int sd1, sd2, sd;
            switch(eval_list[r].oper) {
                case OPER_ADD:
                    sd1 = calc_list[num - 1].sigpow;
                    sd2 = calc_list[num - 2].sigpow;
                    SAY(0, "ADD: sigpowers are %d and %d\n", sd1, sd2);
                    calc_list[num - 2].val = calc_list[num - 2].val + calc_list[num - 1].val;
                    determine_fmt(&calc_list[num - 2], &calc_list[num - 1]);
                    calc_list[num - 2].sigpow = MAX(sd1, sd2);
                    calc_list[num - 2].pow = floor(log10(calc_list[num - 2].val));
                    num--; 
                    SAY(1, "added two nums, ");
                    break;
                case OPER_SUB:
                    calc_list[num - 2].val = calc_list[num - 2].val - calc_list[num - 1].val;
                    determine_fmt(&calc_list[num - 2], &calc_list[num - 1]);
                    calc_list[num - 2].sigpow = MAX(calc_list[num - 2].sigpow, 
                                                    calc_list[num - 1].sigpow);
                    calc_list[num - 2].pow = floor(log10(calc_list[num - 2].val));
                    num--;
                    SAY(1, "subtracted one num from another, ");
                    break;
                case OPER_MULT:
                    sd1 = calc_list[num - 1].pow - calc_list[num - 1].sigpow + 1;
                    sd2 = calc_list[num - 2].pow - calc_list[num - 2].sigpow + 1;
                    sd = MIN(sd1, sd2);
                    SAY(0, "Mult: sd1 %d, sd2 %d, sd %d\n", sd1, sd2, sd);
                    calc_list[num - 2].val = calc_list[num - 2].val * calc_list[num - 1].val;
                    determine_fmt(&calc_list[num - 2], &calc_list[num - 1]);                    
                    calc_list[num - 2].pow = floor(log10(calc_list[num - 2].val));
                    calc_list[num - 2].sigpow = calc_list[num - 2].pow + 1 - sd;
                    num--;
                    SAY(1, "multiplied two nums, ");
                    break;
                case OPER_DIV:
                    sd1 = calc_list[num - 1].pow - calc_list[num - 1].sigpow + 1;
                    sd2 = calc_list[num - 2].pow - calc_list[num - 2].sigpow + 1;
                    sd = MIN(sd1, sd2);
                    calc_list[num - 2].val= calc_list[num - 2].val / calc_list[num - 1].val;
                    determine_fmt(&calc_list[num - 2]);
                    calc_list[num - 2].pow = floor(log10(calc_list[num - 2].val));
                    calc_list[num - 2].sigpow = calc_list[num - 2].pow + 1 - sd;
                    num--;
                    SAY(1, "divided two nums, ");
                    break;
                case OPER_MOD:
                    calc_list[num - 2].val= (int)calc_list[num - 2].val % (int)calc_list[num - 1].val;
                    calc_list[num - 2].prec = 0;
                    if (calc_list[num - 2].fmt >= REAL_NUM) { 
                        calc_list[num - 2].fmt = INT_NUM; 
                    } 
                    num--;
                    SAY(1, "integer modulus of two nums, ");
                    break;
                case OPER_REMAINDER:
                    if (calc_list[num - 2].fmt < REAL_NUM && calc_list[num - 1].fmt < REAL_NUM) {
                        calc_list[num - 2].fmt = REAL_NUM;
                    }
                    calc_list[num - 2].val= fmod(calc_list[num - 2].val, calc_list[num - 1].val);
                    determine_fmt(&calc_list[num - 2], &calc_list[num - 1]);
                    num--;
                    SAY(1, "remainder of dividing first num by second, ");
                    break;
                case OPER_ROUND:
                    calc_list[num - 2].val =    calc_list[num - 1].val 
                                             *  fround( calc_list[num - 2].val 
                                                       /calc_list[num - 1].val);
                    determine_fmt(&calc_list[num - 2]);
                    num--;
                    SAY(1, "rounding the first number by second number's factor, ");
                    break;
                case OPER_AND:
                    calc_list[num - 2].val=    (unsigned long int)calc_list[num - 2].val  
                                                & (unsigned long int)calc_list[num - 1].val;
                    SAY(1, "AND-ed bitwise two nums, ");
                    num--;
                    break;
                case OPER_OR:
                    calc_list[num - 2].val=    (unsigned long int)calc_list[num - 2].val  
                                                | (unsigned long int)calc_list[num - 1].val;
                    SAY(1, "OR-ed bitwise two nums, ");
                    num--;
                    break;
                case OPER_XOR:
                    calc_list[num - 2].val=    (unsigned long int)calc_list[num - 2].val  
                                                ^ (unsigned long int)calc_list[num - 1].val;
                    SAY(1, "XOR-ed bitwise two nums, ");
                    num--;
                    break;
                case OPER_NOT:
                    calc_list[num - 1].val = (double)((unsigned long int)bitmask & ~(unsigned long int)calc_list[num - 1].val);
                    SAY(1, "inverted bitwise one num, ");
                    break;
                case OPER_POW:
                    calc_list[num - 2].val= pow(calc_list[num - 2].val, calc_list[num - 1].val);
                    determine_fmt(&calc_list[num - 2]);
                    num--;
                    SAY(1, "raised a number by a power, ");
                    break;
                case OPER_LN:
                    calc_list[num - 1].val= log(calc_list[num - 1].val);
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "natural logarithm of one num, ");
                    break;
                case OPER_LOG10:
                    calc_list[num - 1].val= log10(calc_list[num - 1].val);
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "logarithm of one num, ");
                    break; 
                case OPER_SIGN:
                    if (calc_list[num - 1].val < 0.0l) {
                        calc_list[num - 1].val = -1.0l;
                    }
                    else {
                        calc_list[num - 1].val = 1.0l;
                    }
                    calc_list[num - 1].fmt = INT_NUM;
                    SAY(1, "sign of one num, ");
                    break;
                case OPER_SQRT:
                    calc_list[num - 1].val= sqrt(calc_list[num - 1].val);
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "computed square root of one num, ");
                    break;
                case OPER_DEG2RAD:
                    calc_list[num - 1].val= calc_list[num - 1].val / DEG_PER_RAD;
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "converted degrees to radians, ");
                    break;
                case OPER_RAD2DEG:
                    calc_list[num - 1].val= calc_list[num - 1].val * DEG_PER_RAD;
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "converted radians to degrees, ");
                    break;
/*                case OPER_RADIANS:
                    SAY(1, "(nop, assuming we are already in radians), ");
                    break;
                case OPER_DEGREES:
                    calc_list[num - 1].val= calc_list[num - 1].val * DEG_PER_RAD;
                    calc_list[num - 1].prec = 9; // hackish :-(
                    SAY(1, "converted radians to degrees, ");
                    break; */
                case OPER_SIN:
                    calc_list[num - 1].val
                        = sin(  calc_list[num - 1].val / 
                                (AngleUnitsAreDegrees ? DEG_PER_RAD : 1.0));
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "calculated sin(), ");
                    break;
                case OPER_COS:
                    calc_list[num - 1].val
                        = cos(  calc_list[num - 1].val / 
                                (AngleUnitsAreDegrees ? DEG_PER_RAD : 1.0));
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "calculated cos(), ");
                    break;
                case OPER_TAN:
                    calc_list[num - 1].val
                        = tan(  calc_list[num - 1].val / 
                                (AngleUnitsAreDegrees ? DEG_PER_RAD : 1.0));
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "calculated tan(), ");
                    break;
                case OPER_ARCSIN:
                    calc_list[num - 1].val
                        = asin( calc_list[num - 1].val ) * 
                                (AngleUnitsAreDegrees ? DEG_PER_RAD : 1.0);
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "calculated arcsin(), ");
                    break;
                case OPER_ARCCOS:
                    calc_list[num - 1].val
                        = acos( calc_list[num - 1].val ) * 
                                (AngleUnitsAreDegrees ? DEG_PER_RAD : 1.0);
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "calculated arccos(), ");
                    break;
                case OPER_ARCTAN:
                    calc_list[num - 1].val
                        = atan(  calc_list[num - 1].val ) * 
                                (AngleUnitsAreDegrees ? DEG_PER_RAD : 1.0);
                    determine_fmt(&calc_list[num - 1]);
                    SAY(1, "calculated arctan(), ");
                    break;
                case OPER_PREVRES:
                    offset = (int)calc_list[num - 1].val;
                    i = result - offset;
                    if (i < 0 || i > MAXENTRY) {
                        SAY(4, "Invalid row (%d) referenced!\n", i);
                        return 0;
                    }
                    calc_list[num - 1].val  = ResultList[i].val;
                    calc_list[num - 1].fmt  = ResultList[i].fmt;
                    calc_list[num - 1].prec = ResultList[i].prec;
                    calc_list[num - 1].oper = CONST;
                    SAY(1, "fetched relative (-%d) result# %d, ", offset, i);
                    break;
                case OPER_RESULT:
                    i = (int)calc_list[num - 1].val;
                    if (i < 0 || i > MAXENTRY) {
                        SAY(4, "Invalid row (%d) referenced!\n", i);
                        return 0;
                    }
                    calc_list[num - 1].val  = ResultList[i].val;
                    calc_list[num - 1].fmt  = ResultList[i].fmt;
                    calc_list[num - 1].prec = ResultList[i].prec;
                    calc_list[num - 1].oper = CONST;
                    SAY(1, "fetched specific result# %d, ", i);
                    break;
                case OPER_PERCENT:
                    calc_list[num - 1].val  = calc_list[num - 1].val / 100.0;
                    calc_list[num - 1].fmt  = REAL_NUM;
                    SAY(1, "calculated percentage, ");
                    break;
                case OPER_INT:
                case OPER_HEX:
                case OPER_BIN:
                case OPER_REAL:
                case OPER_SCI:
                    calc_list[num - 1].fmt  = eval_list[r].oper - OPER_INT;
                    SAY(1, "converted format, ");
                    break;
                case OPER_FACT:
                    {
                        int n = (int)floor(calc_list[num - 1].val);
                        if (n >= 0) { 
                            double fact = 1.0;
                            for (i = 2; i <= n; i++) {
                                fact *= (double)i;
                            }
                            calc_list[num - 1].val  = fact;
                            SAY(1, "calculated factorial of %d, ", n);
                            if (n > 15) {
                                calc_list[num - 1].fmt  = REAL_NUM;
                            }
                        }
                        else {
                            SAY(4, "cannot compute negative factorial!\n");
                            return 0;
                        }
                    }
                    break;
                case OPER_STORE:
                    ResultList[result].val     = calc_list[num - 1].val;
                    ResultList[result].prec    = calc_list[num - 1].prec;
                    ResultList[result].oper    = CONST;
                    ResultList[result].fmt     = calc_list[num - 1].fmt;
                    ResultList[result].pow     = calc_list[num - 1].pow;
                    ResultList[result].sigpow  = calc_list[num - 1].sigpow;
                    num--;
                    SAY(1, "popped result\n ");
                    return 1;
                    break;
            }
            scratch_str[0] = '\0';
            render_item(scratch_str, &(calc_list[num - 1]));
            SAY(2, "result is %s (val %lf, fmt %d, pow %d, sigpow %d), stack depth is %d\n", 
                    scratch_str, calc_list[num - 1].val, calc_list[num - 1].fmt,
                    calc_list[num - 1].pow, calc_list[num - 1].sigpow,
                    num);
             
        }
    }
    return 0;
}

void print_expression(struct item* eval_list, int items)
{
    int r;
    char scratch_str[256] = {""};
    SAY(1, "Evaluation List has %d items:\n", items);
    for (r = 0; r < items; r++) {
        if (eval_list[r].oper == CONST) {
            render_item(scratch_str, &eval_list[r]);
            SAY(2, "PUSH CONSTANT %s on CALC STACK", scratch_str);
        }
        else {
            SAY(2, "PERFORM OPERATION ");
            int op = 0;
            while (operation_defs[op].oper >= 0) {
                if (operation_defs[op].oper == eval_list[r].oper) {
                    SAY(2, "%s", operation_defs[op].desc);
                    break;
                }
                op++;
            }
            SAY(2, " on top %d number(s) on CALC STACK", eval_list[r].prec);
        }
        if (eval_list[r].used) {
            SAY(2, "\n");
        }
        else {
            SAY(2, " <<< ITEM UNUSED\n");
        }
    }
   
}

void skip_whitespace(char** p) 
{
    while (**p && (**p == ' ' || **p == '\t')) {
        (*p)++;
    }
}

void preparse_expression(char* raw_expr, char* preparsed_expr, char* note_str)
{
    char* a = raw_expr;
    char* b = preparsed_expr;
    enum {  PREVCHAR_NONE, PREVCHAR_OPER, PREVCHAR_COMMA, PREVCHAR_NUM, 
            PREVCHAR_LPAREN, PREVCHAR_RPAREN, PREVCHAR_SCINUM_E};
    int prevchar_type = PREVCHAR_NONE;
    
    while (*a) {
        if (*a == '\'' || *a == '"') {
            *b++ = 'p';
            *b++ = '[';
            *b++ = (*a == '\'') ? '1' : '2'; // single quote is one back, double quote is two back
            *b++ = ']';
            prevchar_type = PREVCHAR_NUM;
            a++;
        }
        else if (*a == '-') {
            switch (prevchar_type) {
                case PREVCHAR_OPER:
                case PREVCHAR_NONE:
                case PREVCHAR_COMMA:
                case PREVCHAR_LPAREN:
                    if (*(b-1) != ' ' ) *b++ = ' ';
                    *b++ = *a++;    
                    // No trailing space!
                    prevchar_type = PREVCHAR_NUM;
                    break;
                    
                case PREVCHAR_NUM:
                case PREVCHAR_RPAREN:
                    if (*(b-1) != ' ' ) *b++ = ' ';
                    *b++ = *a++;    
                    *b++ = ' '; // Trailing space
                    prevchar_type = PREVCHAR_OPER;
                    break;

                case PREVCHAR_SCINUM_E:
                    *b++ = *a++;    
                    // No trailing space!
                    prevchar_type = PREVCHAR_NUM;
                    break;

                default:
                    SAY(3, "Unknown char type during parsing!");
                    break;
            }
        }
        else if (*a == ';') {
            a++;
            if (strlen(a) < 255) {
                skip_whitespace(&a);
                strcpy(note_str, a);
            }
            break;
        }
        else {
            switch (*a) {
                case ' ':
                case '\t':
                    // don't update prevchar_type
                    break;                    
                case ',':
                    prevchar_type = PREVCHAR_COMMA;
                    break;
                case '(':
                case '[':
                    prevchar_type = PREVCHAR_LPAREN;
                    break;
                case ')':
                case ']':
                    prevchar_type = PREVCHAR_RPAREN;
                    break;                    
                case '+':
                case '*':
                case '/':
                case '^':
                case '&':
                case '|':
                case '~':
                    if (prevchar_type != PREVCHAR_NONE && *(b-1) != ' ' ) {
                        *b++ = ' ';
                    }
                    prevchar_type = PREVCHAR_OPER;
                    break;
                case 'e':
                case 'E':
                    prevchar_type = PREVCHAR_SCINUM_E;
                    break;                    
                default:
                    prevchar_type = PREVCHAR_NUM;
                    break;
                
            }
            *b++ = *a++;    // copy the char
            if (prevchar_type == PREVCHAR_OPER && *(b-1) != ' ') *b++ = ' ';
            if ((b - preparsed_expr) > MAXENTRYLEN - 8) {
                *b = '\0';
                return;
            } 
        }
        *b = '\0'; 
        SAY(0, "Pre-parsed expression in progress: %s\n", preparsed_expr); fflush(0);
    }
    *b = '\0';
}

int closing_paren_offset(char* inexpr)
{
    int offset = -1;    // means "no closing paren
    int level = 0;      // nesting level
    char* p = inexpr;
    
    while (*p) {
        if (*p == '(') level++;
        else if (*p == ')') level--;
        if (!level) {
            offset = p - inexpr;
            if (!offset) offset = -1;
            break;
        }
        p++;
    }
    return offset;
}

int oper_type(int oper)
{
    int op = 0;
    while (operation_defs[op].oper >= 0) {
        if (oper == operation_defs[op].oper) {
            return operation_defs[op].type;
        }
        op++;
    }
    return -1;
}

// This function inserts parentheses around subexpressions that need
// higher precedence, like exponentation and multiplication.
// Note: outexpr should point to a buffer at least 1.5 times len in size
void group_for_precedence(char* inexpr, int len, char* outexpr) {
    struct item si;
    char buf[1024]      = {""};
    char subexpr[1024]  = {""};
    char newexpr[1024]  = {""};
    char token[128]     = {""};
    enum { UNKNOWN_ITEM, NUMBER_ITEM, COMMA_ITEM, SUBEXPR_ITEM, OPER_ITEM, 
           ADDOPER_ITEM, POWOPER_ITEM, FACTOPER_ITEM, CONVERT_ITEM};
    int item = 0;
    int itemtype[64];
    int itempos[64];
    int itemlen[64];
    char* inp = inexpr;
    char* newp = newexpr;
    int ilen;
    int adj;    // current difference between inexpr and newexpr locations
       
    skip_whitespace(&inp);
    while ((inp - inexpr) < len && (ilen = have_item(inp, token))) {
        itemlen[item] = ilen;   // might override below
        SAY(0, "generic item length is %d, char '%c'\n", itemlen[item], *inp); fflush(0);
        int offset = -1;
        if (*inp == '(') {
            offset = closing_paren_offset(inp);
            itemlen[item] = offset + 1;
            itemtype[item] = SUBEXPR_ITEM;
        }
        else if (*inp == ',') {
            itemtype[item] = COMMA_ITEM;
            SAY(1, "It's a comma\n");
        }
        else {
            SAY(0, "Token is '%s'\n", token); fflush(0);
            ilen = usable_number(token, &si);
            SAY(0, "Number string length is %d\n", ilen);
            if (ilen > 0) {
                itemtype[item] = NUMBER_ITEM;
                itemlen[item] = ilen;
            }
            else {
                ilen = operation(token, &si);
                SAY(0, "operation() returned length %d\n", ilen); fflush(0);
                char * p;
                if (ilen) {
                    itemtype[item] = OPER_ITEM;
                    switch(oper_type(si.oper)) {
                        case PUSH_OPER:
                            break;
                        case INFIX_OPER:
                            break;
                        case FUNC_OPER:
                            p = inp;
                            while (*p && *p != '(') p++;
                            offset = closing_paren_offset(p);
                            itemlen[item] = (p - inp) + offset + 1;
                            itemtype[item] = NUMBER_ITEM;
                            break;
                        case REF_OPER:
                            p = inp;
                            while (*p && *p != ']') {
                                SAY(0, "  Checking for ]: '%s'\n", p);
                                p++;
                            }
                            SAY(0, "  Found ] or end: '%s' %d chars away\n", p, (p - inp));
                            itemlen[item] = (p - inp) + 1;
                            itemtype[item] = NUMBER_ITEM;
                            break;
                        case SUFFIX_OPER:
                            itemtype[item] = OPER_UNKNOWN;
                            if (si.oper == OPER_FACT) {
                                itemtype[item] = FACTOPER_ITEM;
                            }
                            else if (si.oper >= OPER_INT && si.oper <= OPER_SCI) {
                                itemtype[item] = CONVERT_ITEM;
                            }
                            break;
                        default:
                            itemtype[item] = OPER_UNKNOWN;
                            break;
                    }
                    if (si.oper == OPER_POW) {
                        itemtype[item] = POWOPER_ITEM;
                    }
                    else if (si.oper == OPER_ADD || si.oper == OPER_SUB) {
                        itemtype[item] = ADDOPER_ITEM;
                    }
                }
                else {
                    itemtype[item] = OPER_UNKNOWN;
                    si.oper = OPER_UNKNOWN;
                }
            }
        }
        itempos[item] = newp - newexpr;
        SAY(1, "Item %d starts at char %d and is %d chars long and type %d\n", 
            item, itempos[item], itemlen[item], itemtype[item]);
        if (itemtype[item] == SUBEXPR_ITEM || oper_type(si.oper) == FUNC_OPER) {
            char* startp = inp; // inexpr + itempos[item];
            while(*startp && *startp != '(') {
                *newp++ = *startp++;
            }
            if (*startp) startp++;
            char* endp = inp + itemlen[item] - 1;
            while (*endp && *endp != ')')   endp--; // chop any whitespace (needed?)
            SAY(0, "endp is pointing to char '%c' (offset %d)\n", *endp, endp - inp);
            int subexpr_len = endp - startp;
            strncpy(buf, startp, subexpr_len);
            buf[subexpr_len] = '\0';
            SAY(1, "Will perform grouping analysis on subexpression '%s'...\n", buf);
            SAY(1, "||||||||||||||||||||||||||||||||||\n");

            // recursive call
            group_for_precedence(buf, subexpr_len, subexpr);

            SAY(1, "||||||||||||||||||||||||||||||||||\n");
            SAY(1, "New version of subexpression is '%s'\n", subexpr); fflush(0);

            inp += itemlen[item];
            
            subexpr_len = strlen(subexpr);
            strcat(newp++, "(");
            strncat(newp, subexpr, subexpr_len);
            newp += subexpr_len;
            strcat(newp++, ")");
            
            itemlen[item] = newp - (newexpr + itempos[item]);    // adjust to new length
        }
        else {
            strncat(newp, inp, itemlen[item]);
            newp += itemlen[item];
            inp += itemlen[item];
        }
        strcat(newp++, " ");
        item++;
        skip_whitespace(&inp);
        SAY(1, "New expression so far: '%s'\n", newexpr); fflush(0);
    }
    int i;
    int prev_oper_type = -1;
    int mr_start = 0;   // "multiplicative run" start
    int mr_end = 0;     // same idea
    char* outp = outexpr;
    *outp = '\0';       // make sure it is an empty string
    int items = item;   // semantic sugar
    int j;
    
    int need_exp_right_paren = 0;
    int exp_start_paren = -1;
    int exp_end_paren = -1;
    int factorial_group_needs_closure = 0;
    for (i = 0; i < items; i++) {
        strncpy(token, newexpr+itempos[i], itemlen[i]);
        token[itemlen[i]] = '\0';
        const char* type_desc[] = {"(unknown)", "number", "comma", "subexpression", "operation", "add or subtract operation", "exponent operation", "postfix operation", "convert operation"};
        SAY(1, "Item %d is a %s, '%s', starting at char %d ('%c')and %d chars long\n", 
                i, type_desc[itemtype[i]], token, itempos[i], *(newexpr+itempos[i]), itemlen[i]);
        
        
        if (i > 0 || i == (items - 1)) {
            if (    itemtype[i] == ADDOPER_ITEM 
                ||  itemtype[i] == COMMA_ITEM
                ||  !strcmp(token, "AS") || !strcmp(token, "as")
                ||  itemtype[i] == CONVERT_ITEM
                ||  i == (items - 1)
            ) {
                mr_end = i - 1;
                if (i == (items - 1)) {
                    mr_end = i;
                }
                if ((mr_start > 0 || mr_end < items - 1) && mr_end - mr_start > 1) {
                    strcat(outp, "("); 
                    SAY(0, "Appending (\n");
                }
                for (j = mr_start; j <= mr_end; j++) {
                    if (j > mr_start) {
                        strcat(outp, " ");
                    }
                    if (    (   j > mr_start
                             || j + 2 < mr_end
                            )
                        &&  (   itemtype[j] == NUMBER_ITEM
                             || itemtype[j] == SUBEXPR_ITEM
                            )
                        &&  itemtype[j+1] == POWOPER_ITEM
                        &&  (   itemtype[j+2] == NUMBER_ITEM
                             || itemtype[j+2] == SUBEXPR_ITEM
                            )
                    ) {
                        strcat(outp, "(");
                        SAY(0, "Appending exponent group left-paren\n", buf);
                    }
                    else if (   j > 0
                             && (   j >= mr_start
                                 || j + 1 <= mr_end
                                )
                            &&  (   itemtype[j] == NUMBER_ITEM
                                 || itemtype[j] == SUBEXPR_ITEM
                                )
                            &&  itemtype[j+1] == FACTOPER_ITEM
                    ) {
                        strcat(outp, "(");
                        SAY(0, "Appending factorial group left-paren\n", buf);
                        factorial_group_needs_closure = 1;
                    }

                    *buf = '\0';
                    strncat(buf, newexpr+itempos[j], itemlen[j]);
                    strcat(outp, buf);
                    SAY(0, "Appending item '%s'\n", buf);
                    if (    (   j < mr_end 
                             || j - 2 > mr_start
                            )
                        && (   itemtype[j-2] == NUMBER_ITEM
                            || itemtype[j-2] == SUBEXPR_ITEM
                           )
                        &&  itemtype[j-1] == POWOPER_ITEM
                        &&  (   itemtype[j] == NUMBER_ITEM
                             || itemtype[j] == SUBEXPR_ITEM
                            )
                        ) {
                        strcat(outp, ")");
                        SAY(0, "Appending exponent group right-paren\n", buf);
                    }
                    else if (   factorial_group_needs_closure
                             && (   j <= mr_end 
                                 || j - 1 >= mr_start
                                )
                             && (   itemtype[j-1] == NUMBER_ITEM
                                 || itemtype[j-1] == SUBEXPR_ITEM
                                )
                             &&  itemtype[j] == FACTOPER_ITEM
                            ) {
                        strcat(outp, ")");
                        factorial_group_needs_closure = 0;
                        SAY(0, "Appending factorial group right-paren\n", buf);
                    }

                }
                if ((mr_start > 0 || mr_end < items - 1) && mr_end - mr_start > 1) {
                    strcat(outp, ")");
                    SAY(0, "Appending )\n");
                }
                if (i < (items - 1)) {
                    if (itemtype[i] != COMMA_ITEM)   strcat(outp, " ");
                    mr_end = i;
                    mr_start = i + 1;
                    *buf = '\0';
                    strncat(buf, newexpr+itempos[i], itemlen[i]);
                    strcat(outp, buf);
                    SAY(0, "Appending item '%s'\n", buf);
                    strcat(outp, " ");
                }
            }
        }

    }
    SAY(1, "Output buffer: %s\n", outexpr);
}

int expr_is_unbalanced(char* expr) {
    char* p = expr;
    int paren_level = 0;
    while (*p) {
        if      (*p == '(') paren_level++;
        else if (*p == ')') paren_level--;

        p++;
    }
    return (paren_level != 0);
}

int calculate_expr(char* expr_input_str, char* expr_fixed_str, int resultnum)
{
    struct item eval_list[MAXITEMS];
    char expr_prepped_str[MAXENTRYLEN] = {0};
    int buf_len = strlen(expr_input_str);
    if (InputMode == RPN_INPUT) {
        strcpy(expr_fixed_str, expr_input_str);
        char* c = expr_fixed_str;
        while (*c && (*c != '=')) c++;
        if (*c == '=')  *c-- = '\0';    // chop off at equal sign
        while( *c == ' ') *c-- = '\0';  // trim whitespace
        int items = build_rpn_expression(expr_fixed_str, eval_list);
        if (items > 0) {
            SAY(2, "\nEvaluation List for: %s\n", expr_fixed_str);
            print_expression(eval_list, items); fflush(0);
            if (evaluate_expression(eval_list, items, resultnum)) { 
                return CALC_SUCCESS;
            }
            else {
                return CALC_ERROR;
            }
        }
        else {
            return CALC_ERROR;
        }
    }
    
    if (expr_is_unbalanced(expr_input_str)) {
        SAY(3, "Expression is unbalanced!\n"); fflush(0);
        return CALC_UNBALANCED;
    }
    preparse_expression(expr_input_str, expr_prepped_str, Note_str[resultnum]);
    if (strlen(expr_prepped_str) > MAXENTRYLEN - 8) {
        SAY(3, "Expression too long!\n"); fflush(0);
        return CALC_EXPRLENGTH;
    }
    group_for_precedence(expr_prepped_str, strlen(expr_prepped_str), 
                            expr_fixed_str);
    int items = build_expression(expr_fixed_str, eval_list); fflush(0);
    if (items) {
        SAY(2, "\nEvaluation List for: %s\n", expr_fixed_str);
        print_expression(eval_list, items); fflush(0);
        if (evaluate_expression(eval_list, items, resultnum)) { 
            return CALC_SUCCESS;
        }
    }
    return CALC_ERROR;
}


#ifdef STANDALONE

int main(int argc, char** argv)
{
    const char* test_expr[99] = {
        "9",
        "3",
        "\" - \'",
        "p(3) - p(2)",
        "5-4", 
        //"r[0] + r(1)",
        "XOR(0xFFFFFFFF, 0xAAFF0055)",
        "0xFFFFFFFF xor 0xAAFF0055",
        "~0xFFFFAA55",
        "pi() / 2.0",
        "12",
        "10 + 7",
        "10.0 - 4.0e0",
        "0x0A&0x07",
        "200*300",
        "20000.5*3.5e5",
        "100/3.0",
        "1 + 2 + 3",
        "1 + (2)",
        "1 + (2 * 3)",
        "pi() / 2.0",
        "6.0 ^(1 - -1)",
        "1.0 * e() + 0.0",
        "rad2deg(deg2rad(180.0) + deg2rad(180.0))",
        "sin(deg2rad(45.0))",
        "cos(deg2rad(45.0))",
        "rad2deg(arctan(3.0 / 5.0))",
        "1 + (2 * (3 + 4))",
        "(3 * 3) + (4 * 4) - (5 * 5)", 
        "1 + ( 43 - (6*7)) + ((2 - 1)*1)",
        "rad2deg(arcsin(1.0/2.0))",
        "sin(deg2rad(45.0))",
        "(20)+(30)",
        "55",
        "(sin(deg2rad(20)))^2 + (cos(deg2rad(20)))^2",
        "1.0+sin(pi/2.0)",
        "1+2^3*4*5+7",
        "1.0+(2^2)",
        "4 + ((5 - 6) * 5)",
        "r[0]- 6.0 / 0x0A ^ sin(pi/2.0) + p[1] + (ln(1) + pi) - r[22] * (5/4)",
        "2 * 3 + 4 * 5",
        "1 - 2 * 3 / (2+2) - 5",
        "1 + 2 - 3 + 4 - 5",
        "5 + 6 + 7 + 8*9",
        "5, 6, 7, 8*9",
        "1 + 2 * (1 + 2 * 2)",
        "3 * (4 + 5 * 5) + 2 * (1 + 2 * 2)",
        "1+2^2+7",
        "1+2^3*4*5+7",
        ""
        };
    char** p = (char**)test_expr;
    char scratch_str[256]   = {""};
    char preparsed_expr[256]    = {""};
    char grouped_expr[256]  = {""};

    struct item eval_list[256];
    int bitlen = 32;
    int bitmask = 0xFFFFFFFF;

    while (**p) {
        preparse_expression(*p, preparsed_expr, NULL); // NULL is only safe if expressions don't have notes
        SAY(1, "Prepared expression is '%s'\n", preparsed_expr); fflush(0);
        group_for_precedence(preparsed_expr, strlen(preparsed_expr), grouped_expr);
        SAY(2, "Grouped expression is '%s'\n", grouped_expr); fflush(0);
        int items = build_expression(grouped_expr, eval_list); fflush(0);
        if (items) {
            print_expression(eval_list, items); fflush(0);
            
            // Evaluation:    
            if (evaluate_expression(eval_list, items, Result)) {
                
                render_item(scratch_str, &ResultList[Result]);
                SAY(3, "Original expression:\t%s\n", *p);
#ifdef THINKOUTLOUD
                SAY(4, "Evaluation:\t\t%s = %s\n\n", grouped_expr, scratch_str); 
#else                
                printf("%s = %s\n", grouped_expr, scratch_str); fflush(0);
#endif
                
#if 1
                printf("Press enter to continue...\n"); fflush(0);
                getchar();
#endif
                Result++;
            }
        }
        p++;
    }
    return 0;
}
#endif
