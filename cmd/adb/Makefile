#*
#****************************************************************
#*	Makefile para o programa "adb"				*
#****************************************************************
#*
ROOT	=
DIR	= /usr/bin
PGM	= adb
PROT	= a+rx
MAN	= cmd
CC	= cc
CFLAGS	= -O -I h

SRC	= src/main.c src/setup.c src/sym.c src/runpcs.c src/command.c

.virtual	clean cp cmp cmpq cmpqcp

$PGM:		$SRC h/defs.h h/machine.h
		$CC $CFLAGS -o $PGM $SRC
		size $PGM

clean:
		-rm -f $PGM

cp:		$PGM
		cp -f $PGM $DIR
		chmod $PROT $DIR/$PGM
		ls -L $DIR/$PGM

cmp:
		-cmpobj $PGM $DIR

cmpq:
		-cmpobj -q $PGM $DIR

cmpqcp:
		-cmpobj -q $PGM $DIR || make cp
