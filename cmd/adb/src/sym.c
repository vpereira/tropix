/*
 ****************************************************************
 *								*
 *			sym.c					*
 *								*
 *	Gerenciamento da tabela de s�mbolos			*
 *								*
 ****************************************************************
 */

#include "../h/defs.h"

/*
 ******	Vari�veis locais ****************************************
 */
static SYMENT	*symtab;		/* Tabela de s�mbolos em mem�ria */
static int	nsyms;			/* N�mero de s�mbolos */
static SYMENT	cursym;			/* S�mbolo corrente */

/*
 ****************************************************************
 *	Carrega a tabela de s�mbolos do arquivo			*
 ****************************************************************
 */
void
loadsym (void)
{
	long		symoff;		/* Offset da tabela no arquivo */
	long		symend;		/* Fim da tabela */
	long		pos;
	SYM		sym;
	int		count = 0;

	if (fsym < 0 || hdr.h_ssize == 0)
		return;

	/*
	 *	Calcula offset: HEADER + TEXT + DATA
	 */
	symoff = sizeof (HEADER) + hdr.h_tsize + hdr.h_dsize;
	symend = symoff + hdr.h_ssize;

	/*
	 *	Primeira passada: conta s�mbolos
	 */
	pos = symoff;
	while (pos < symend)
	{
		if (lseek (fsym, pos, SEEK_SET) < 0)
			break;
		if (read (fsym, &sym, sizeof (SYM)) != sizeof (SYM))
			break;
		count++;
		pos += SYM_SIZEOF (sym.s_nm_len);
	}

	if (count == 0)
		return;

	/*
	 *	Aloca tabela
	 */
	symtab = (SYMENT *)malloc (count * sizeof (SYMENT));
	if (symtab == NOVOID)
	{
		fprintf (stderr, "adb: Mem�ria insuficiente para tabela de s�mbolos\n");
		return;
	}

	/*
	 *	Segunda passada: carrega s�mbolos
	 */
	nsyms = 0;
	pos = symoff;

	while (pos < symend && nsyms < count)
	{
		char		namebuf[256];
		int		nmlen;
		int		stype;

		if (lseek (fsym, pos, SEEK_SET) < 0)
			break;
		if (read (fsym, &sym, sizeof (SYM)) != sizeof (SYM))
			break;

		/*
		 *	L� o nome (come�a em s_name, pode ser maior)
		 */
		nmlen = sym.s_nm_len;
		if (nmlen > 63)
			nmlen = 63;

		memcpy (namebuf, sym.s_name, 4);
		if (sym.s_nm_len > 4)
		{
			read (fsym, namebuf + 4, nmlen - 4);
		}
		namebuf[nmlen] = '\0';

		/*
		 *	Converte tipo
		 */
		stype = NSYM;
		switch (sym.s_type & 0x0F)
		{
		    case ABS:	stype = ASYM; break;
		    case TEXT:	stype = TSYM; break;
		    case DATA:	stype = DSYM; break;
		    case BSS:	stype = BSYM; break;
		}

		/*
		 *	Armazena
		 */
		strcpy (symtab[nsyms].name, namebuf);
		symtab[nsyms].value = sym.s_value;
		symtab[nsyms].type = stype;
		nsyms++;

		pos += SYM_SIZEOF (sym.s_nm_len);
	}

	printf ("adb: %d s�mbolos carregados\n", nsyms);
}

/*
 ****************************************************************
 *	Encontra s�mbolo mais pr�ximo de um endere�o		*
 ****************************************************************
 */
int
findsym (long addr, int type)
{
	int		i;
	long		best_diff = 0x7FFFFFFF;
	int		best = -1;
	long		diff;

	for (i = 0; i < nsyms; i++)
	{
		if (type != NSYM && symtab[i].type != type)
			continue;

		diff = addr - symtab[i].value;

		if (diff >= 0 && diff < best_diff)
		{
			best_diff = diff;
			best = i;
		}
	}

	if (best >= 0 && best_diff <= maxoff)
	{
		cursym = symtab[best];
		return ((int)best_diff);
	}

	return (-1);
}

/*
 ****************************************************************
 *	Imprime endere�o com s�mbolo+offset			*
 ****************************************************************
 */
void
psymoff (long addr, int type, const char *suffix)
{
	int		off;

	off = findsym (addr, type);

	if (off >= 0)
	{
		printf ("%s", cursym.name);
		if (off > 0)
			printf ("+0x%X", off);
	}
	else
	{
		printf ("0x%lX", addr);
	}

	if (suffix)
		printf ("%s", suffix);
}

/*
 ****************************************************************
 *	Procura s�mbolo por nome				*
 ****************************************************************
 */
int
lookup (const char *name, long *valuep)
{
	int		i;

	for (i = 0; i < nsyms; i++)
	{
		if (strcmp (symtab[i].name, name) == 0)
		{
			*valuep = symtab[i].value;
			return (1);
		}
	}

	return (0);
}

/*
 ****************************************************************
 *	Retorna nome do s�mbolo corrente			*
 ****************************************************************
 */
char *
symname (void)
{
	return (cursym.name);
}

/*
 ****************************************************************
 *	Retorna valor do s�mbolo corrente			*
 ****************************************************************
 */
long
symvalue (void)
{
	return (cursym.value);
}
