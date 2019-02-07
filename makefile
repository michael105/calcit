PROG=calc

#BUILDDIR=.

# Don't create obj files, include evrything in one gcc run.
#SINGLERUN=1 

#Compile with minilib or
include minilib/makefile.include


# make native 
# compile with dynamic loading, os native libs
native:
	gcc -o calc calc.c



ifdef undef
		Not tested. 
calcdoubletest: build/calcdoubletest.o build/minilib.o
	ld $(LDFLAGS) -o calcdoubletest build/calcdoubletest.o build/minilib.o

build/calcdoubletest.o: calcdoubletest.c
	gcc $(CFLAGS) -nodefaultlibs -c -o build/calcdoubletest.o calcdoubletest.c
endif

