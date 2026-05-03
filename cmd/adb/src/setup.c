/*
 ****************************************************************
 *								*
 *			setup.c					*
 *								*
 *	Inicialização e leitura do a.out			*
 *								*
 ****************************************************************
 */

#include "../h/defs.h"

/*
 ****************************************************************
 *	Lê o arquivo a.out e configura mapeamento		*
 ****************************************************************
 */
void
setsym (void)
{
	fsym = getfile (symfil, O_RDONLY);

	if (fsym < 0)
		return;

	/*
	 *	Lê o cabeçalho
	 */
	if (read (fsym, &hdr, sizeof (HEADER)) != sizeof (HEADER))
	{
		fprintf (stderr, "adb: Não consegui ler cabeçalho de \"%s\"\n", symfil);
		close (fsym);
		fsym = -1;
		return;
	}

	/*
	 *	Verifica magic number
	 */
	if (hdr.h_magic != FMAGIC && hdr.h_magic != NMAGIC && hdr.h_magic != SMAGIC)
	{
		fprintf (stderr, "adb: \"%s\" não é um executável válido (magic=%04o)\n",
			symfil, hdr.h_magic);
		hdr.h_magic = 0;
	}

	/*
	 *	Configura o mapeamento do arquivo
	 *
	 *	Segmento 1: TEXT
	 *	Segmento 2: DATA
	 */
	txtmap.fd = fsym;
	txtmap.b1 = hdr.h_tstart;			/* Início do text */
	txtmap.e1 = hdr.h_tstart + hdr.h_tsize;		/* Fim do text */
	txtmap.f1 = sizeof (HEADER);			/* Offset no arquivo */

	txtmap.b2 = hdr.h_dstart;			/* Início do data */
	txtmap.e2 = hdr.h_dstart + hdr.h_dsize;		/* Fim do data */
	txtmap.f2 = sizeof (HEADER) + hdr.h_tsize;	/* Offset no arquivo */

	printf ("\"%s\": magic=%04o text=%ld data=%ld bss=%ld entry=0x%lX\n",
		symfil, hdr.h_magic, hdr.h_tsize, hdr.h_dsize,
		hdr.h_bsize, hdr.h_entry);
}

/*
 ****************************************************************
 *	Lê o arquivo core e configura mapeamento		*
 ****************************************************************
 */
void
setcor (void)
{
	fcor = getfile (corfil, O_RDONLY);

	if (fcor < 0)
		return;

	/*
	 *	Core file format:
	 *	UPROC + DATA + STACK
	 *
	 *	Por enquanto, apenas lê como dados brutos
	 */
	datmap.fd = fcor;
	datmap.b1 = 0;
	datmap.e1 = 0x7FFFFFFF;	/* Aceita qualquer endereço */
	datmap.f1 = 0;
	datmap.b2 = 0;
	datmap.e2 = 0;
	datmap.f2 = 0;

	printf ("core: \"%s\"\n", corfil);
}

/*
 ****************************************************************
 *	Abre um arquivo						*
 ****************************************************************
 */
int
getfile (const char *name, int mode)
{
	int		fd;

	if (name == NOSTR || name[0] == '-')
		return (-1);

	if ((fd = open (name, mode)) < 0)
	{
		if (errno != ENOENT)
			fprintf (stderr, "adb: Não consegui abrir \"%s\"\n", name);
		return (-1);
	}

	return (fd);
}
