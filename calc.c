#ifdef MLIB
#include "minilib.h"
#else
#include <stdio.h>
#include <string.h>
//#include <math.h>
#endif

//TODO: rewrite for double ...

//TODO: variables: array[] of refs to the values in struct expression
//-> müssen nicht bei jedem calculate-aufruf neu zugewiesen werden.
// 

#define STANDALONE
//#define DEBUG

#if 0
#ifdef DEBUG
#define dbgf(...) printf(__VA_ARGS__)
#else
#define dbgf(...)
#endif
#endif

typedef struct _expression { 
		int token[1024];
		int numtoken;
		int currenttoken;
		int bracketflag;
		int bracketcount;
		int syntaxerror;
		int *x;
} expression;

int calculate_loop(expression* exp, int cv);

// test A for being a Digit( 0..9).
#define ISDIGIT(A) ( !( A & ( ~(__INT_MAX__) ) || (  A>10) ))

// parse input into tokens, write them to exp
int nexttoken(expression* exp, const char *str, int pos){
		char c;
		int a;
		// Weed out space
		while ( str[pos] == 32 ){
				pos++;
				if ( str[pos] == 0 )
						return(pos);
		}
		c = str[pos];

		a = c - '0';
		if ISDIGIT(a){ // convert a digit
				pos++;
				int b = str[pos] - '0';
				while ( ( str[pos] != 0 ) && ISDIGIT(b) ){
						a= a*10 + b;
						pos++;
						b = str[pos] - '0';
				}
				exp->token[exp->numtoken] = a;
				exp->numtoken++;
				return( pos );
		}

		// got an operator
		exp->token[exp->numtoken] = -c; // operators are negativ. 
		// numbers don't need to be negativ, negativ values are already prepended with the operator '-' ..
		exp->numtoken++;
		pos++;
		return(pos);
}


/* ------------------------------------------------------------------------- */
/// parse the expression str.
/// returns the "compiled" expression,
/// 0 if a error occured.
/* ------------------------------------------------------------------------- */
expression* parse(const char*str, expression* exp){
		int pos = 0;
		//expression *exp = //malloc(sizeof(expression));
		exp->numtoken = 0;
		exp->bracketcount = 0;

		int l = strlen(str);
		while ( pos < l ){
				pos = nexttoken(exp,str,pos);
				if ( pos <0 ){ // error. also free exp here.
						//free(str);
						fprintf(stderr,"Error parsing: %s\nIn %s, line %d\n",str,__FILE__,__LINE__);
						return(0);
				}
		}
		return(exp);
}

#define INCTOKEN(A) exp->currenttoken++;\
		if ( exp->currenttoken > exp->numtoken )\
				return(n); // end of input reached.
		
/// recursive function,
/// calculates the tokens.
/// callen by calculate
int calculate_token(expression *exp, int cv, int n, int rem ){
		dbgf("cv: %d, n: %d, rem: %d\n",cv,n,rem);
		int a;
		int neg = 0;
		int t = exp->token[exp->currenttoken];
		INCTOKEN(exp->currenttoken);
		dbgf("t1: %d\n",t);

		if ( t == -'-' ){ // handle things like 4*(-5) 
				// syntaxerror if exp->currenttoken>scalar @c (!)
				if ( rem == 0 ){
						if ( exp->token[exp->currenttoken] >= 0 ){ 
								n=-exp->token[exp->currenttoken];
								INCTOKEN(exp->currenttoken);
						} else {
								switch(exp->token[exp->currenttoken]){
										case -'(' : { break; }
										case -'X': { break; }
										default: {
													// delete this for further expressions.. (sqrt, X, ...)
													fprintf(stderr, "Syntaxerror\n"); // try to continue..
													exp->syntaxerror = 1;
													}
								}
								
						}
								t=exp->token[exp->currenttoken];
								INCTOKEN(exp->currenttoken);
								dbgf("neg: %d\n",n);
				} 
				neg=1;
		} else if ( t >=0 ){ // got a num. fetch operator.
				n = t;
				t = exp->token[exp->currenttoken];
				dbgf("n: %d, t: %c\n",n,-t);
				INCTOKEN(exp->currenttoken);
		} else {
				switch ( t ){
						case -'X': { n = *exp->x; if ( neg ) n=-n; //BUG: neg can't be set here.
				t = exp->token[exp->currenttoken];
				dbgf("x: %d, t: %c\n",n,-t);
				INCTOKEN(exp->currenttoken);
											 }
				}
		}



		dbgf(": %d%c\n",n,-t);
		switch ( t ){
				case -'(': { dbgf("Bracket open, neg: %d\n",neg);
											 exp->bracketcount++; 
											 if ( neg ) n = -calculate_loop(exp,cv); 
											 		else n = calculate_loop(exp,cv); 
											 break;}
				case -')': { exp->bracketflag = 1; 
										 exp->bracketcount--;
										 break;}
				case -'=': { if ( cv > 1 ){ exp->currenttoken--;} else { ( n == calculate_token(exp,2,0,0) ) ? (n=1) : (n=0);}break;}
				case -'+': { if ( cv > 2 ){ exp->currenttoken--;} else { n += calculate_token(exp,3,0,0);} break;}
				case -'-': { if ( cv > 3 ){ exp->currenttoken--;} else { n -= calculate_token(exp,3,0,0);} break;}
				case -'*': { if ( cv > 3 ){ exp->currenttoken--;} else { n *= calculate_token(exp,3,0,0);} break;}
				case -'/': { if ( cv > 3 ){ exp->currenttoken--;} else { n /= calculate_token(exp,4,0,0);} break;}
				case -'%': { if ( cv > 3 ){ exp->currenttoken--;} else { n %= calculate_token(exp,4,0,0);} break;}
#ifdef MLIB
				case -'^': { if ( cv > 5 ){ exp->currenttoken--;} else { n = ipoweri(n,calculate_token(exp,5,0,0));} break;}
				case -'$': { if ( cv > 5 ){ exp->currenttoken--;} else { n = ipoweriui(n,(unsigned int)(calculate_token(exp,5,0,0)));} break;}
#else
				case -'^': { if ( cv > 5 ){ exp->currenttoken--;} else { n ^= calculate_token(exp,5,0,0);} break;}
				case -'$': { if ( cv > 5 ){ exp->currenttoken--;} else { n ^= calculate_token(exp,5,0,0);} break;}
#endif
				case -'&': { if ( cv > 6 ){ exp->currenttoken--;} else { n = n & calculate_token(exp,6,0,0);}break;}
				case -'|': { if ( cv > 6 ){ exp->currenttoken--;} else { n = n | calculate_token(exp,6,0,0);}break;}
				case -'X': { if ( cv > 6 ){ exp->currenttoken--;} else { asm volatile ("xor %2,%0":"=m"(n):"m"(n),"r"(calculate_token(exp,6,0,0)));}break;}
				case -'L': { if ( cv > 7 ){ exp->currenttoken--;} else { n = n << calculate_token(exp,7,0,0);}break;}
				case -'R': { if ( cv > 7 ){ exp->currenttoken--;} else { n = n >> calculate_token(exp,7,0,0);}break;}
				case -'<': { if ( cv > 8 ){ exp->currenttoken--;} else { int t = calculate_token(exp,8,0,0); if ( t < n ) n=t;}break;}
				case -'>': { if ( cv > 8 ){ exp->currenttoken--;} else { int t = calculate_token(exp,8,0,0); if ( t > n ) n=t;}break;}
				case -'!': { if ( cv > 9 ){ exp->currenttoken--;} else { for (a=2;a<n-1;a++){n*=a;};}; break; }
				default: {
						fprintf(stderr, "Syntaxerror: %d\n",t );
						exp->syntaxerror = 1;
						// try to continue, anyway..
				}
		}
		return(n);
}

// main loop for calculation.
// also handles brackets
int calculate_loop(expression* exp, int cv){
		int erg = calculate_token( exp,0,0,0 );
		while ( (exp->currenttoken < exp->numtoken) && (!exp->bracketflag) ){ // <= ?
				erg = calculate_token( exp,0,erg,1 );
				dbgf("erg loop, got: %d\n",erg);
		}
		if ( exp->bracketflag ){
				dbgf("Bracket out, got: %d\n",erg);
				exp->bracketflag = 0;
				erg = calculate_token( exp, cv, erg,1);
				dbgf("Bracket cont, got: %d\n",erg);
		}
		return(erg);
}

/* ------------------------------------------------------------------------- */
/// calculate a parsed expression.
/// Returns the result. 
/// exp->syntaxerror is set to > 0, if an error occured.
/* ------------------------------------------------------------------------- */
int calculate( expression* exp ){
		exp->bracketflag = exp->currenttoken = exp->syntaxerror = 0;

		int erg = calculate_loop(exp,0);
		if ( exp->bracketcount != 0 ){
				fprintf(stderr,"Unmatched brackets in expression!\n");
				exp->syntaxerror = 2;
		}
		return(erg);
}

#ifdef STANDALONE
int main( int argc, char *argv[] ){
		dbgf("arg: %s\n",argv[1]);
		dbgf("max: %u\n",__INT_MAX__);
		// example of howto use..
		if ( argc < 2 ){
				printf( "calcit, a tiny text mode calculator\n"
								"Written by Misc Myer (misc.myer@zoho.com), 2013-2019\n"
								"No function is guaranteed, although thoroughly tested\n"
								"You are required to drop me an email, when using this piece of cake.\n"
								"The recursive parsing and calculation has been a special fun.. ;)\n"
								"Happy Hacking\n\n"
								"Usage: calc \"'expression'\"\n"
								"\nImplemented operations:\n"
								" + - / *\n"
								" % ^ $   -- modulo, square, square unsigned\n"
								" X & |   -- bitwise XOR, AND, OR, NOT\n"
								" L R     -- shift bits left, right\n"
								" < > !   -- branching\n"
							);
				return(0);
		}
		expression exp;
		parse( argv[1], &exp );
		int erg = calculate( &exp );
		printf("%d\n", erg);

		return(0);
}
#endif




