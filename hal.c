#if 0

COMPILE PRINTF getenv isatty tcgetattr tcsetattr malloc realloc strcasecmp \
			  atexit memmove strdup strncpy memset memcmp fgetc fgets strcmp fopen \
			  ipowui strchr feof fclose fstream sscanf fstream itobin nwrite \
			  nread _snprintf MLVALIST
DUMMYHEADER
MALLOC brk
#GLOBALS

OPTFLAG -Os
STRIPFLAG
#FULLDEBUG

return
#endif

#define error(exp,number,fmt, ...) \
	{ fprintf(stderr,"\x1b[1;5;31m" fmt "\x1b[0m" __VA_OPT__(,) __VA_ARGS__);return 0; }



#include "linenoise/linenoise.c"
#include "libcalc.c"


/* since this is sort of retro,
 I'm eager to use some retro programming style.
 */

// todo: enter "programs" with linenumbers.
//  enter functions also in more lines.
//  ( 3*5 RET +7 RET ..)
// nest functions
//
// "config" system: output, base, ..


typedef struct {
	char* cmd;
	void (*callback)(void);
} _cmd;


void quit(){
	printf("Quit\n");
	exit(0);
}

void help(){
	printsl("\nHelp.\n"
			"h - help\n"
			"q - quit\n"
			);
}

/*
_cmd _commands[] = {
	{"q",&quit},
	{"h",&_help},
	{0,0}
};
*/

typedef struct {
	char* cmd;
	void (*label)(void);
} cmd;



int main(int argc,char**argv){

	static int erg;
	static char *l;
	static int b;
	static expression *exp[32];
	static int cf = 0;
	static int lf = 0;

	argc ? prints(AC_LGREEN"HAL 300 rev.1\nREADY:\n(type help for help)\n\n")  :0;
	exp[0] = exp[0] ? exp[0] 
	:new_exp ();

	goto HOP;

HOP: // damned gcc
	l = 0;

static cmd commands[] = {
	{"q",quit},
	{"quit",quit},
	{"h",help},
	{"help",help},
	{0,0}
};

while( argc&&main(0,0) );
// replace ret of main with _RET
/*
asm volatile("pop %%rax\n"
		".global _RET\n"
		"_RET:\n"
		"leaq (_RET),%%rax\n"
		"push %%rax\n" ::: "rax");
(segfault with optimizing) */

// ********************************** LOOP //
LOOP:
free(l);
l= linenoise("> ");
 *l? linenoiseHistoryAdd(l):0;

	for ( cmd *c = commands; c->cmd; c++ ){
		if ( strcmp(l,c->cmd) == 0 ){
			printf("cmd\n");
			c->label();
			//goto *(c->label);
			goto LOOP;
		}
	}

if ( *l == 'f' ){
	if ( *(l+1) == '=' ){
		lf++;
		cf = lf;
		exp[cf] = new_exp();
		printf("STORE: f%d\n",cf);
		l+=2;
	}
}

printf("l: %s\ncf %d\n",l,cf);
parse(l,exp[cf]);
printf("1\n");

erg= calculate( exp[cf] );
printf(AC_LCYAN"\n%d\n0%o\n0x%x\n%b\n"AC_LGREEN, erg,erg,erg,erg);

free(l);	
//asm("retq");
//goto LOOP;
return(1);
// childish laughter...
// how not to loop - once.
}
