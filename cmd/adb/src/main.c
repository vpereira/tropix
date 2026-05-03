/*
 ****************************************************************
 *								*
 *			main.c					*
 *								*
 *	Depurador ADB para TROPIX				*
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

#include "../h/defs.h"

/*
 ******	Vari�veis globais ***************************************
 */
const char	pgversion[] = "Vers�o: 1.0.0, de 26.01.26";

HEADER		hdr;		/* Cabe�alho a.out */
MAP		txtmap;		/* Mapa do arquivo */
MAP		datmap;		/* Mapa do core/processo */

int		fsym = -1;	/* FD do arquivo a.out */
int		fcor = -1;	/* FD do core */
int		pid;		/* PID do processo filho */
int		wtflag;		/* Flag de escrita */

long		dot;		/* Endere�o corrente */
long		expv;		/* Valor da express�o */
int		adrflg;		/* Flag de endere�o */
int		errflg;		/* Flag de erro */
int		signo;		/* Sinal recebido */
int		mkfault;	/* Fault flag */
int		executing;	/* Executando */

STRING		symfil = "a.out";
STRING		corfil = "core";

char		lastc;		/* �ltimo caractere lido */
char		*lp;		/* Ponteiro de linha */
char		linebuf[MAXLIN];

long		var[36];	/* Vari�veis a-z, 0-9 */
int		maxoff = MAXOFF;

jmp_buf		erradb;

entry int	sigint;		/* Handler original SIGINT */
entry int	sigqit;		/* Handler original SIGQUIT */

/*
 ****************************************************************
 *	Tratamento de erros					*
 ****************************************************************
 */
void
adb_error (const char *msg)
{
	errflg = 1;
	if (msg != NOSTR)
		fprintf (stderr, "%s\n", msg);
	longjmp (erradb, 1);
}

void
fault (int sig)
{
	signal (sig, fault);
	mkfault++;
}

/*
 ****************************************************************
 *	Programa principal					*
 ****************************************************************
 */
int
main (int argc, const char *argv[])
{
	int		opt;

	/*
	 *	Analisa as op��es
	 */
	while ((opt = getopt (argc, argv, "wH")) != EOF)
	{
		switch (opt)
		{
		    case 'w':		/* Permite escrita */
			wtflag = O_RDWR;
			break;

		    case 'H':		/* Help */
			fprintf (stderr,
				"%s - depurador simb�lico\n"
				"\n%s\n"
				"\nSintaxe:\n"
				"\t%s [-w] [<programa> [<core>]]\n"
				"\nOp��es:\n"
				"\t-w: permite escrita no arquivo\n"
				"\nComandos:\n"
				"\t<addr>?<fmt>  examina arquivo\n"
				"\t<addr>/<fmt>  examina mem�ria/core\n"
				"\t<addr>:b      breakpoint\n"
				"\t:d            deleta breakpoints\n"
				"\t:r            executa programa\n"
				"\t:c            continua\n"
				"\t:s            single step\n"
				"\t$r            mostra registradores\n"
				"\t$q            sai\n",
				pgname, pgversion, pgname
			);
			exit (0);

		    default:
			putc ('\n', stderr);
			exit (2);
		}
	}

	argv += optind;
	argc -= optind;

	/*
	 *	Argumentos: programa e core
	 */
	if (argc > 0)
		symfil = (char *)argv[0];
	if (argc > 1)
		corfil = (char *)argv[1];

	/*
	 *	Abre arquivos
	 */
	setsym ();
	setcor ();

	/*
	 *	Configura vari�veis
	 */
	var['b' - 'a'] = hdr.h_dstart;		/* base do data */
	var['d' - 'a'] = hdr.h_dsize;		/* tamanho data */
	var['e' - 'a'] = hdr.h_entry;		/* entry point */
	var['m' - 'a'] = hdr.h_magic;		/* magic */
	var['t' - 'a'] = hdr.h_tsize;		/* tamanho text */

	/*
	 *	Configura sinais
	 */
	if ((sigint = (int)signal (SIGINT, SIG_IGN)) != (int)SIG_IGN)
	{
		sigint = (int)fault;
		signal (SIGINT, fault);
	}
	sigqit = (int)signal (SIGQUIT, SIG_IGN);

	/*
	 *	Loop principal
	 */
	setjmp (erradb);

	if (executing)
		delbp ();
	executing = 0;

	for (;;)
	{
		flushbuf ();

		if (errflg)
		{
			errflg = 0;
		}

		if (mkfault)
		{
			mkfault = 0;
			printf ("\n");
		}

		/*
		 *	Prompt
		 */
		printf ("* ");
		fflush (stdout);

		/*
		 *	L� comando
		 */
		if (fgets (linebuf, MAXLIN, stdin) == NOSTR)
		{
			printf ("\n");
			done ();
		}

		lp = linebuf;
		command ();
	}
}

/*
 ****************************************************************
 *	Termina o depurador					*
 ****************************************************************
 */
void
done (void)
{
	endpcs ();
	exit (0);
}
