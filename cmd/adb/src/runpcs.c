/*
 ****************************************************************
 *								*
 *			runpcs.c				*
 *								*
 *	Controle de processo com ptrace				*
 *								*
 ****************************************************************
 */

#include "../h/defs.h"

/*
 ******	Vari�veis globais ***************************************
 */
extern int	pid;
extern int	signo;
extern int	executing;
extern int	sigint, sigqit;
extern char	*lp;

BKPT		*bkpthead;	/* Lista de breakpoints */
static int	userpc = 1;	/* PC do usu�rio */

static void	bpwait (void);	/* Forward declaration */

/*
 ****************************************************************
 *	Inicia o processo filho					*
 ****************************************************************
 */
void
setup (void)
{
	close (fsym);
	fsym = -1;

	pid = fork ();

	if (pid == 0)
	{
		/*
		 *	Processo filho
		 */
		ptrace (PT_SETTRC, 0, 0, 0);
		signal (SIGINT, (void (*)(int))sigint);
		signal (SIGQUIT, (void (*)(int))sigqit);

		/*
		 *	Executa o programa
		 *	Por simplicidade, sem argumentos por enquanto
		 */
		execl (symfil, symfil, (char *)NULL);
		fprintf (stderr, "adb: N�o consegui executar \"%s\"\n", symfil);
		exit (1);
	}
	else if (pid < 0)
	{
		adb_error ("N�o consegui criar processo filho");
	}
	else
	{
		/*
		 *	Processo pai: espera o filho parar
		 */
		bpwait ();
		fsym = open (symfil, wtflag);
		printf ("Processo %d iniciado\n", pid);
	}
}

/*
 ****************************************************************
 *	Termina o processo filho				*
 ****************************************************************
 */
void
endpcs (void)
{
	BKPT	*bp;

	if (pid)
	{
		ptrace (PT_EXIT, pid, 0, 0);
		pid = 0;
		userpc = 1;

		/*
		 *	Reseta breakpoints
		 */
		for (bp = bkpthead; bp != NULL; bp = bp->nxtbkpt)
		{
			if (bp->flag)
				bp->flag = BKPTSET;
		}
	}
}

/*
 ****************************************************************
 *	Executa o processo					*
 ****************************************************************
 */
int
runpcs (int runmode, int execsig)
{
	BKPT		*bp;

	if (adrflg)
		userpc = (int)dot;

	setbp ();
	printf ("Executando...\n");

	/*
	 *	Continua ou single-step
	 */
	ptrace (runmode, pid, userpc, execsig);
	bpwait ();

	/*
	 *	Verifica breakpoint
	 */
	if (signo == 0)
	{
		/*
		 *	L� o PC atual
		 */
		long pc = ptrace (PT_RUREGS, pid, /* offset do EIP */ 0, 0);

		if ((bp = scanbkpt (pc - 1)) != NULL)
		{
			printf ("Breakpoint em ");
			psymoff (bp->loc, TSYM, "\n");
			userpc = bp->loc;
			return (1);
		}
	}
	else
	{
		printf ("Sinal %d recebido\n", signo);
	}

	userpc = 1;
	return (0);
}

/*
 ****************************************************************
 *	Espera o processo filho parar				*
 ****************************************************************
 */
void
bpwait (void)
{
	int		w, status;

	signal (SIGINT, SIG_IGN);

	while ((w = wait (&status)) != pid && w != -1)
		;

	signal (SIGINT, fault);

	if (w == -1)
	{
		pid = 0;
		adb_error ("Processo filho terminou");
	}
	else if (WIFEXITED (status))
	{
		printf ("Processo terminou com c�digo %d\n", WEXITSTATUS (status));
		pid = 0;
		signo = 0;
	}
	else if (WIFSIGNALED (status))
	{
		printf ("Processo morto por sinal %d\n", WTERMSIG (status));
		pid = 0;
		signo = WTERMSIG (status);
	}
	else if (WIFSTOPPED (status))
	{
		signo = WSTOPSIG (status);
		if (signo == SIGTRAP)
			signo = 0;	/* Breakpoint ou single-step */
	}
}

/*
 ****************************************************************
 *	Insere breakpoints no processo				*
 ****************************************************************
 */
void
setbp (void)
{
	BKPT	*bp;
	long		ins;

	for (bp = bkpthead; bp != NULL; bp = bp->nxtbkpt)
	{
		if (bp->flag)
		{
			/*
			 *	Salva instru��o original
			 */
			bp->ins = ptrace (PT_RIUSER, pid, bp->loc, 0);

			/*
			 *	Insere INT3 (0xCC)
			 */
			ins = (bp->ins & ~0xFF) | BPT;
			ptrace (PT_WIUSER, pid, bp->loc, ins);
		}
	}
}

/*
 ****************************************************************
 *	Remove breakpoints do processo				*
 ****************************************************************
 */
void
delbp (void)
{
	BKPT	*bp;

	for (bp = bkpthead; bp != NULL; bp = bp->nxtbkpt)
	{
		if (bp->flag)
		{
			ptrace (PT_WIUSER, pid, bp->loc, bp->ins);
		}
	}
}

/*
 ****************************************************************
 *	Procura breakpoint em um endere�o			*
 ****************************************************************
 */
BKPT *
scanbkpt (long addr)
{
	BKPT	*bp;

	for (bp = bkpthead; bp != NULL; bp = bp->nxtbkpt)
	{
		if (bp->flag && bp->loc == addr)
			return (bp);
	}

	return (NULL);
}

/*
 ****************************************************************
 *	Adiciona um breakpoint					*
 ****************************************************************
 */
void
addbkpt (long addr)
{
	BKPT	*bp;

	/*
	 *	Verifica se j� existe
	 */
	if (scanbkpt (addr) != NULL)
	{
		printf ("Breakpoint j� existe em ");
		psymoff (addr, TSYM, "\n");
		return;
	}

	/*
	 *	Aloca novo breakpoint
	 */
	bp = (BKPT *)malloc (sizeof (BKPT));
	if (bp == NULL)
	{
		adb_error ("Mem�ria insuficiente para breakpoint");
		return;
	}

	bp->loc = addr;
	bp->ins = 0;
	bp->flag = BKPTSET;
	bp->count = 1;
	bp->initcnt = 1;
	bp->comm = NULL;
	bp->nxtbkpt = bkpthead;
	bkpthead = bp;

	printf ("Breakpoint em ");
	psymoff (addr, TSYM, "\n");
}

/*
 ****************************************************************
 *	Remove todos os breakpoints				*
 ****************************************************************
 */
void
clrbkpt (void)
{
	BKPT	*bp, *next;

	for (bp = bkpthead; bp != NULL; bp = next)
	{
		next = bp->nxtbkpt;
		free (bp);
	}

	bkpthead = NULL;
	printf ("Breakpoints removidos\n");
}
