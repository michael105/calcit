#ifndef MLIB
#include <stdio.h>
#include <string.h>
//#include <math.h>
#endif

// An expression parser and calculator with (in theory) unlimited brackets.
// Brackets are only limited by the stack.
//
// The original intent did had been to write something
// like a mathematical expression parser for configuration files.
//
// parse some formulas once, while reading the configuration;
// and insert different variables later, at runtime.
//
// this well. would work out, much more performant than
// an embedded scripting language.
// 
// did publish this anyway, albite not finished.
// would, however work. 
// And I wouldn't have thought I'd need to. however.
//
// just now I'm about to clean and document the code a bit.
//
// originally I did plan to implement a mouse driver,
// which should be able to do dynamic accelerations,
// or accelerate or translate key and button combinations
// with mouse movements.
// Therefore a low latency has been my main concern.
//


//TODO: rewrite for double ...

//TODO: variables: array[] of refs to the values in struct expression
//-> müssen nicht bei jedem calculate-aufruf neu zugewiesen werden.
// 
//
//
// persistent:
// assign variables. A,B,..X
// assign functions. f1,f2,..
// calculate f: f1(X=3,Y=1)
//
// interactive persistent mode
// store parsed expressions to disk
// load parsed expressions
// multiple expressions, separated by ;
//
//
//
// -- option:
//  rewrite this as a compiler.
//  funnily, should be easy.
//  needs just to replace the operators with a write of the according 
//  opcodes to an executable string.
//  variables, with x64, there are plenty. just the registers.
//
// muss mir mal assembly von gcc ansehen - abder ich hat bisher immer den eindruck,
// da kommt eher umstaendliches bei raus.

#define STANDALONE
#define DEBUG

#if 1
#ifdef DEBUG
#define dbgf(...) printf(__VA_ARGS__)
#else
#define dbgf(...)
#endif
#endif
#define dbgf2(...)

#define TOKENEND -0xFFFF
// number of preallocated tokens 
#define PREALLOC 32

// executed on errors
#define error(exp,number,fmt, ...) \
	{ fprintf(stderr,fmt __VA_OPT__(,) __VA_ARGS__);\
	exit(number); }

#if 0
#define error(exp,number,fmt, ...) \
	{ snprintf(exp->errormsg,32,fmt __VA_OPT__(,) __VA_ARGS__);\
	exp->error = number;\
	return(exp); }
#endif



typedef struct _expression { 
	char errormsg[32];
	int var[26];
	int numtoken;
	int bracketflag;
	int flag;
	int bracketcount;
	int error;
	int* tokenend;
	int* p;
	int* token;
} expression;

int calculate_loop(expression* exp, int cv);


// parsing

// append a token, allocate memory when needed
int add_token( expression* exp, int i ){
	*(exp->p) = i;
	exp->p++;
	exp->numtoken++;
	
	if ( exp->p == exp->tokenend ){
		exp->token = realloc( exp->token, (exp->tokenend - exp->token) + PREALLOC );
		if ( ! exp->token ){
			error(exp,2, "Unable to allocate memory\n" );
		}
		dbgf("allocate: old: %x %d", exp->tokenend, (exp->tokenend - exp->token) ); 
		exp->tokenend = exp->token + (exp->tokenend - exp->token) + PREALLOC; 
		dbgf("  new: %x\n",exp->tokenend );
	}
	return(0);
}

const static char* op_list = "()+-/*%^&|X~";
#define BROPEN -1
#define BRCLOSE -2
#define PLUS -3
#define MINUS -4
#define MULT -6


const char* next_token(expression *exp, const char *c, const char *str ){
		// strip spaces
		if ( *c==32 ){
			while ( *c == 32 )
				c++;
		}
		
		if ( *c == 0 )
			return(c);

		// numbers
		if ( ( *c >='0' ) && ( *c <='9' ) ){
			int i = 0; 
			do {
				i = i*10 + ( *c - '0' );
				c++;
			} while ( ( *c >='0' ) && ( *c <='9' ) );

			if ( exp->flag ){
				add_token(exp,MULT);
			}

			add_token(exp,i);
			dbgf2("i: %d\n",i);
			exp->flag=1;
			return(c);
		}

		// -, with preceding operator - escape with brackets
		if ( *c=='-' && ( exp->p > exp->token ) && ( *(exp->p-1) < -2 ) && ( *(exp->p-1) > -0xFF ) ){
			exp->flag=1;
			add_token( exp, BROPEN ); // (
			add_token( exp, MINUS ); // -
			c++;
			c = next_token( exp, c, str );
			add_token( exp, BRCLOSE ); // )
			return(c);
		}

		// variables
		if ( ( *c >= 'a' ) && ( *c <= 'z' ) ){
			if ( exp->flag ){
				add_token(exp,MULT);
			}
			add_token(exp, (*c - 'a' ) | INT_MIN ); // set leftmost bit
			exp->flag=1;
		} else {
			// operator
			const char *p_op = op_list;

			while( *p_op != *c ){
				p_op ++;
				if ( *p_op == 0 ){
					error(exp,1, "Syntaxerror at pos %d, char: %c\n", c-str+1, *c );
				}
			}

			if ( *c == ')' ){
				exp->bracketcount--;
				exp->flag=1;
				if ( exp->bracketcount < 0 ){
					error( exp,3, "Unmatched closing bracket at pos %d\n", c-str+1 );
				}
			} else if ( *c == '(' ){
				if ( exp->flag ){
					add_token(exp,MULT);
				}
				exp->bracketcount++;
				exp->flag=0;
			} else {
				exp->flag = 0;
			}

			add_token( exp, (op_list - p_op ) -1 ); // operators are negative tokens 
			// numbers aren't negativ, 
			// negativ values are prepended with the operator '-' 
		}


	return(++c);
}

// parse the expression str.
// returns the "compiled" expression,
expression* parse(const char*str, expression* exp){
	exp->numtoken = 0;
	exp->flag = 0;
	exp->bracketcount = 0;
	exp->token = malloc( sizeof(int) * PREALLOC );
	exp->tokenend = exp->token + PREALLOC;
	exp->p = exp->token;

	const char *c = str;
	while (*c && ( exp->error == 0 )){
		c = next_token( exp, c, str );
	}

	if ( exp->error )
		return(exp);
	
	if ( exp->bracketcount ){
		error( exp, 4, "Missing closing bracket\n");
	}

	add_token(exp,PLUS);
	*(exp->p) = TOKENEND;
	return(exp);
}


void print_tokens(expression *exp){
	for ( int *t = exp->token; (*(t+1)!=TOKENEND); *t++ ){
		if ( *t>=0 ){
			printf("%d",*t);
		} else if ( *t> -0xFF ){
			printf( AC_LBLUE " %c " AC_NORM ,op_list[-*t-1]);
		} else {
			printf("%c", (*t & (~INT_MIN)) + 'a' );
		}
	}
	printf("\n");
}


// below are the functions for the calculation(s)
#define INCTOKEN exp->p++;\
	if ( *(exp->p) == TOKENEND )\
		return(n); // end of input reached.

/// recursive function,
/// calculates the tokens.
/// callen by calculate
int calculate_token(expression *exp, int cv, int n ){
	int tmp = 0;
	int t = *(exp->p);
	dbgf("cv: %d, n: %d, t: %d\n",cv,n,t);

	if ( (t<BRCLOSE) && (t>-256) && ( cv <= t ) ){ // op
		return(n);
	}

	INCTOKEN; // increment index

	if ( t >=0 ){ // token is a number
		dbgf("number: %d\n",t);
		return( calculate_token(exp,cv,t) );
	} 

	if ( t < ( INT_MIN + 27 ) ){ // variable
		return( calculate_token(exp,cv, exp->var[t&31]) );
	} 

	if ( t<-2 ){ // no bracket, but operator
		// recursion - calculate dependent on math. order ( * before +, % before -, ..)
		tmp = calculate_token(exp,t,0);
	}

#ifdef DEBUG
	if ( exp->bracketcount )
		dbgf( AC_YELLOW " %d ", exp->bracketcount );
	dbgf( AC_BLUE " %d %c %d\n" AC_NORM ,n,op_list[-t-1],tmp);
#endif

	const static void *_operators[] = { &&op_inv, &&op_xor, &&op_or, &&op_and, 
		&&op_sqr, &&op_mod, &&op_mult, &&op_div, 
		&&op_minus, &&op_plus, &&op_brclose, &&op_bropen };
	const static void **operator = _operators + sizeof(_operators) / sizeof(void*);


	goto *operator[t];

op_bropen:
#ifdef DEBUG
	exp->bracketcount++;
#endif
	return( calculate_loop(exp,cv) );
op_brclose:
#ifdef DEBUG
	exp->bracketcount--;
#endif
	exp->bracketflag = 1; 
	return(n);
op_plus:	
	return( n + tmp );
op_minus:	
	return( n - tmp );
op_div:
	if ( tmp==0 )
		error(exp,5, "Division by zero\n");
	return( n / tmp );
op_mult:
	return( n * tmp );
op_mod:
	return( n % tmp );
op_sqr:
	return( ipowui( n, tmp ) );
op_and:
	return( n & tmp );
op_or:
	return( n | tmp );
op_xor:
	return( n ^ tmp );
op_inv:
	return( ~tmp );

}

// main loop for calculation.
// also handles brackets
int calculate_loop(expression* exp, int cv){
	int erg = 0; //calculate_token( exp,0,0 );
	while ( ( *(exp->p)!= TOKENEND ) && (!exp->bracketflag) ){ // <= ?
		erg = calculate_token( exp,0,erg ); // rem 1
		dbgf("erg loop, got: %d\n",erg);
	}
	if ( exp->bracketflag ){
		dbgf("Bracket out, got: %d\n",erg);
		exp->bracketflag = 0;
		erg = calculate_token( exp, cv, erg ); // rem 1
		dbgf("Bracket cont, got: %d\n",erg);
	}
	return(erg);
}

// calculate a parsed expression.
// Returns the result. 
// exp->error is set to > 0, if an error occured.
int calculate( expression* exp ){
	exp->bracketflag = exp->error = exp->flag = 0;
	exp->p = exp->token;
	return( calculate_loop(exp,0) );
}

#ifdef STANDALONE
int main( int argc, char *argv[] ){
	if ( (argc < 2) || ( argv[1][0]=='-' && argv[1][1]=='h') ){
		printf( "calcit, an expression parser and calculator\n"
				"BSD 3clause, Misc Myer (misc.myer@zoho.com), 2013-2022\n"
				"\n"
				"Usage: calc \"'expression'\"\n"
				"\nImplemented operations:\n"
				" + - / *\n"
				" %% ^       -- modulo, square\n"
				" X & | ~   -- bitwise XOR, AND, OR, INVERSE\n"
				" a..z      -- variables\n"
				" Lx Rx     -- shift bits left, right by x\n"
				" x<y x>y   -- return 1 when true, 0 when false\n"
				" xx ? yy : zz  -- if xx == 0 return yy; else zz\n"
				);
		exit(0);
	}
	expression exp; // sbrk todo
	parse( argv[1], &exp );
	printf("   ");
	print_tokens(&exp);
	int erg;
	
	exp.var[1] = 2;
	for ( int a = 0; a<3; a++ ){
		exp.var[0] = a;
		erg= calculate( &exp );
		printf(AC_LGREEN"%d\n"AC_NORM, erg);
	}

	exit(0);
}
#endif




