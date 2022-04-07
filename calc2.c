#ifndef MLIB
#include <stdio.h>
#include <string.h>
//#include <math.h>
#endif

// An expression parser and calculator
// original intent did had been to write something
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
// needs some cleanup. And there are variables missing.
// (So the original main goal, well.)
// But since the recursive parsing and nested calculation has been done,
// this might be trivial.
//
// just now I'm about to clean and document the code a bit.
//
// originally I did plan to implement a mouse driver,
// which should be able to do dynamic accelerations,
// or accelerate or translate key and button combinations
// with mouse movements.
// Therefore a low latency has been my main concern.
//
// Just now I'm wondering, wheter to replace the 
// operator case table with a jumptable
// or, albite unusual, with a goto label table;
// so the operators can be assigned at parse time to 
// the according symbols.
// spares one branch.
// 
// have to dig up the gcc documentation.
// another possilibty could be naked closures.
// 
//
// currently I'm trying to understand, what I've done.


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

typedef struct _expression { 
	int token[1024]; // better allocate dynamically. sbrk.
	int numtoken;
	int currenttoken;
	int bracketflag;
	int bracketcount;
	int syntaxerror;
	int *x;
} expression;

int calculate_loop(expression* exp, int cv);


// parsing

// parse input into tokens, write them to exp
char* nexttoken(expression* exp, char *c ){
	const static char op[] = { 0, '(', ')', '+', '-', '/', '*', '%', '^' };

	// Weed out space
	while ( *c == 32 ){
		c++;
		if ( !*c )
			return(c);
	}

	// number
	if ( ( *c >='0' ) && ( *c <='9' ) ){
		int i = 0; 
		do {
			i = i*10 + ( *c - '0' );
			c++;
			} while ( ( *c >='0' ) && ( *c <='9' ) );
		exp->token[exp->numtoken] = i;
		exp->numtoken++;
		printf("i: %d\n",i);
		return( c );
	}

	// operator
	int opi = 1;

	while( op[opi] != *c ){
		opi ++;
		if ( opi >= (sizeof(op)/sizeof(char)) ){
			//printf("Syntaxerror at pos %d, char: %c\n", 1, *c );
			printf("Syntaxerror, char: %c\n", *c );
			exit(1);
		}
	}

	exp->token[exp->numtoken] = -opi; // operators are negativ. 

	// numbers don't need to be negativ, negativ values are already prepended with the operator '-' ..
	// todo - check size of numtoken, reallocate
	exp->numtoken++;
	c++;
	return(c);
}


// parse the expression str.
// returns the "compiled" expression,
expression* parse(const char*str, expression* exp){
	//expression *exp = //malloc(sizeof(expression));
	exp->numtoken = 0;
	exp->bracketcount = 0;

	char *p = str;
	while (*p){
		p = nexttoken(exp,p);
	}

	return(exp);
}


// below are the functions for the calculation(s)
#define INCTOKEN(A) exp->currenttoken++;\
	if ( exp->currenttoken > exp->numtoken )\
	return(n); // end of input reached.

/// recursive function,
/// calculates the tokens.
/// callen by calculate
int calculate_token(expression *exp, int cv, int n ){
	dbgf("cv: %d, n: %d, rem: %d\n",cv,n,rem);
	int a;
	int neg = 0;
	int t = exp->token[exp->currenttoken];
	INCTOKEN(exp->currenttoken); // increment index
	dbgf("t1: %d\n",t);

	if ( t >=0 ){ // token is a number
		n = t;
		t = exp->token[exp->currenttoken];
		dbgf("n: %d, t: %c\n",n,-t);
		INCTOKEN(exp->currenttoken); // inc second time
	} 

	if ( t == -1 ){ // bracketopen
		exp->bracketcount++; 
		int tmp = calculate_loop(exp,cv);
		dbgf("bracket, %d\n",tmp);
		if ( neg ) 
			return( -tmp );
		return(tmp);
	}
	if ( t == -2 ){ // bracketclose
		exp->bracketflag = 1; 
		exp->bracketcount--;
		return(n);
	}

	if ( cv <= t ){
		exp->currenttoken--;
		return(n);
	}


	int tmp = calculate_token(exp,t,0);
	dbgf("tmp: %d, t: %d\n",tmp,t);

	const static void *_operators[] = { &&op_sqr, &&op_mod, &&op_mult, &&op_div, 
		&&op_minus, &&op_plus };
	const static void **operators = _operators + sizeof(_operators) / sizeof(void*) +2 ;


	goto *operators[t];

op_plus:	
	return( n + tmp );
op_minus:	
	return( n - tmp );
op_mult:
	return( n * tmp );
op_div:
	return( n / tmp );
op_mod:
	return( n % tmp );
op_sqr:
	return( ipowui( n, tmp ) );

}

// main loop for calculation.
// also handles brackets
int calculate_loop(expression* exp, int cv){
	int erg = calculate_token( exp,0,0 );
	while ( (exp->currenttoken < exp->numtoken) && (!exp->bracketflag) ){ // <= ?
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
// exp->syntaxerror is set to > 0, if an error occured.
int calculate( expression* exp ){
	exp->bracketflag = exp->currenttoken = exp->syntaxerror = 0;

	int erg = calculate_loop(exp,0);
	if ( exp->bracketcount != 0 ){
		fprintf(stderr,"Unmatched brackets in expression.\n");
		exp->syntaxerror = 2;
	}
	return(erg);
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
				" %% ^ $   -- modulo, square, square unsigned\n"
				" X & |   -- bitwise XOR, AND, OR, NOT\n"
				" L R     -- shift bits left, right\n"
				" < > !   -- branching\n"
				);
		exit(0);
	}
	expression exp; // sbrk todo
	parse( argv[1], &exp );
	int erg = calculate( &exp );
	printf("%d\n", erg);

	exit(0);
}
#endif




