// flume.cpp  -- Fast, Light Utilitarian Math Environment

#define Version_str "0.9.2" // 2010-12-27

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "math_expression.h"

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/Fl_File_Chooser.H>


#ifndef MAX
#define MAX(n1, n2) (((n1) > (n2)) ? (n1) : (n2))
#endif

#define PATH_TO_FLUME_HELP "/usr/share/doc/tc/flume_help.htm"

// RPN or STD input
enum {STD_INPUT, RPN_INPUT};
extern int InputMode;

const char About_text[] = 
"Flume version %s\n"
"copyright 2010 by Michael A. Losh\n"
"\n"
"Flume is a fast and light utilitarian math environment.\n"
"\n"
"Flume is free software: you can redistribute it and/or\n"
"modify it under the terms of the GNU General Public License\n"
"as published by the Free Software Foundation, version 3.\n"
"\n"
"Flume is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
"See http://www.gnu.org/licenses/ for more details.";

const char Hint_text[] = 
"Hints and Quick Reference\n\n"
"Flume recalculates all rows each time you press the Enter key.\n"
"NOTE: function and operator spellings are case-INsensitive.\n"
"\n"
"ARITHMETIC: + - * (multiply) / (divide), ^ for exponents\n"
"   MOD (integer modulus)  REMAINDER(n1, n2) for real-value remainders\n"
"   SQRT (square root)\n"
"BITWISE LOGIC: & or AND  | or OR  XOR  ~ (inverse)\n"
"TRIG: SIN COS TAN ARCSIN ARCCOS ARCTAN\n"
"ANGLE CONVERSION: DEG2RAD RAD2DEG\n"
"PI and e (lower-case only) are defined constants\n"
"RECALLING PREVIOUS RESULTS:\n"
"   ' (single-quote) or p[1] recalls previous result\n"
"   \" or p[2] recalls second-most previous result, etc.\n"
"   r[n] recalls the result from row n (note square brackets)\n"
"PERCENTAGE: %% divides by 100.0, OF is an alias for *, (e.g. 80%% OF 9.95)\n" 
"\n"
"When you save, all the current results are saved, but when you re-open\n"
"the file, all rows will be automatically recalculated.\n"
;  

char Title_str[128] = "";
//char ExprLabel_str[MAXENTRY] = "  0:   Expression";
char RowLabels_str[(MAXENTRY+1)*4] = "";

double n0, n1;

int Base = 10;

Fl_Window * MainWin_p = NULL;
Fl_Multiline_Output* RowLabels_p = NULL;
Fl_Multiline_Input* WorkArea_p = NULL;
Fl_Button* OpenBtn_p = NULL;
Fl_Button* SaveBtn_p = NULL;
Fl_Button* ClearBtn_p = NULL;
Fl_Button* QuitBtn_p = NULL;
Fl_Button* HintBtn_p = NULL;
Fl_Button* AboutBtn_p = NULL;
Fl_Button* HelpBtn_p = NULL;
Fl_Round_Button* RadAngleBtn_p = NULL;
Fl_Round_Button* DegAngleBtn_p = NULL;
Fl_Check_Button* RpnModeChk_p = NULL;
Fl_Check_Button* TrackSigChk_p = NULL;

char ExprEntry_str[MAXENTRY][MAXENTRYLEN];
char ExprEntryDispPos[MAXENTRY];

char CurrentFileSpec_str[MAXENTRY];

//int SelEntry = 0;  
int Unsaved = 0;
int Running = 1;

double fround(double a);

void update_main_win_title(void) 
{
    int havefile = (*CurrentFileSpec_str != '\0'); 
    
    sprintf(Title_str, "%s%s%sFlume %s %s", 
            Unsaved ? "* " : "",
            havefile ? CurrentFileSpec_str : "",
            havefile ? " - " : "",
            Version_str,
            havefile ? "" : "- Fast Light Utilitarian Math Environment");
    
    if (MainWin_p)   MainWin_p->label(Title_str);
}

void update_results_display(void)
{ 
    char entry_str[1024] = "\0";
    char result_str[MAXENTRYLEN] = "\0";
    char disp_str[MAXENTRY*1024] = "\0";
    int pos = 0;

    char fmt[32]; 
    char val_str[32]; 
    int i = 0;
    ExprEntryDispPos[0] = 0;
    
    for (i = 0; i < Result; i++) {
        render_item(result_str, &ResultList[i]);
        sprintf(entry_str, "%s = %s", ExprEntry_str[i], result_str);
        if (strlen(Note_str[i])) {
            strcat(entry_str, " ; ");
            strcat(entry_str, Note_str[i]);
        }
        strcat(entry_str, "\n");
        pos += strlen(entry_str);
        ExprEntryDispPos[i+1] = pos;
        strcat(disp_str, entry_str);
    }
//    printf("disp_str is now '%s'\n", disp_str); fflush(0);
    WorkArea_p->value(disp_str);
    update_main_win_title();
}

void clear_content(void) 
{
    int i;
    for(i = 0; i < MAXENTRY; i++) {
        ExprEntry_str[i][0] = '\0';
        Note_str[i][0] = '\0';
    }
    Result = 0;
    strcpy(CurrentFileSpec_str, "");
    if (WorkArea_p) {
        WorkArea_p->position(0,0);
        WorkArea_p->take_focus();
    }
}

int load_content(char* filespec_str)
{
    char* selected_filespec_str = NULL;
    if (filespec_str && *filespec_str) {
        selected_filespec_str = filespec_str;
    }
    else {
        selected_filespec_str = fl_file_chooser(
                                  "Select Flume workspace content file", 
                                  "Flume Files(*.flume)\tFlume Files(*.ume)\tText files (*.txt)\tAll Files (*)", 
                                  NULL);
    }
    if (!selected_filespec_str) {
        return 0;
    }
    
    SAY(2, "LOADING FILE '%s'\n", selected_filespec_str);
    int lines = 0;
    FILE* f =  fopen(selected_filespec_str, "r");
    if (f) {
        char fileline_str[MAXENTRYLEN];
        char content_str[(MAXENTRY+1)*MAXENTRYLEN] = "";
        while (lines < MAXENTRY && fgets(fileline_str, MAXENTRYLEN, f)){
            SAY(0, "File line is %d chars long\n", strlen(fileline_str)); fflush(0);
            if (strlen(fileline_str) > 1) {
                SAY(0, "Concatenating '%s'\n", fileline_str); fflush(0);
                strcat(content_str, fileline_str);
                if (    strlen(fileline_str) == (MAXENTRYLEN-1) 
                    &&  fileline_str[strlen(fileline_str)-1] != '\n') 
                {
                    strcat(content_str, "\n");
                    fl_alert("Line in file too long!  It will be split.\n");
                }
                lines++;
            }
        }
        if (!feof(f)) {
            SAY(3, "Row limit reached, extra file content ignored!\n");
        }
        fclose(f);
        SAY(0, "File closed\n"); fflush(0);
        strcpy(CurrentFileSpec_str, selected_filespec_str);
        char* p = content_str + strlen(content_str) - 1;
        if (*p == '\n') {
            *p = '\0';  // clip final blank newline
        } 
        if (WorkArea_p) {
            WorkArea_p->value(content_str);
            WorkArea_p->do_callback();
            Unsaved = 0;
            update_main_win_title();
        }
        return 1;
    }
    else {
        return 0;
    }
}

void open_callback(Fl_Widget* w)
{
    int allowed = 1;
    if (*CurrentFileSpec_str && Unsaved) {
        allowed = fl_choice("    Opening a new file now will discard unsaved work     ", 
                            "  Cancel open  ", "  Discard and open a new file  ", NULL);
    }
    
    if (allowed) {
        load_content(NULL);
    }
}

void save_callback(Fl_Widget* w)
{
    char* selected_filespec_str = NULL;
    selected_filespec_str = fl_file_chooser(
                              "Save Flume workspace content file to...", 
                              "Flume Files(*.flume)\tFlume Files(*.ume)\tText files (*.txt)\tAll Files (*)", 
                              CurrentFileSpec_str);
    if (!selected_filespec_str || !*selected_filespec_str) {
        return;
    }
    FILE* f = fopen(selected_filespec_str, "w");
    if (f) {
        fwrite(WorkArea_p->value(), 1, WorkArea_p->size(), f);
        fclose(f);
        strcpy(CurrentFileSpec_str, selected_filespec_str);
        Unsaved = 0;
        SaveBtn_p->deactivate();
        update_main_win_title();
    }
    else {
        fl_alert("File not writeable!\n");
    }
}

void clear_callback(Fl_Widget* w)
{
    int allowed = 1;
    if (Unsaved) {
        allowed = fl_choice("Clearing will discard unsaved work", 
                            "  Cancel clear  ", "  Discard unsaved work  ", NULL);
    }
    
    if (allowed) {
        clear_content();
        Unsaved = 0;
        *CurrentFileSpec_str = '\0';
        SaveBtn_p->deactivate();
        update_results_display();
    } 
}

void workarea_callback(Fl_Widget* w)
{
    struct item eval_list[MAXITEMS];
    char disp_str[MAXENTRY*1024] = "\0";
    char expr_input_str[MAXENTRYLEN] = {0};
    char expr_prepped_str[MAXENTRYLEN] = {0};
    Result = 0;
    strcpy(disp_str, WorkArea_p->value());

    //printf("buf content is '%s'\n", disp_str); fflush(0);
    //printf("Result is %d\n", Result);
    const char * ep = disp_str;
    const char * p = ep;
    int disp_len = strlen(disp_str);
    
    while (Result < MAXENTRY && *ep && (ep - disp_str) < disp_len) {
        while (*p && *p != '\n' /*&& *p != '='*/) p++;
        int len = p - ep;
        if (len > (MAXENTRYLEN - 1)) {
            fl_alert("Entry is too long! (%d chars)", len);
            WorkArea_p->position(p - disp_str);
            return;
        }
        
        strncpy(expr_input_str, ep, len);
        expr_input_str[len] = '\0';
        SAY(0, "Expression entry %d is '%s'\n", Result, expr_input_str); fflush(0);
        
        int rc = calculate_expr(expr_input_str, ExprEntry_str[Result], Result);
        if (rc == CALC_SUCCESS) {
            Result++;
        }
        else {
            if (rc == CALC_UNBALANCED) {
                fl_alert("Expression on row %d is unbalanced!\nCheck parentheses carefully.", Result);
            }
            else if (rc == CALC_ERROR) {
                fl_alert("Expression on row %d cannot be calculated!\nCheck expression carefully.", Result);
            }
            else if (rc == CALC_EXPRLENGTH) {
                fl_alert("Expression on row %d too long!\nBreak into smaller parts.", Result);
            }
            WorkArea_p->position(p - disp_str);
            return;
        }
        //printf("*p is '%c', char # %d in buffer\n", *p, p - disp_str); fflush(0);
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        ep = p;
        //printf("(after) *p is '%c', char # %d in buffer\n", *p, p - disp_str); fflush(0);
        //printf("*ep is '%c', char # %d in buffer\n", *ep, ep - disp_str); fflush(0);
        Unsaved = 1;
        SaveBtn_p->activate();

    }
    if (p - disp_str < disp_len) {
        fl_alert("Row limit reached,\nremaining %d characters will be ignored!", 
                    disp_len - (p - disp_str));
    }
    
    update_results_display();
}

void quit_callback(Fl_Widget* w)
{
    int allowed = 1;
//    printf("Current filespec is %s, unsaved is %d\n", CurrentFileSpec_str, Unsaved);
    if (*CurrentFileSpec_str && Unsaved) {
        allowed = fl_choice("Quitting now will discard unsaved work", 
                            "  Cancel quit  ", "  Discard and quit  ", NULL);
    }
    
    if (allowed) {
        Running = 0;
    }
}

void hint_callback(Fl_Widget* w)
{
    fl_message(Hint_text);
}

void help_callback(Fl_Widget* w)
{
    Fl_Help_Dialog hd;
    
    hd.load(PATH_TO_FLUME_HELP);
    hd.textsize(14);
    hd.show();
    while (hd.visible()) {
        Fl::wait(1);
    } 
}

void about_callback(Fl_Widget* w)
{
    fl_message(About_text, Version_str);
}

void radians_callback(Fl_Widget* w)
{
    DegAngleBtn_p->value(0);
    AngleUnitsAreDegrees = 0;
    workarea_callback((Fl_Widget*)WorkArea_p);
}

void degrees_callback(Fl_Widget* w)
{
    RadAngleBtn_p->value(0);
    AngleUnitsAreDegrees = 1;
    workarea_callback((Fl_Widget*)WorkArea_p);
}

void rpn_callback(Fl_Widget* w) 
{
    InputMode = (RpnModeChk_p->value() == 1);
    workarea_callback((Fl_Widget*)WorkArea_p);
}

void track_sig_callback(Fl_Widget* w) 
{
    TrackSignificance = TrackSigChk_p->value();
    workarea_callback((Fl_Widget*)WorkArea_p);
}

int sig_digits(double a);

int main(int argc, char **argv) {
    // Check command-line options
    char *p;
    int a = 1;
    Fl::args(argc, argv, a);
    Fl::scheme(NULL);  // NULL causes libr. to look up style in .Xdefaults

    memset(ExprEntry_str, '\0', sizeof(ExprEntry_str));
    clear_content();

/*    
    printf("fround(0.0) is %1.2lf\n", fround(0.0));
    printf("fround(0.1) is %1.2lf\n", fround(0.1));
    printf("fround(0.49) is %1.2lf\n", fround(0.49));
    printf("fround(0.50) is %1.2lf\n", fround(0.50));
    printf("fround(0.51) is %1.2lf\n", fround(0.51));
    printf("fround(0.9) is %1.2lf\n", fround(0.9));
    printf("fround(1.0) is %1.2lf\n", fround(1.0));
    printf("fround(1.1) is %1.2lf\n", fround(1.1));
    printf("fround(-0.1) is %1.2lf\n", fround(-0.1));
    printf("fround(-0.4) is %1.2lf\n", fround(-0.4));
    printf("fround(-0.5) is %1.2lf\n", fround(-0.5));
    printf("fround(-0.51) is %1.2lf\n", fround(-0.51));
    printf("fround(-0.9) is %1.2lf\n", fround(-0.9));
    printf("fround(-1.0) is %1.2lf\n", fround(-1.0));
    printf("fround(-1.1) is %1.2lf\n", fround(-1.1));
    printf("fround(-1.4) is %1.2lf\n", fround(-1.4));
    printf("fround(-1.5) is %1.2lf\n", fround(-1.5));
    printf("fround(-1.6) is %1.2lf\n", fround(-1.6));
    printf("fround(-1.9) is %1.2lf\n", fround(-1.9));
    printf("\n");
    
    printf("123.34 has %d significant digits\n", sig_digits(123.45));
    printf("7 has %d significant digits\n", sig_digits(7));
    printf("654.00001 has %d significant digits\n", sig_digits(654.00001));
    printf("-123.34 has %d significant digits\n", sig_digits(-123.45));
    printf("-7 has %d significant digits\n", sig_digits(-7));
    printf("-654.00001 has %d significant digits\n", sig_digits(-654.00001));
    printf("1.234e10 has %d significant digits\n", sig_digits(1.234e10));
    printf("0.0001200560000 has %d significant digits\n", sig_digits(0.0001200560000));
    printf("-1.20056e-4 has %d significant digits\n", sig_digits(-1.20056e-4));

    //return 0;
*/    
    char num_str[8];
    int i;
    for(i = 0; i < MAXENTRY; i++) {
        sprintf(num_str, "%d\n", i);
        strcat(RowLabels_str, num_str);
    }

    //printf("argc is %d\n", argc); fflush(0);
    char cmdline_expr[MAXENTRYLEN] = "";
    char expr_prepped_str[MAXENTRYLEN] = "";
    while (a < argc) {
        p = argv[a];
        if (*p == '-' && *(p+1) == '-') {
            // OK, it's some kind of option
            char option = *(p+2);
            if (option == 't') {
                // Change terseness/verbosity threshold
                if (*(p+3)) {
                    SayThreshold = *(p+3) - '0';
                    SAY(1, "Terseness level is %d\n", SayThreshold); 
                }
                else if (a+1 < argc) {
                    a++;
                    p = argv[a];
                    SayThreshold = *p - '0';
                    SAY(1, "Terseness level is %d\n", SayThreshold); 
                }
            }
            else if (option == 'd') {
                // Change angle encoding to degrees 
                AngleUnitsAreDegrees = 1;
                SAY(1, "Angles expressed as degrees\n"); 
            }
            else if (option == 'r') {
                // Change angle encoding to radians 
                AngleUnitsAreDegrees = 0;
                SAY(1, "Angles expressed as radians\n"); 
            }
            else if (option == 'p') {
                // Switch to Reverse Polish Notation (RPN) entry syntax
                InputMode = RPN_INPUT;
                SAY(1, "Input syntanx mode is RPN (Reverse Polish Notation)\n"); fflush(0);
            }
            else if (option == 'v') {
                // Print name and version 
                update_main_win_title();
                printf("%s\n", Title_str); fflush(0);
            }
            else if (option == 'f') {
                // Pre-load a specific workspace content file 
                char filespec_str[MAXENTRYLEN] = "";
                if (*(p+3)) {
                    strcpy (CurrentFileSpec_str, (p+3));
                }
                else {
                    a++;
                    if (*argv[a]) {
                        strcpy (CurrentFileSpec_str, argv[a]);
                    }
                }
                if (!*CurrentFileSpec_str) {
                    fl_alert("File not specified!\n");
                }
            }
            a++;
        }
        else {
            // assume the remainder of command-line is math expression
            while (a < argc) {
                strncat(cmdline_expr, argv[a], 32);
                strcat(cmdline_expr, " ");
                a++;
            }
            SAY(1, "Commandline expression is '%s'\n", cmdline_expr);
            if (strlen(cmdline_expr)) {
                int rc = calculate_expr(cmdline_expr, ExprEntry_str[Result], Result);
                if (rc == CALC_SUCCESS) {
                    char result_str[64];
                    render_item(result_str, &ResultList[Result]);
                    printf("%s = %s\n", ExprEntry_str[Result], result_str);
                    return 0;
                }
                else {
                    if (rc == CALC_UNBALANCED) {
                        printf("Expression is unbalanced! Check parentheses carefully.\n");
                    }
                    else if (rc == CALC_ERROR) {
                        printf("Expression cannot be calculated!\n");
                    }
                    return 1;
                }
            }
        }
    }
    //fl_register_images();
    update_main_win_title();
    MainWin_p = new Fl_Window(560,352, Title_str);

    MainWin_p->begin();
    
    Fl_Scroll expr_win_scroll(6, 12, 546, 258);
    expr_win_scroll.begin();    
    Fl_Pack expr_win_group(6, 12, 546, 1024);
    expr_win_group.type(FL_HORIZONTAL);
    expr_win_group.begin();
  
    RowLabels_p = new Fl_Multiline_Output(6, 12,  30, 1024 );
    RowLabels_p->box(FL_FLAT_BOX);
    RowLabels_p->color(FL_BACKGROUND_COLOR);
    RowLabels_p->align(FL_ALIGN_RIGHT);
    RowLabels_p->value(RowLabels_str);
    
    WorkArea_p = new Fl_Multiline_Input(40, 10,  500, 1024 );
    WorkArea_p->box(FL_FLAT_BOX);
    WorkArea_p->label("");
    WorkArea_p->when(FL_WHEN_ENTER_KEY);
    WorkArea_p->callback(workarea_callback);
    WorkArea_p->take_focus();
    update_results_display();
    WorkArea_p->position(0,0);
    expr_win_group.end();
    expr_win_scroll.end();
    
    QuitBtn_p =  new Fl_Button(30, 280, 60, 30, "Quit");
    QuitBtn_p->callback(quit_callback);
    QuitBtn_p->shortcut(FL_Escape);
        
    OpenBtn_p =  new Fl_Button(120, 280, 60, 30, "Open...");
    OpenBtn_p->callback(open_callback);
    OpenBtn_p->shortcut(FL_CTRL + 'o');
        
    SaveBtn_p =  new Fl_Button(210, 280, 60, 30, "Save...");
    SaveBtn_p->callback(save_callback);
    SaveBtn_p->deactivate();
    SaveBtn_p->shortcut(FL_CTRL + 's');
        
    ClearBtn_p =  new Fl_Button(300, 280, 60, 30, "Clear");
    ClearBtn_p->callback(clear_callback);
    ClearBtn_p->shortcut(FL_CTRL + 'c');

    HintBtn_p =  new Fl_Button(420, 280, 30, 30, "!");
    HintBtn_p->callback(hint_callback);
    HintBtn_p->tooltip("Hints");
    HintBtn_p->shortcut(FL_CTRL + 'i');

    AboutBtn_p =  new Fl_Button(460, 280, 30, 30, "a");
    AboutBtn_p->callback(about_callback);
    AboutBtn_p->tooltip("About Flume");
    AboutBtn_p->shortcut(FL_CTRL + 'a');

    HelpBtn_p =  new Fl_Button(500, 280, 30, 30, "?");
    HelpBtn_p->callback(help_callback);
    HelpBtn_p->tooltip("Help (F1)");
    HelpBtn_p->shortcut(FL_F + 1);

    Fl_Box* pAngleLabel = new Fl_Box(20, 325, 40, 12, "Angles:");
    pAngleLabel->box(FL_FLAT_BOX);
    RadAngleBtn_p = new Fl_Round_Button(72, 324, 80, 16, "Radians");
    RadAngleBtn_p->value(AngleUnitsAreDegrees ? 0 : 1);
    RadAngleBtn_p->callback(radians_callback);

    DegAngleBtn_p = new Fl_Round_Button(152, 324, 80, 16, "Degrees");
    DegAngleBtn_p->value(AngleUnitsAreDegrees ? 1 : 0);
    DegAngleBtn_p->callback(degrees_callback);
    
    RpnModeChk_p = new Fl_Check_Button(280, 324, 96, 16, "RPN mode");
    RpnModeChk_p->value((InputMode == RPN_INPUT) ? 1 : 0);
    RpnModeChk_p->callback(rpn_callback);
    
    TrackSigChk_p = new Fl_Check_Button(392, 324, 140, 16, "Track significance");
    TrackSigChk_p->value(TrackSignificance);
    TrackSigChk_p->callback(track_sig_callback);
    
    MainWin_p->end();
    
    if (*CurrentFileSpec_str) {
        if (!load_content(CurrentFileSpec_str)) {
            fl_alert("File not loadable!\n");
        }
    }
    
    MainWin_p->show(argc, argv);
    Running = 1;
    while(Running && Fl::wait());
    return 0;
}

