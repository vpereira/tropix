/*
 ****************************************************************
 *								*
 *			command.c				*
 *								*
 *	Interpretador de comandos				*
 *								*
 ****************************************************************
 */

#include "../h/defs.h"

/*
 ******	Vari�veis externas **************************************
 */
extern char	*lp;
extern char	lastc;
extern long	dot;
extern long	expv;
extern int	adrflg;
extern int	pid;
extern MAP	txtmap, datmap;

/*
 ******	Prot�tipos locais ***************************************
 */
static void	printmap (MAP *, long);
static long	getaddr (void);
static void	printregs (void);
static void	listbkpt (void);

/*
 ****************************************************************
 *	Processa um comando					*
 ****************************************************************
 */
void
command (void)
{
	int		c;
	long		addr;

	adrflg = 0;

	/*
	 *	Pula espa�os
	 */
	while (*lp == ' ' || *lp == '\t')
		lp++;

	if (*lp == '\n' || *lp == '\0')
		return;

	/*
	 *	Verifica se tem endere�o
	 */
	if (*lp != ':' && *lp != '$' && *lp != '?' && *lp != '/')
	{
		addr = getaddr ();
		adrflg = 1;
		dot = addr;
	}

	/*
	 *	L� o comando
	 */
	c = *lp++;

	switch (c)
	{
	    /*
	     *	Examina arquivo (?)
	     */
	    case '?':		
		printmap (&txtmap, dot);
		break;

	    /*
	     *	Examina mem�ria/core (/)
	     */
	    case '/':		
		if (pid)
		{
			long val = ptrace (PT_RDUSER, pid, dot, 0);
			psymoff (dot, TSYM, ":\t");
			printf ("0x%08lX\n", val);
		}
		else
		{
			printmap (&datmap, dot);
		}
		dot += 4;
		break;

	    /*
	     *	Comandos especiais (:)
	     */
	    case ':':		
		c = *lp++;
		switch (c)
		{
		    case 'b':	/* Breakpoint */
			if (adrflg)
				addbkpt (dot);
			else
				printf ("Endere�o necess�rio\n");
			break;

		    case 'd':	/* Delete breakpoints */
			clrbkpt ();
			break;

		    case 'r':	/* Run */
			if (pid)
				printf ("Processo j� em execu��o\n");
			else
				setup ();
			break;

		    case 'c':	/* Continue */
			if (!pid)
				printf ("Nenhum processo\n");
			else
				runpcs (PT_CONTIN, 0);
			break;

		    case 's':	/* Single step */
			if (!pid)
				printf ("Nenhum processo\n");
			else
				runpcs (PT_SINGLE, 0);
			break;

		    case 'k':	/* Kill */
			if (pid)
			{
				endpcs ();
				printf ("Processo terminado\n");
			}
			break;

		    default:
			printf ("Comando desconhecido: :%c\n", c);
		}
		break;

	    /*
	     *	Comandos $ (registradores, etc)
	     */
	    case '$':		
		c = *lp++;
		switch (c)
		{
		    case 'r':	/* Registradores */
			if (!pid)
				printf ("Nenhum processo\n");
			else
				printregs ();
			break;

		    case 'q':	/* Quit */
			done ();
			break;

		    case 'b':	/* Lista breakpoints */
			listbkpt ();
			break;

		    case 'm':	/* Mostra mapas */
			printf ("txtmap: b1=0x%lX e1=0x%lX f1=0x%lX\n",
				txtmap.b1, txtmap.e1, txtmap.f1);
			printf ("        b2=0x%lX e2=0x%lX f2=0x%lX\n",
				txtmap.b2, txtmap.e2, txtmap.f2);
			break;

		    default:
			printf ("Comando desconhecido: $%c\n", c);
		}
		break;

	    /*
	     *	Sem comando, avan�a e mostra
	     */
	    case '\n':		
		if (adrflg)
		{
			printmap (&txtmap, dot);
			dot += 4;
		}
		break;

	    default:		
		printf ("Comando desconhecido: %c\n", c);
		break;
	}
}

/*
 ****************************************************************
 *	L� um endere�o (n�mero hex ou s�mbolo)			*
 ****************************************************************
 */
static long
getaddr (void)
{
	long		val = 0;
	char		name[64];
	int		i;

	/*
	 *	Pula espa�os
	 */
	while (*lp == ' ' || *lp == '\t')
		lp++;

	/*
	 *	N�mero hexadecimal (0x...)
	 */
	if (lp[0] == '0' && lp[1] == 'x')
	{
		lp += 2;
		while (1)
		{
			char c = *lp;
			if (c >= '0' && c <= '9')
				val = val * 16 + (c - '0');
			else if (c >= 'a' && c <= 'f')
				val = val * 16 + (c - 'a' + 10);
			else if (c >= 'A' && c <= 'F')
				val = val * 16 + (c - 'A' + 10);
			else
				break;
			lp++;
		}
		return (val);
	}

	/*
	 *	N�mero decimal
	 */
	if (*lp >= '0' && *lp <= '9')
	{
		while (*lp >= '0' && *lp <= '9')
		{
			val = val * 10 + (*lp - '0');
			lp++;
		}
		return (val);
	}

	/*
	 *	S�mbolo
	 */
	i = 0;
	while (*lp && *lp != ' ' && *lp != '\t' && *lp != '\n' &&
	       *lp != '?' && *lp != '/' && *lp != ':' && *lp != '+')
	{
		if (i < 63)
			name[i++] = *lp;
		lp++;
	}
	name[i] = '\0';

	if (i > 0)
	{
		if (!lookup (name, &val))
			printf ("S�mbolo n�o encontrado: %s\n", name);
	}

	/*
	 *	Operador +
	 */
	while (*lp == ' ' || *lp == '\t')
		lp++;

	if (*lp == '+')
	{
		lp++;
		val += getaddr ();
	}

	return (val);
}

/*
 ****************************************************************
 *	L� e imprime de um mapa				*
 ****************************************************************
 */
static void
printmap (MAP *mp, long addr)
{
	long		off, val;
	int		n;

	/*
	 *	Determina offset no arquivo
	 */
	if (addr >= mp->b1 && addr < mp->e1)
		off = mp->f1 + (addr - mp->b1);
	else if (addr >= mp->b2 && addr < mp->e2)
		off = mp->f2 + (addr - mp->b2);
	else
	{
		printf ("Endere�o fora do mapa: 0x%lX\n", addr);
		return;
	}

	/*
	 *	L� do arquivo
	 */
	if (mp->fd < 0)
	{
		printf ("Arquivo n�o aberto\n");
		return;
	}

	if (lseek (mp->fd, off, SEEK_SET) < 0)
	{
		printf ("Erro de seek\n");
		return;
	}

	if ((n = read (mp->fd, &val, 4)) != 4)
	{
		printf ("Erro de leitura\n");
		return;
	}

	psymoff (addr, TSYM, ":\t");
	printf ("0x%08lX\n", val);
}

/*
 ****************************************************************
 *	Imprime registradores					*
 ****************************************************************
 */
static void
printregs (void)
{
	if (!pid)
		return;

	/*
	 *	TODO: Ler registradores da UPROC
	 *	Por enquanto, placeholder
	 */
	printf ("EAX=????????  EBX=????????  ECX=????????  EDX=????????\n");
	printf ("ESI=????????  EDI=????????  EBP=????????  ESP=????????\n");
	printf ("EIP=????????  EFLAGS=????????\n");
}

/*
 ****************************************************************
 *	Lista breakpoints					*
 ****************************************************************
 */
static void
listbkpt (void)
{
	BKPT	*bp;
	int	count;

	count = 0;
	for (bp = bkpthead; bp != NULL; bp = bp->nxtbkpt)
	{
		if (bp->flag)
		{
			printf ("%d: ", count++);
			psymoff (bp->loc, TSYM, "\n");
		}
	}

	if (count == 0)
		printf ("Nenhum breakpoint\n");
}

/*
 ****************************************************************
 *	Fun��es auxiliares					*
 ****************************************************************
 */
void
flushbuf (void)
{
	fflush (stdout);
}

void
printc (int c)
{
	putchar (c);
}

void
prints (const char *s)
{
	printf ("%s", s);
}
