PROG=calc

#BUILDDIR=.

# Don't create obj files, include evrything in one gcc run.
#SINGLERUN=1 

#DEBUG=1

ifdef with-minilib
include minilib/makefile.include 
endif

#Compile with minilib or
default: 
	$(if $(wildcard minilib/makefile.include), make with-minilib=1, make native )


# make native 
# compile with dynamic loading, os native libs
native: calc.c
	$(info call "make getminilib" to fetch and extract the "minilib" and compile $(PROG) static (recommended) )
	gcc -o calc calc.c


ifndef with-minilib

rebuild:
	make clean
	make

clean:
	rm -f calc
	cd build && rm -f *.o

endif

getminilib: minilib/minilib.h

minilib/minilib.h:
	$(info get minilib)
	curl https://codeload.github.com/michael105/minilib/zip/master > minilib.zip
	unzip minilib.zip
	mv minilib-master minilib
	make rebuild

install: calc
	cp calc /usr/local/bin

ifdef undef
		Not tested. 
calcdoubletest: build/calcdoubletest.o build/minilib.o
	ld $(LDFLAGS) -o calcdoubletest build/calcdoubletest.o build/minilib.o

build/calcdoubletest.o: calcdoubletest.c
	gcc $(CFLAGS) -nodefaultlibs -c -o build/calcdoubletest.o calcdoubletest.c
endif

