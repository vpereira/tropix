/*
 ****************************************************************
 *								*
 *			h/machine.h				*
 *								*
 *	Definições específicas do i386				*
 *								*
 ****************************************************************
 */

/*
 ******	Instrução de breakpoint (INT3) **************************
 */
#define	BPT		0xCC

/*
 ******	Tamanho da página ***************************************
 */
#define	PGSIZE		4096

/*
 ******	Registradores do i386 ***********************************
 *
 *	Offsets na UPROC (após trap/interrupt)
 */
#define	NREGS		16

typedef struct
{
	char	*rname;		/* Nome do registrador */
	int	roffs;		/* Offset na UPROC */

}	REGLIST;

/*
 *	Tabela de registradores (definida em pcs.c)
 */
extern REGLIST	reglist[];

/*
 ******	Ptrace requests *****************************************
 */
#define	PT_SETTRC	0	/* Filho pede trace */
#define	PT_RIUSER	1	/* Lê memória (I space) */
#define	PT_RDUSER	2	/* Lê memória (D space) */
#define	PT_RUREGS	3	/* Lê UPROC */
#define	PT_WIUSER	4	/* Escreve text */
#define	PT_WDUSER	5	/* Escreve data */
#define	PT_WUREGS	6	/* Escreve UPROC */
#define	PT_CONTIN	7	/* Continua execução */
#define	PT_EXIT		8	/* Termina filho */
#define	PT_SINGLE	9	/* Single step */
