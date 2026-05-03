/*
 ****************************************************************
 *								*
 *			h/defs.h				*
 *								*
 *	Defini��es principais do ADB				*
 *								*
 *	Vers�o	1.0.0, de 26.01.26				*
 *								*
 *	M�dulo: adb						*
 *		Utilitarios B�sicos				*
 *		Categoria B					*
 *								*
 *	TROPIX: Sistema Operacional Tempo-Real Multiprocessado	*
 *		Copyright � 2026 NCE/UFRJ			*
 *								*
 ****************************************************************
 */

/*
 *	Layout do arquivo a.out (Tropix):
 *
 *	HEADER (sizeof HEADER bytes)
 *	TEXT   (h_tsize bytes)
 *	DATA   (h_dsize bytes)
 *	SYMTAB (h_ssize bytes)
 *	RELOC  (h_rtsize + h_rdsize bytes)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
/* #include <sys/uproc.h> */	/* TODO: precisa de vers�o user-space */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <a.out.h>

#include "machine.h"

/*
 ******	Macros de wait (Tropix n�o tem sys/wait.h) ******************
 */
#define	WIFEXITED(s)	(((s) & 0x7F) == 0)
#define	WEXITSTATUS(s)	(((s) >> 8) & 0xFF)
#define	WIFSIGNALED(s)	(((s) & 0x7F) != 0 && ((s) & 0x7F) != 0x7F)
#define	WTERMSIG(s)	((s) & 0x7F)
#define	WIFSTOPPED(s)	(((s) & 0xFF) == 0x7F)
#define	WSTOPSIG(s)	(((s) >> 8) & 0xFF)

/*
 ******	Tipos b�sicos *******************************************
 */
typedef	char		CHAR;
typedef	char		*STRING;
typedef	short		INT;
typedef	int		BOOL;
typedef	long		L_INT;
typedef	unsigned	POS;

#define	NOSTRING	(STRING)NULL

/*
 ******	Modos de espa�o de endere�amento ************************
 */
#define	NSP		0	/* Nenhum */
#define	ISP		1	/* Instru��es (text) */
#define	DSP		2	/* Dados */

/*
 ******	Tipos de s�mbolos ***************************************
 */
#define	NSYM		0	/* Nenhum */
#define	ASYM		1	/* Absoluto */
#define	TSYM		2	/* Text */
#define	DSYM		3	/* Data */
#define	BSYM		4	/* BSS */
#define	ESYM		(-1)	/* Fim */

/*
 ******	Breakpoints *********************************************
 */
#define	BKPTSET		1
#define	BKPTEXEC	2
#define	MAXBKPT		32

typedef struct bkpt
{
	long		loc;		/* Endere�o */
	long		ins;		/* Instru��o original */
	int		flag;		/* Estado */
	int		count;		/* Contador */
	int		initcnt;	/* Contador inicial */
	char		*comm;		/* Comando */
	struct bkpt	*nxtbkpt;	/* Pr�ximo */

}	BKPT;

/*
 ******	Mapeamento de mem�ria ***********************************
 */
typedef struct
{
	long		b1, e1, f1;	/* Segmento 1: base, end, file offset */
	long		b2, e2, f2;	/* Segmento 2: base, end, file offset */
	int		fd;		/* File descriptor */

}	MAP;

/*
 ******	S�mbolo em mem�ria **************************************
 */
typedef struct
{
	char		name[64];	/* Nome do s�mbolo */
	long		value;		/* Valor */
	int		type;		/* Tipo */

}	SYMENT;

/*
 ******	Constantes **********************************************
 */
#define	MAXOFF		255	/* Offset m�ximo para s�mbolo+offset */
#define	MAXPOS		80	/* Largura m�xima da linha */
#define	MAXLIN		256	/* Tamanho m�ximo da linha */
#define	MAXARG		32	/* Argumentos m�ximos para exec */
#define	MAXSYM		4096	/* S�mbolos m�ximos em cache */

#define	EOR		'\n'
#define	TB		'\t'
#define	SP		' '

/*
 ******	Vari�veis globais ***************************************
 */
extern HEADER		hdr;		/* Cabe�alho a.out */
extern MAP		txtmap;		/* Mapa do arquivo */
extern MAP		datmap;		/* Mapa do core/processo */

extern int		fsym;		/* FD do arquivo a.out */
extern int		fcor;		/* FD do core */
extern int		pid;		/* PID do processo filho */

extern long		dot;		/* Endere�o corrente */
extern long		expv;		/* Valor da express�o */
extern int		adrflg;		/* Flag de endere�o */
extern int		errflg;		/* Flag de erro */
extern int		signo;		/* Sinal recebido */

extern STRING		symfil;		/* Nome do arquivo */
extern STRING		corfil;		/* Nome do core */
extern int		maxoff;		/* Offset m�ximo para s�mbolo+offset */
extern int		wtflag;		/* Flag de escrita */
extern BKPT		*bkpthead;	/* Lista de breakpoints */

extern jmp_buf		erradb;		/* Recupera��o de erro */

/*
 ******	Prot�tipos **********************************************
 */
/* setup.c */
void		setsym (void);
void		setcor (void);
int		getfile (const char *, int);

/* sym.c */
int		findsym (long, int);
void		psymoff (long, int, const char *);
char		*symname (void);
long		symvalue (void);

/* runpcs.c */
void		setup (void);
void		endpcs (void);
int		runpcs (int, int);
void		setbp (void);
void		delbp (void);
BKPT		*scanbkpt (long);

/* pcs.c */
long		ptget (int, long);
void		ptput (int, long, long);

/* print.c */
void		printc (int);
void		prints (const char *);
void		printn (long, int);

/* command.c */
void		command (void);

/* expr.c */
int		expr (int);

/* opset.c */
int		printins (long);

/* access.c */
long		get (long, int);
void		put (long, int, long);

/* input.c */
int		rdc (void);
char		readc (void);
int		getnum (int);

/* output.c */
void		flushbuf (void);

/* main.c */
void		adb_error (const char *);
void		fault (int);
void		done (void);
