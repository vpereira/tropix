/*
 ****************************************************************
 *																*
 *			vic.c												*
 *																*
 *	Editor de texto vic (versao compacta do vi)					*
 *																*
 *	Versao	1.0.0, de 03.02.26									*
 *																*
 *	Modulo: vic													*
 *		Utilitarios Basicos										*
 *		Categoria B												*
 *																*
 *	TROPIX: Sistema Operacional Tempo-Real Multiprocessado		*
 *		Copyright (C) 2026 NCE/UFRJ								*
 *																*
 *	Baseado em tiny vi.c de Sterling Huxley						*
 *	Licensed under the GPL v2 or later							*
 *																*
 ****************************************************************
 */

/*
 ****************************************************************
 *	Includes para TROPIX										*
 ****************************************************************
 */
#include <sys/types.h>
#include <sys/syscall.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <setjmp.h>
#include <stat.h>
#include <termio.h>
#include <terminfo.h>

/*
 ****************************************************************
 *	Definicoes de configuracao									*
 ****************************************************************
 */
#define	BB_VER			"versao 1.0 TROPIX"
#define	BB_BT			"NCE/UFRJ"

#define	CONFIG_FEATURE_VI_MAX_LEN	4096


/*
 ****************************************************************
 *	Macros auxiliares											*
 ****************************************************************
 */
#define	ALIGN1
#define	FALSE			0
#define	TRUE			1
#define	NOSTR			(char *)NULL

#define	ARRAY_SIZE(x)		((unsigned)(sizeof(x) / sizeof((x)[0])))

#undef	isdigit
#define	isdigit(a)		((unsigned)((a) - '0') <= 9)

#define	Isprint(c)		((unsigned char)(c) >= ' ' && (c) != 0x7f && (unsigned char)(c) != 0x9b)

#define	bb_show_usage()
#define	bb_perror_msg(msg)	perror(msg)
#define	bb_putchar(c)		putc((c), stdout)

#define	INIT_G()

typedef signed char	smallint;
typedef unsigned char	uchar;
typedef struct stat	STAT;

/*
 ****************************************************************
 *	Constantes													*
 ****************************************************************
 */
enum
{
	MAX_TABSTOP = 32,
	MAX_INPUT_LEN = 128,
	MAX_SCR_COLS = CONFIG_FEATURE_VI_MAX_LEN,
	MAX_SCR_ROWS = CONFIG_FEATURE_VI_MAX_LEN
};

#define	VI_K_UP			(char)128
#define	VI_K_DOWN		(char)129
#define	VI_K_RIGHT		(char)130
#define	VI_K_LEFT		(char)131
#define	VI_K_HOME		(char)132
#define	VI_K_END		(char)133
#define	VI_K_INSERT		(char)134
#define	VI_K_DELETE		(char)135
#define	VI_K_PAGEUP		(char)136
#define	VI_K_PAGEDOWN		(char)137
#define	VI_K_FUN1		(char)138
#define	VI_K_FUN2		(char)139
#define	VI_K_FUN3		(char)140
#define	VI_K_FUN4		(char)141
#define	VI_K_FUN5		(char)142
#define	VI_K_FUN6		(char)143
#define	VI_K_FUN7		(char)144
#define	VI_K_FUN8		(char)145
#define	VI_K_FUN9		(char)146
#define	VI_K_FUN10		(char)147
#define	VI_K_FUN11		(char)148
#define	VI_K_FUN12		(char)149

#define	SOlen			4
static const char	SOs[] ALIGN1 = "\033[7m";
static const char	SOn[] ALIGN1 = "\033[0m";
static const char	bell[] ALIGN1 = "\007";
static const char	Ceol[] ALIGN1 = "\033[0K";
static const char	Ceos[] ALIGN1 = "\033[0J";
static const char	CMrc[] ALIGN1 = "\033[%d;%dH";

static INFO		info;
static const char	*ti_clear;
static const char	*ti_cup;
static const char	*ti_ed;
static const char	*ti_el;
static const char	*ti_rev;
static const char	*ti_sgr0;
static int		terminfo_ok;
static int		window_changed;

enum
{
	YANKONLY = FALSE,
	YANKDEL = TRUE,
	FORWARD = 1,
	BACK = -1,
	LIMITED = 0,
	FULL = 1,
	S_BEFORE_WS = 1,
	S_TO_WS = 2,
	S_OVER_WS = 3,
	S_END_PUNCT = 4,
	S_END_ALNUM = 5
};

enum
{
	CMODE_COMMAND,
	CMODE_INSERT,
	CMODE_REPLACE,
	CMODES,
	CMODE_LINE_INPUT = 1<<4
};

static const char	*cmd_mode_indicator[] = { "COMANDO", "INSERIR", "SUBSTITUIR", "?!?" };

#define	EDIT_STATUS	"%s: %s%s%s linha %d/%d %d%%"
#define	STATUS_BUFFER_LEN	200
/*
 ****************************************************************
 *	Estrutura global											*
 ****************************************************************
 */
struct globals
{
	char		*text, *end;
	char		*dot;
	int		text_size;

	smallint	vi_setops;

	smallint	readonly_mode;

	smallint	editing;
	smallint	cmd_mode;
	int		file_modified;
	int		last_file_modified;
	int		fn_start;
	int		save_argc;
	int		cmdcnt;
	unsigned	rows, columns;
	int		crow, ccol;
	int		offset;
	char		*current_filename;
	char		*screenbegin;
	char		*screen;
	int		screensize;
	int		tabstop;
	char		erase_char;
	char		last_input_char;
	char		last_forward_char;

	smallint	adding2q;
	int		lmc_len;
	char		*ioq, *ioq_start;
	int		my_pid;
	char		*modifying_cmds;
	char		*last_search_pattern;
	int		chars_to_parse;
	char		*edit_file__cur_line;
	int		refresh__old_offset;
	int		format_edit_status__tot;

	int		YDreg, Ureg;
	char		*reg[28];
	char		*mark[28];
	char		*context_start, *context_end;
	jmp_buf		restart;
	TERMIO		term_orig, term_vi;
	unsigned	ticsPerChar;
	char		*initial_cmds[3];
	char		readbuffer[128];
	char		status_buffer[STATUS_BUFFER_LEN];
	char		displayed_buffer[STATUS_BUFFER_LEN];
	int		have_status_msg;
	char		last_modifying_cmd[MAX_INPUT_LEN];
	char		get_input_line__buf[MAX_INPUT_LEN];
	char		scr_out_buf[MAX_SCR_COLS + MAX_TABSTOP * 2];
};

struct globals	G;

#define	VI_ERR_METHOD	8
#define	VI_NUMBER	16
#define	err_method	(vi_setops & VI_ERR_METHOD)
#define	number_mode	(vi_setops & VI_NUMBER)
#define	SET_READONLY_FILE(flags)	((flags) |= 0x01)
#define	SET_READONLY_MODE(flags)	((flags) |= 0x02)
#define	UNSET_READONLY_FILE(flags)	((flags) &= 0xfe)

#define	text			(G.text)
#define	text_size		(G.text_size)
#define	end			(G.end)
#define	dot			(G.dot)
#define	reg			(G.reg)
#define	vi_setops		(G.vi_setops)
#define	editing			(G.editing)
#define	cmd_mode		(G.cmd_mode)
#define	file_modified		(G.file_modified)
#define	last_file_modified	(G.last_file_modified)
#define	fn_start		(G.fn_start)
#define	save_argc		(G.save_argc)
#define	cmdcnt			(G.cmdcnt)
#define	rows			(G.rows)
#define	columns			(G.columns)
#define	crow			(G.crow)
#define	ccol			(G.ccol)
#define	offset			(G.offset)
#define	status_buffer		(G.status_buffer)
#define	displayed_buffer	(G.displayed_buffer)
#define	have_status_msg		(G.have_status_msg)
#define	current_filename	(G.current_filename)
#define	screen			(G.screen)
#define	screensize		(G.screensize)
#define	screenbegin		(G.screenbegin)
#define	tabstop			(G.tabstop)
#define	erase_char		(G.erase_char)
#define	last_input_char		(G.last_input_char)
#define	last_forward_char	(G.last_forward_char)
#define	readonly_mode		(G.readonly_mode)
#define	adding2q		(G.adding2q)
#define	lmc_len			(G.lmc_len)
#define	ioq			(G.ioq)
#define	ioq_start		(G.ioq_start)
#define	my_pid			(G.my_pid)
#define	modifying_cmds		(G.modifying_cmds)
#define	last_search_pattern	(G.last_search_pattern)
#define	chars_to_parse		(G.chars_to_parse)
#define	edit_file__cur_line	(G.edit_file__cur_line)
#define	refresh__old_offset	(G.refresh__old_offset)
#define	format_edit_status__tot	(G.format_edit_status__tot)
#define	YDreg			(G.YDreg)
#define	Ureg			(G.Ureg)
#define	mark			(G.mark)
#define	context_start		(G.context_start)
#define	context_end		(G.context_end)
#define	restart			(G.restart)
#define	term_orig		(G.term_orig)
#define	ticsPerChar		(G.ticsPerChar)
#define	term_vi			(G.term_vi)
#define	initial_cmds		(G.initial_cmds)
#define	readbuffer		(G.readbuffer)
#define	scr_out_buf		(G.scr_out_buf)
#define	last_modifying_cmd	(G.last_modifying_cmd)
#define	get_input_line__buf	(G.get_input_line__buf)

/*
 ****************************************************************
 *	Prototipos de funcoes										*
 ****************************************************************
 */
static int	init_text_buffer (char *);
static void	edit_file (char **);
static int	next_tabstop (int);
static void	sync_cursor (char *, int *, int *);
static char	*begin_line (char *);
static char	*end_line (char *);
static char	*prev_line (char *);
/* Navegacao no texto */
static char	*begin_line (char *);
static char	*end_line (char *);
static char	*next_line (char *);
static char	*prev_line (char *);
static int	count_lines (char *, char *);
static char	*find_line (int);
static void	sync_cursor (char *, int *, int *);

/* Manipulacao de texto */
static char	*char_insert (char *, char);
static char	*text_hole_delete (char *, char *);
static char	*text_hole_make (char *, int);

/* Terminal e tela */
static void	show_help (void);
static int	rawmode (void);
static void	cookmode (void);
static int	awaitInput (int);
static int	get_one_char (void);
static void	place_cursor (int, int, int);
static void	screen_erase (void);
static char	*new_screen (int, int);
static void	clear_to_eol (void);
static void	clear_to_eos (void);
static void	standout_start (void);
static void	standout_end (void);
static void	flash (int);
static void	status_line (const char *, ...);
static void	status_line_bold (const char *, ...);
static void	redraw (int);
static void	format_line (int, char *, int, int, int, int);
static void	refresh (int);
static void	Indicate_Error (void);
#define	indicate_error(c)	Indicate_Error ()
static void	Hit_Return (void);
static void	init_terminfo (void);
static void	update_screen_size (void);
static void	draw_status_line (void);

/* Arquivo */
static int	file_insert (const char *, char *);
static int	file_write (const char *, char *, char *);
static void	edit_file_by_name (const char *);

/* Comandos */
static void	do_cmd (int);
static void	do_insert (int);

static char	*search (char *, char *, int);
static const char *get_one_address (const char *, int *);
static const char *get_address (const char *, int *, int *);
static void	colon (const char *);
static void	winch_sig (int, ...);
static void	catch_sig (int, ...);
static void	quit_sig (int, ...);
/* TROPIX nao tem SIGTSTP/SIGCONT - suspend_sig removida */

/*
 ****************************************************************
 *	Funcoes auxiliares											*
 ****************************************************************
 */
static char *
strchrnul (const char *s, int c_in)
{
	char		c = c_in;

	while (*s && (*s != c))
		s++;

	return ((char *)s);

}	/* end strchrnul */

static void *
memrchr (const void *s, int c_in, int n)
{
	const unsigned char	*cp;

	if (n != 0)
	{
		cp = (const unsigned char *)s + n;

		do
		{
			if (*(--cp) == (unsigned char)c_in)
				return ((void *)cp);
		}
		while (--n != 0);
	}

	return (NOSTR);

}	/* end memrchr */

static void *
xmalloc (int size)
{
	void		*ptr;

	ptr = malloc (size);

	if (ptr != NULL)
		return (ptr);

	fprintf (stderr, "vi: Memoria esgotada\n");
	exit (1);

}	/* end xmalloc */

static void *
xzalloc (int size)
{
	return (memset (xmalloc (size), 0, size));

}	/* end xzalloc */

static void *
xstrdup (const char *s)
{
	void		*ptr;

	ptr = strdup (s);

	if (ptr != NULL)
		return (ptr);

	fprintf (stderr, "vi: Memoria esgotada\n");
	exit (1);

}	/* end xstrdup */

static void *
xrealloc (void *old, int size)
{
	void		*ptr;

	ptr = realloc (old, size);

	if (ptr != NULL)
		return (ptr);

	fprintf (stderr, "vi: Memoria esgotada\n");
	exit (1);

}	/* end xrealloc */

static char *
last_char_is (const char *s, int c)
{
	int		sz;

	if (s && *s)
	{
		sz = strlen (s) - 1;
		s += sz;

		if ((unsigned char)*s == c)
			return ((char *)s);
	}

	return (NOSTR);

}	/* end last_char_is */

static int
safe_read (int fd, void *buf, int count)
{
	int		n;

	do
	{
		n = read (fd, buf, count);
	}
	while (n < 0 && errno == EINTR);

	return (n);

}	/* end safe_read */

static int
safe_write (int fd, const void *buf, int count)
{
	int		n;

	do
	{
		n = write (fd, buf, count);
	}
	while (n < 0 && errno == EINTR);

	return (n);

}	/* end safe_write */

static int
full_write (int fd, const void *buf, int len)
{
	int		cc, total;

	total = 0;

	while (len > 0)
	{
		cc = safe_write (fd, buf, len);

		if (cc < 0)
		{
			if (total)
				return (total);
			return (cc);
		}

		total += cc;
		buf = (const char *)buf + cc;
		len -= cc;
	}

	return (total);

}	/* end full_write */

static void
write1 (const char *out)
{
	fputs (out, stdout);

}	/* end write1 */

/*
 ****************************************************************
 *	Controle do terminal										*
 ****************************************************************
 */
static int
rawmode (void)
{
	int		err, baud;

	err = ioctl (0, TCGETS, &term_orig);

	if (err)
		return (err);

	term_vi = term_orig;
	term_vi.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
	term_vi.c_iflag &= ~(IXON | ICRNL | INLCR | IGNCR);
	term_vi.c_oflag &= ~(ONLCR | OCRNL);
	term_vi.c_cc[VMIN] = 1;
	term_vi.c_cc[VTIME] = 0;
	erase_char = term_vi.c_cc[VERASE];

	ioctl (0, TCSETAW, &term_vi);

	baud = term_vi.c_cflag & CBAUD;
	ticsPerChar = 1;

	switch (baud)
	{
	    case B600:
		ticsPerChar = 2;
		break;
	    case B300:
		ticsPerChar = 4;
		break;
	    case B150:
		ticsPerChar = 7;
		break;
	    case B110:
		ticsPerChar = 10;
		break;
	}

	return (0);

}	/* end rawmode */

static void
cookmode (void)
{
	ioctl (0, TCSETAW, &term_orig);

}	/* end cookmode */

static int
awaitInput (int tics)
{
	int		fd_index[1];
	int		timeout;

	fflush (stdout);

	fd_index[0] = 0;
	timeout = tics * 10;

	return (attention (1, fd_index, 0, timeout) > 0);

}	/* end awaitInput */

/*
 ****************************************************************
 *	Tratadores de sinais										*
 ****************************************************************
 */
static void
quit_sig (int sig, ...)
{
	cookmode ();
	place_cursor (rows - 1, 0, FALSE);
	clear_to_eol ();
	fflush (stdout);
	signal (sig, SIG_DFL);
	kill (my_pid, sig);

}	/* end quit_sig */


static void
catch_sig (int sig, ...)
{
	signal (SIGINT, catch_sig);

	if (sig)
		longjmp (restart, sig);

}	/* end catch_sig */

static void
winch_sig (int sig, ...)
{
	signal (SIGWINCH, winch_sig);
	window_changed = 1;

}	/* end winch_sig */

/*
 ****************************************************************
 *	Funcoes de tela						*
 ****************************************************************
 */
static void
clear_screen (void)
{
	if (terminfo_ok && ti_clear)
	{
		write1 (ti_clear);
	}
	else
	{
		place_cursor (0, 0, FALSE);
		clear_to_eos ();
	}

}	/* end clear_screen */

static void
gracefulExit (void)
{
	cookmode ();
	place_cursor (rows - 1, 0, FALSE);
	clear_to_eol ();
	fflush (stdout);

}	/* end gracefulExit */

static void
clampScreenSize (void)
{
	if (rows < 2)
		rows = 2;
	else if (rows > MAX_SCR_ROWS)
		rows = MAX_SCR_ROWS;

	if (columns < 2)
		columns = 2;
	else if (columns > MAX_SCR_COLS)
		columns = MAX_SCR_COLS;

}	/* end clampScreenSize */

static void
update_screen_size (void)
{
	TERMIO		termio;

	if (ioctl (0, TCGETS, &termio) == 0)
	{
		if (termio.t_nline > 0)
			rows = termio.t_nline;

		if (termio.t_ncol > 0)
			columns = termio.t_ncol;
	}

}	/* end update_screen_size */

static void
createScreen (void)
{
	const char	*txt;

	update_screen_size ();

	txt = getenv ("LINES");

	if (txt && rows == 0)
		rows = atoi (txt);

	txt = getenv ("COLUMNS");

	if (txt && columns == 0)
		columns = atoi (txt);

	clampScreenSize ();
	new_screen (rows, columns);

}	/* end createScreen */

static char *
new_screen (int ro, int co)
{
	int		li;

	if (screen != NOSTR)
		free (screen);

	screensize = ro * co + 8;
	screen = xmalloc (screensize);
	screen_erase ();

	for (li = 1; li < ro - 1; li++)
		screen[(li * co) + 0] = '~';

	return (screen);

}	/* end new_screen */

static void
screen_erase (void)
{
	memset (screen, ' ', screensize);

}	/* end screen_erase */

static void
place_cursor (int row, int col, int optimize)
{
	if (terminfo_ok && ti_cup)
		write1 (parmexec (ti_cup, row, col));
	else
		printf (CMrc, row + 1, col + 1);

}	/* end place_cursor */

static void
clear_to_eol (void)
{
	if (terminfo_ok && ti_el)
		write1 (ti_el);
	else
		write1 (Ceol);

}	/* end clear_to_eol */

static void
clear_to_eos (void)
{
	if (terminfo_ok && ti_ed)
		write1 (ti_ed);
	else
		write1 (Ceos);

}	/* end clear_to_eos */

static void
standout_start (void)
{
	if (terminfo_ok && ti_rev)
		write1 (ti_rev);
	else
		write1 (SOs);

}	/* end standout_start */

static void
standout_end (void)
{
	if (terminfo_ok && ti_sgr0)
		write1 (ti_sgr0);
	else
		write1 (SOn);

}	/* end standout_end */

static void
flash (int h)
{
	standout_start ();
	redraw (TRUE);
	awaitInput (h);
	standout_end ();
	redraw (TRUE);

}	/* end flash */

static void
Indicate_Error (void)
{
	if (err_method)
		flash (10);
	else
		write1 (bell);

}	/* end Indicate_Error */

static void
init_terminfo (void)
{
	if (getinfo (NOSTR, &info) == 0)
	{
		terminfo_ok = 0;
		return;
	}

	ti_clear = info.i_strings[s_clear];
	ti_cup = info.i_strings[s_cup];
	ti_ed = info.i_strings[s_ed];
	ti_el = info.i_strings[s_el];
	ti_rev = info.i_strings[s_rev];
	ti_sgr0 = info.i_strings[s_sgr0];

	if (ti_clear == NOSTR || ti_cup == NOSTR || ti_ed == NOSTR || ti_el == NOSTR)
	{
		terminfo_ok = 0;
		return;
	}

	if (ti_clear[0] == '\0' || ti_cup[0] == '\0' || ti_ed[0] == '\0' || ti_el[0] == '\0')
	{
		terminfo_ok = 0;
		return;
	}

	if (ti_rev == NOSTR || ti_sgr0 == NOSTR)
	{
		ti_rev = "";
		ti_sgr0 = "";
	}

	terminfo_ok = 1;

}	/* end init_terminfo */

static void
show_help (void)
{
	fprintf (stderr, "vic %s (%s)\n\n", BB_VER, BB_BT);
	fprintf (stderr, "Uso: vic [-R] [-c cmd] arquivo ...\n");
	fprintf (stderr, "\nOpcoes:\n");
	fprintf (stderr, "  -R      Modo somente leitura\n");
	fprintf (stderr, "  -c cmd  Executa comando apos carregar arquivo\n");

}	/* end show_help */

static void
Hit_Return (void)
{
	int		c;

	standout_start ();
	write1 ("\n[Tecle ENTER]");
	standout_end ();

	while ((c = get_one_char ()) != '\n' && c != '\r')
		/* vazio */;

}	/* end Hit_Return */

/*
 ****************************************************************
 *	Leitura de caracteres					*
 ****************************************************************
 */
static int
get_one_char (void)
{
	int		c;
	char		buf[1];
	int		esc;

	fflush (stdout);

	if (readbuffer[0])
	{
		c = (uchar)readbuffer[0];
		memmove (readbuffer, readbuffer + 1, MAX_INPUT_LEN - 1);
		return (c);
	}

	if (safe_read (0, buf, 1) <= 0)
		return (-1);

	c = (uchar)buf[0];

	if (c != 0x1B)
		return (c);

	if (!awaitInput (ticsPerChar))
		return (c);

	if (safe_read (0, buf, 1) <= 0)
		return (c);

	if (buf[0] != '[' && buf[0] != 'O')
		return (c);

	if (!awaitInput (ticsPerChar))
		return (c);

	if (safe_read (0, buf, 1) <= 0)
		return (c);

	esc = (uchar)buf[0];

	switch (esc)
	{
	    case 'A':
		return ('k');
	    case 'B':
		return ('j');
	    case 'C':
		return ('l');
	    case 'D':
		return ('h');
	    case 'H':
		return ('0');
	    case 'F':
		return ('$');
	    case '5':	/* Page up */
		if (awaitInput (ticsPerChar))
			safe_read (0, buf, 1);
		return (0x02);
	    case '6':	/* Page down */
		if (awaitInput (ticsPerChar))
			safe_read (0, buf, 1);
		return (0x06);
	    case '7':	/* Home */
		if (awaitInput (ticsPerChar))
			safe_read (0, buf, 1);
		return ('0');
	    case '8':	/* End */
		if (awaitInput (ticsPerChar))
			safe_read (0, buf, 1);
		return ('$');
	}

	return (c);

}	/* end get_one_char */

/*
 ****************************************************************
 *	Redesenha a tela					*
 ****************************************************************
 */
static void
redraw (int full)
{
	if (full)
		clear_screen ();

	refresh (full);

}	/* end redraw */

/*
 ****************************************************************
 *	Atualiza a tela						*
 ****************************************************************
 */
static void
refresh (int full)
{
	int		li, changed;
	char		*tp, *sp;
	int		show_number;
	int		numw;
	int		base_line;
	int		total_lines;

	if (text == NULL || screen == NULL)
		return;

	if (window_changed)
	{
		window_changed = 0;
		createScreen ();
	}

	if (full)
		screen_erase ();

	show_number = number_mode ? 1 : 0;
	numw = 0;
	base_line = 0;
	offset = 0;

	if (show_number)
	{
		total_lines = count_lines (text, end - 1) + 1;

		if (total_lines <= 0)
			total_lines = 1;

		numw = 1;

		while (total_lines >= 10)
		{
			total_lines /= 10;
			numw++;
		}

		if (numw + 1 >= columns)
			show_number = 0;
		else
			base_line = count_lines (text, screenbegin);
	}

	if (show_number)
		offset = numw + 1;

	/* Encontra a primeira linha visivel */
	sync_cursor (dot, &crow, &ccol);

	/* Redesenha cada linha */
	for (li = 0; li < rows - 1; li++)
	{
		tp = screenbegin;

		/* Avanca ate a linha li */
		for (changed = 0; changed < li && tp < end; changed++)
		{
			tp = next_line (tp);
		}

		if (tp >= end)
		{
			/* Linha alem do texto - limpa */
			if (screen[li * columns] != '~' || show_number)
			{
				format_line (li, NOSTR, show_number, numw, 0, 1);
			}
		}
		else
		{
			/* Mostra a linha */
			format_line (li, tp, show_number, numw, base_line + li + 1, 0);
		}
	}

	if (have_status_msg)
		have_status_msg = 0;
	else
		draw_status_line ();

	/* Posiciona o cursor */
	place_cursor (crow, ccol, TRUE);
	fflush (stdout);

}	/* end refresh */

/*
 ****************************************************************
 *	Formata uma linha para exibicao				*
 ****************************************************************
 */
static void
format_line (int li, char *src, int show_number, int numw, int line_no, int tilde)
{
	int		co, i;
	char		c, *dst;
	char		buf[MAX_SCR_COLS + 2];
	char		numbuf[16];

	dst = buf;
	co = 0;

	if (show_number)
	{
		if (tilde)
		{
			for (i = 0; i < numw + 1 && co < columns; i++)
			{
				*dst++ = ' ';
				co++;
			}
		}
		else
		{
			snprintf (numbuf, sizeof (numbuf), "%*d ", numw, line_no);
			for (i = 0; numbuf[i] != '\0' && co < columns; i++)
			{
				*dst++ = numbuf[i];
				co++;
			}
		}
	}

	if (tilde)
	{
		if (co < columns)
		{
			*dst++ = '~';
			co++;
		}
	}
	else
	{
		while (co < columns - 1)
		{
			c = *src;

			if (c == '\n' || c == '\0')
				break;

			if (c < ' ' || c == 0x7F)
			{
				if (co < columns - 2)
				{
					*dst++ = '^';
					*dst++ = (c == 0x7F) ? '?' : (c + '@');
					co += 2;
				}
				else
				{
					break;
				}
			}
			else if (c == '\t')
			{
				do
				{
					*dst++ = ' ';
					co++;

				}	while ((co % tabstop) && co < columns - 1);
			}
			else
			{
				*dst++ = c;
				co++;
			}

			src++;
		}
	}

	/* Preenche com espacos */
	while (co < columns)
	{
		*dst++ = ' ';
		co++;
	}

	*dst = '\0';

	/* Verifica se mudou */
	if (memcmp (buf, &screen[li * columns], columns) != 0)
	{
		place_cursor (li, 0, FALSE);
		write (1, buf, columns);
		memcpy (&screen[li * columns], buf, columns);
	}

}	/* end format_line */

static void
draw_status_line (void)
{
	const char	*fn;
	const char	*ro;
	const char	*mod;
	const char	*ins;
	int		cur_line;
	int		total_lines;
	int		percent;
	char		buf[STATUS_BUFFER_LEN];

	if (rows < 2)
		return;

	fn = current_filename ? current_filename : "[Sem nome]";
	ro = readonly_mode ? " [R]" : "";
	mod = file_modified ? " [+]" : "";
	ins = cmd_mode ? " (I)" : "";

	cur_line = count_lines (text, dot) + 1;
	total_lines = count_lines (text, end - 1) + 1;

	if (total_lines <= 0)
		total_lines = 1;

	percent = (cur_line * 100) / total_lines;

	snprintf (buf, sizeof (buf), EDIT_STATUS,
		fn, ro, mod, ins, cur_line, total_lines, percent);

	place_cursor (rows - 1, 0, FALSE);
	standout_start ();
	clear_to_eol ();
	write1 (buf);
	standout_end ();
}

/*
 ****************************************************************
 *	Sincroniza cursor com a posicao do texto		*
 ****************************************************************
 */
static void
sync_cursor (char *p, int *row, int *col)
{
	char		*beg, *tp;
	int		li, co, c;

	/* Garante que p esta no texto */
	if (p < text)
		p = text;

	if (p > end - 1)
		p = end - 1;

	/* Encontra o inicio da linha */
	beg = begin_line (p);

	/* Ajusta screenbegin se necessario */
	if (beg < screenbegin)
	{
		screenbegin = beg;
	}
	else
	{
		/* Verifica se esta alem da tela */
		tp = screenbegin;

		for (li = 0; li < rows - 1 && tp < end; li++)
		{
			if (tp == beg)
				break;

			tp = next_line (tp);
		}

		if (li >= rows - 1)
		{
			/* Fora da tela - ajusta */
			screenbegin = beg;

			for (li = 0; li < rows / 2 && screenbegin > text; li++)
			{
				screenbegin = prev_line (screenbegin);
			}
		}
	}

	/* Calcula a linha relativa */
	tp = screenbegin;
	li = 0;

	while (tp < beg && li < rows - 1)
	{
		tp = next_line (tp);
		li++;
	}

	/* Calcula a coluna */
	co = 0;

	for (tp = beg; tp < p; tp++)
	{
		c = *tp;

		if (c == '\t')
			co = ((co / tabstop) + 1) * tabstop;
		else if (c < ' ' || c == 0x7F)
			co += 2;
		else
			co++;
	}

	*row = li;
	*col = co + offset;

	if (*col >= (int)columns)
		*col = columns - 1;

}	/* end sync_cursor */

/*
 ****************************************************************
 *	Funcoes de navegacao no texto				*
 ****************************************************************
 */
static char *
begin_line (char *p)
{
	if (p < text)
		return (text);

	while (p > text && p[-1] != '\n')
		p--;

	return (p);

}	/* end begin_line */

static char *
end_line (char *p)
{
	if (p >= end)
		return (end - 1);

	while (p < end - 1 && *p != '\n')
		p++;

	return (p);

}	/* end end_line */

static char *
next_line (char *p)
{
	p = end_line (p);

	if (p < end - 1)
		p++;

	return (p);

}	/* end next_line */

static char *
prev_line (char *p)
{
	p = begin_line (p);

	if (p > text)
	{
		p--;
		p = begin_line (p);
	}

	return (p);

}	/* end prev_line */

/*
 ****************************************************************
 *	Funcoes de manipulacao de texto				*
 ****************************************************************
 */
static char *
text_hole_make (char *p, int size)
{
	int		bias;

	if (size <= 0)
		return (p);

	end += size;

	if (end >= (text + text_size))
	{
		/* Precisa aumentar o buffer */
		bias = p - text;
		text_size += size + 8192;
		text = xrealloc (text, text_size);
		p = text + bias;
		end = text + text_size;
	}

	memmove (p + size, p, end - p - size);

	memset (p, ' ', size);

	file_modified++;

	return (p);

}	/* end text_hole_make */

static char *
text_hole_delete (char *p, char *q)
{
	int		size;
	char		*tmp;

	if (q < p)
	{
		tmp = p;
		p = q;
		q = tmp;
	}

	size = q - p + 1;

	if (size <= 0)
		return (p);

	if (p < text)
		p = text;

	if (q >= end)
		q = end - 1;

	/* Ajusta marcadores */
	{
		int	i;

		for (i = 0; i < 28; i++)
		{
			if (mark[i] >= p && mark[i] <= q)
				mark[i] = NULL;
			else if (mark[i] > q)
				mark[i] -= size;
		}
	}

	if (dot >= p && dot <= q)
		dot = p;
	else if (dot > q)
		dot -= size;

	memmove (p, q + 1, end - q - 1);

	end -= size;

	file_modified++;

	return (p);

}	/* end text_hole_delete */

static char *
char_insert (char *p, char c)
{
	if (c == 0x7F)
		c = ' ';

	p = text_hole_make (p, 1);
	*p = c;

	return (p);

}	/* end char_insert */

/*
 ****************************************************************
 *	Comandos colon						*
 ****************************************************************
 */
static const char *
get_one_address (const char *p, int *addr)
{
	int		n;

	*addr = -1;

	if (*p == '.')
	{
		p++;
		*addr = count_lines (text, dot);
	}
	else if (*p == '$')
	{
		p++;
		*addr = count_lines (text, end - 1);
	}
	else if (*p == '\'')
	{
		p++;

		if (*p >= 'a' && *p <= 'z')
		{
			if (mark[*p - 'a'])
				*addr = count_lines (text, mark[*p - 'a']);
		}

		p++;
	}
	else if (*p >= '0' && *p <= '9')
	{
		n = 0;

		while (*p >= '0' && *p <= '9')
		{
			n = n * 10 + (*p - '0');
			p++;
		}

		*addr = n;
	}

	return (p);

}	/* end get_one_address */

static const char *
get_address (const char *p, int *beg, int *end_addr)
{
	*beg = *end_addr = -1;

	while (*p == ' ' || *p == '\t')
		p++;

	p = get_one_address (p, beg);

	if (*p == ',')
	{
		p++;
		p = get_one_address (p, end_addr);
	}
	else
	{
		*end_addr = *beg;
	}

	return (p);

}	/* end get_address */

static int
count_lines (char *start, char *stop)
{
	char		*q;
	int		cnt;

	if (stop < start)
	{
		char *tmp = start;
		start = stop;
		stop = tmp;
	}

	cnt = 0;

	for (q = start; q <= stop && q < end; q++)
	{
		if (*q == '\n')
			cnt++;
	}

	return (cnt);

}	/* end count_lines */

static char *
find_line (int n)
{
	char		*p;

	p = text;

	while (n > 1 && p < end)
	{
		if (*p++ == '\n')
			n--;
	}

	return (p);

}	/* end find_line */

static void
colon (const char *buf)
{
	char		c;
	const char	*p, *q;
	const char	*fn;
	int		b, e, li;
	int		useforce;
	int		wq;

	if (buf == NULL || *buf == '\0')
		return;

	p = buf;

	/* Remove espacos iniciais */
	while (*p == ' ' || *p == '\t')
		p++;

	/* Pega enderecos */
	p = get_address (p, &b, &e);

	while (*p == ' ' || *p == '\t')
		p++;

	/* Comando */
	wq = 0;
	c = *p++;

	if (c == 'w' && (*p == 'q' || *p == 'Q'))
	{
		wq = 1;
		p++;
	}

	useforce = 0;

	if (*p == '!')
	{
		useforce = 1;
		p++;
	}

	while (*p == ' ' || *p == '\t')
		p++;

	switch (c)
	{
	    case 'q':	/* quit */
		if (file_modified && !useforce)
		{
			status_line_bold ("Arquivo modificado (use :q!)");
		}
		else
		{
			editing = 0;
		}
		break;

	    case 'w':	/* write */
		fn = p;

		if (*fn == '\0')
			fn = current_filename;

		if (fn == NULL || *fn == '\0')
		{
			status_line_bold ("Sem nome de arquivo");
			break;
		}

		li = file_write (fn, text, end - 1);

		if (li < 0)
		{
			status_line_bold ("Erro gravando \"%s\"", fn);
		}
		else
		{
			status_line ("\"%s\" %dL, %dC",
				fn, count_lines (text, end - 1), (int)(end - text - 1));
			file_modified = 0;

			if (current_filename == NULL)
				current_filename = xstrdup (fn);
		}

		if (wq)
			editing = 0;
		break;

	    case 'x':	/* save and exit */
		fn = current_filename;

		if (fn == NULL)
		{
			status_line_bold ("Sem nome de arquivo");
			break;
		}

		li = file_write (fn, text, end - 1);

		if (li < 0)
		{
			status_line_bold ("Erro gravando");
			break;
		}

		file_modified = 0;
		editing = 0;
		break;

	    case 'e':	/* edit file */
		if (file_modified && !useforce)
		{
			status_line_bold ("Arquivo modificado (use :e!)");
			break;
		}

		fn = p;

		if (*fn == '\0')
			fn = current_filename;

		if (fn == NULL || *fn == '\0')
		{
			status_line_bold ("Sem nome de arquivo");
			break;
		}

		/* Carrega novo arquivo */
		edit_file_by_name (fn);
		break;

	    case 'r':	/* read file */
		fn = p;

		if (*fn == '\0')
		{
			status_line_bold ("Uso: :r arquivo");
			break;
		}

		li = file_insert (fn, dot);

		if (li < 0)
		{
			status_line_bold ("Erro lendo \"%s\"", fn);
		}
		else
		{
			status_line ("\"%s\" %dC", fn, li);
		}
		break;

	    case 's':	/* substitute */
		if (p[0] == 'e' && p[1] == 't')
		{
			p += 2;

			while (*p == ' ' || *p == '\t')
				p++;

			if (strcmp (p, "number") == 0 || strcmp (p, "nu") == 0)
			{
				vi_setops |= VI_NUMBER;
				status_line ("set number");
				redraw (TRUE);
			}
			else if (strcmp (p, "nonumber") == 0 || strcmp (p, "nonu") == 0)
			{
				vi_setops &= ~VI_NUMBER;
				status_line ("set nonumber");
				redraw (TRUE);
			}
			else
			{
				status_line_bold ("Opcao desconhecida: %s", p);
			}
		}
		else
		{
			/* Implementacao simplificada */
			status_line_bold ("Comando :s nao implementado");
		}
		break;

	    case '!':	/* shell command */
		q = p;

		if (*q)
		{
			cookmode ();
			printf ("\n");
			system (q);
			Hit_Return ();
			rawmode ();
			redraw (TRUE);
		}
		break;

	    case '\0':	/* go to line */
		if (b >= 0)
		{
			dot = find_line (b);
			dot = begin_line (dot);
		}
		break;

	    default:
		status_line_bold ("Comando nao reconhecido: %c", c);
		break;
	}

}	/* end colon */

/*
 ****************************************************************
 *	Linha de status						*
 ****************************************************************
 */
static void
status_line (const char *fmt, ...)
{
	va_list		ap;

	va_start (ap, fmt);
	vsnprintf (status_buffer, MAX_SCR_COLS, fmt, ap);
	va_end (ap);

	have_status_msg = 1;

	place_cursor (rows - 1, 0, FALSE);
	clear_to_eol ();
	write1 (status_buffer);
	place_cursor (crow, ccol, TRUE);

}	/* end status_line */

static void
status_line_bold (const char *fmt, ...)
{
	va_list		ap;

	va_start (ap, fmt);
	vsnprintf (status_buffer, MAX_SCR_COLS, fmt, ap);
	va_end (ap);

	have_status_msg = 2;

	place_cursor (rows - 1, 0, FALSE);
	standout_start ();
	clear_to_eol ();
	write1 (status_buffer);
	standout_end ();
	place_cursor (crow, ccol, TRUE);

}	/* end status_line_bold */

/*
 ****************************************************************
 *	Leitura e gravacao de arquivo				*
 ****************************************************************
 */
static int
file_write (const char *fn, char *first, char *last)
{
	int		fd, cnt;

	fd = creat (fn, 0644);

	if (fd < 0)
		return (-1);

	cnt = last - first + 1;

	if (write (fd, first, cnt) != cnt)
	{
		close (fd);
		return (-1);
	}

	close (fd);

	return (cnt);

}	/* end file_write */

static int
file_insert (const char *fn, char *p)
{
	int		fd, cnt, size;
	STAT		sb;
	char		*buf;

	fd = open (fn, O_RDONLY);

	if (fd < 0)
		return (-1);

	if (fstat (fd, &sb) < 0)
	{
		close (fd);
		return (-1);
	}

	size = sb.st_size;

	if (size <= 0)
	{
		close (fd);
		return (0);
	}

	p = text_hole_make (p, size);

	cnt = read (fd, p, size);

	close (fd);

	if (cnt != size)
	{
		/* Ajusta se leu menos */
		text_hole_delete (p + cnt, p + size - 1);
	}

	return (cnt);

}	/* end file_insert */

static void
edit_file_by_name (const char *fn)
{
	int		cnt;
	STAT		sb;

	/* Libera texto anterior */
	if (text != NULL)
	{
		free (text);
		text = NULL;
	}

	if (current_filename != NULL && current_filename != fn)
	{
		free (current_filename);
		current_filename = NULL;
	}

	current_filename = xstrdup (fn);

	/* Inicializa buffer */
	text_size = 8192;
	text = xzalloc (text_size);
	end = text;

	/* Carrega arquivo */
	if (stat (fn, &sb) >= 0 && sb.st_size > 0)
	{
		cnt = file_insert (fn, text);

		if (cnt >= 0)
		{
			status_line ("\"%s\" %dL, %dC",
				fn, count_lines (text, end - 1), cnt);
		}
		else
		{
			status_line_bold ("\"%s\" [Novo arquivo]", fn);
		}
	}
	else
	{
		/* Arquivo novo ou vazio */
		text_hole_make (text, 1);
		*text = '\n';
		status_line ("\"%s\" [Novo arquivo]", fn);
	}

	dot = text;
	screenbegin = text;
	file_modified = 0;

	createScreen ();
	redraw (TRUE);

}	/* end edit_file_by_name */

/*
 ****************************************************************
 *	Processamento de comandos				                    *
 ****************************************************************
 */
static void
do_cmd (int c)
{
	char		*p, *q;
	int		n, dir;

	switch (c)
	{
	    /* Movimento */
	    case 'h':
	    case '\b':
	    case 0x7F:
		if (dot > begin_line (dot))
			dot--;
		break;

	    case 'l':
	    case ' ':
		if (dot < end_line (dot))
			dot++;
		break;

	    case 'j':
	    case '\n':
		p = next_line (dot);

		if (p < end)
		{
			n = dot - begin_line (dot);
			dot = p;
			q = end_line (dot);

			while (n > 0 && dot < q)
			{
				dot++;
				n--;
			}
		}
		break;

	    case 'k':
		if (dot > text)
		{
			p = prev_line (dot);
			n = dot - begin_line (dot);
			dot = p;
			q = end_line (dot);

			while (n > 0 && dot < q)
			{
				dot++;
				n--;
			}
		}
		break;

	    case '0':
		dot = begin_line (dot);
		break;

	    case '$':
		dot = end_line (dot);
		break;

	    case '^':
		dot = begin_line (dot);

		while (*dot == ' ' || *dot == '\t')
			dot++;
		break;

	    case 'G':
		dot = end - 1;
		dot = begin_line (dot);
		break;

	    case 'g':
		n = get_one_char ();

		if (n == 'g')
		{
			dot = text;
		}
		break;

	    /* Scroll */
	    case 0x06:	/* Ctrl-F */
		for (n = 0; n < rows - 2; n++)
		{
			p = next_line (screenbegin);

			if (p >= end)
				break;

			screenbegin = p;
		}

		dot = screenbegin;
		break;

	    case 0x02:	/* Ctrl-B */
		for (n = 0; n < rows - 2; n++)
		{
			if (screenbegin <= text)
				break;

			screenbegin = prev_line (screenbegin);
		}

		dot = screenbegin;
		break;

	    /* Insercao */
	    case 'i':
		cmd_mode = 1;	/* insert mode */
		break;

	    case 'a':
		if (dot < end_line (dot))
			dot++;

		cmd_mode = 1;
		break;

	    case 'A':
		dot = end_line (dot);
		cmd_mode = 1;
		break;

	    case 'I':
		dot = begin_line (dot);

		while (*dot == ' ' || *dot == '\t')
			dot++;

		cmd_mode = 1;
		break;

	    case 'o':
		dot = end_line (dot);
		dot = char_insert (dot + 1, '\n');
		cmd_mode = 1;
		break;

	    case 'O':
		dot = begin_line (dot);
		dot = char_insert (dot, '\n');
		dot--;
		cmd_mode = 1;
		break;

	    /* Delecao */
	    case 'x':
		if (dot < end - 1 && *dot != '\n')
		{
			text_hole_delete (dot, dot);
		}
		break;

	    case 'X':
		if (dot > begin_line (dot))
		{
			dot--;
			text_hole_delete (dot, dot);
		}
		break;

	    case 'd':
		n = get_one_char ();

		if (n == 'd')
		{
			/* Delete line */
			p = begin_line (dot);
			q = end_line (dot);

			if (q < end - 1)
				q++;	/* include newline */

			text_hole_delete (p, q);
			dot = p;

			if (dot >= end)
				dot = end - 1;

			dot = begin_line (dot);
		}
		break;

	    case 'D':
		/* Delete to end of line */
		p = end_line (dot);

		if (p > dot)
		{
			text_hole_delete (dot, p - 1);
		}
		break;

	    /* Colon commands */
	    case ':':
		{
			char	buf[MAX_INPUT_LEN + 1];
			int	i;

			place_cursor (rows - 1, 0, FALSE);
			clear_to_eol ();
			write1 (":");

			i = 0;

			while (i < MAX_INPUT_LEN)
			{
				n = get_one_char ();

				if (n == '\n' || n == '\r')
					break;

				if (n == 0x1B)	/* ESC */
				{
					i = 0;
					break;
				}

				if (n == '\b' || n == 0x7F || n == erase_char)
				{
					if (i > 0)
					{
						i--;
						write1 ("\b \b");
					}

					continue;
				}

				buf[i++] = n;
				write (1, &buf[i-1], 1);
			}

			buf[i] = '\0';

			if (i > 0)
				colon (buf);
		}
		break;

	    /* Substituicao */
	    case 'r':
		n = get_one_char ();

		if (n >= ' ' && dot < end - 1)
		{
			*dot = n;
			file_modified++;
		}
		break;

	    case 'R':
		cmd_mode = 2;	/* replace mode */
		break;

	    /* Busca */
	    case '/':
	    case '?':
		{
			char	buf[MAX_INPUT_LEN + 1];
			int	i;

			dir = (c == '/') ? 1 : -1;

			place_cursor (rows - 1, 0, FALSE);
			clear_to_eol ();
			write (1, &c, 1);

			i = 0;

			while (i < MAX_INPUT_LEN)
			{
				n = get_one_char ();

				if (n == '\n' || n == '\r')
					break;

				if (n == 0x1B)
				{
					i = 0;
					break;
				}

				if (n == '\b' || n == 0x7F || n == erase_char)
				{
					if (i > 0)
					{
						i--;
						write1 ("\b \b");
					}

					continue;
				}

				buf[i++] = n;
				write (1, &buf[i-1], 1);
			}

			buf[i] = '\0';

			if (i > 0)
			{
				if (last_search_pattern)
					free (last_search_pattern);

				last_search_pattern = xstrdup (buf);
				p = search (dot + dir, buf, dir);

				if (p)
				{
					dot = p;
				}
				else
				{
					status_line_bold ("Padrao nao encontrado");
				}
			}
		}
		break;

	    case 'n':
		if (last_search_pattern && last_search_pattern[0])
		{
			p = search (dot + 1, last_search_pattern, 1);

			if (p)
			{
				dot = p;
			}
			else
			{
				status_line_bold ("Padrao nao encontrado");
			}
		}
		break;

	    case 'N':
		if (last_search_pattern && last_search_pattern[0])
		{
			p = search (dot - 1, last_search_pattern, -1);

			if (p)
			{
				dot = p;
			}
			else
			{
				status_line_bold ("Padrao nao encontrado");
			}
		}
		break;

	    /* Desfazer */
	    case 'u':
		status_line ("Undo nao implementado");
		break;

	    /* Repetir */
	    case '.':
		status_line ("Repeat nao implementado");
		break;

	    /* Redesenhar */
	    case 0x0C:	/* Ctrl-L */
		redraw (TRUE);
		break;

	    /* Ajuda */
	    case 0x1B:	/* ESC - nop em modo comando */
		break;

	    case 'Z':
		n = get_one_char ();

		if (n == 'Z')
		{
			/* ZZ - save and quit */
			if (file_modified && current_filename)
			{
				file_write (current_filename, text, end - 1);
			}

			editing = 0;
		}
		else if (n == 'Q')
		{
			/* ZQ - quit without saving */
			editing = 0;
		}
		break;

	    default:
		/* Comando desconhecido */
		Indicate_Error ();
		break;
	}

}	/* end do_cmd */

/*
 ****************************************************************
 *	Busca de padrao, sem regex              					*
 ****************************************************************
 */
static char *
search (char *start, char *pattern, int dir)
{
	char		*p, *found;
	int		len;

	len = strlen (pattern);

	if (len == 0)
		return (NULL);

	if (dir > 0)
	{
		/* Busca para frente */
		for (p = start; p < end - len; p++)
		{
			if (strncmp (p, pattern, len) == 0)
				return (p);
		}

		/* Wrap around */
		for (p = text; p < start; p++)
		{
			if (strncmp (p, pattern, len) == 0)
				return (p);
		}
	}
	else
	{
		/* Busca para tras */
		for (p = start; p >= text; p--)
		{
			if (strncmp (p, pattern, len) == 0)
				return (p);
		}

		/* Wrap around */
		for (p = end - len - 1; p > start; p--)
		{
			if (strncmp (p, pattern, len) == 0)
				return (p);
		}
	}

	return (NULL);

}	/* end search */

/*
 ****************************************************************
 *	Processamento de modo de insercao			                *
 ****************************************************************
 */
static void
do_insert (int c)
{
	switch (c)
	{
	    case 0x1B:	/* ESC - volta ao modo comando */
		cmd_mode = 0;

		if (dot > begin_line (dot))
			dot--;
		break;

	    case '\b':
	    case 0x7F:
		if (dot > begin_line (dot))
		{
			dot--;
			text_hole_delete (dot, dot);
		}
		break;

	    case '\r':
	    case '\n':
		dot = char_insert (dot, '\n');
		dot++;
		break;

	    default:
		if (c >= ' ' || c == '\t')
		{
			if (cmd_mode == 2)	/* replace mode */
			{
				if (*dot != '\n')
					*dot = c;
				else
					dot = char_insert (dot, c);
			}
			else
			{
				dot = char_insert (dot, c);
			}

			dot++;
			file_modified++;
		}
		break;
	}

}	/* end do_insert */

/*
 ****************************************************************
 *	Edicao do arquivo					                        *
 ****************************************************************
 */
static void
edit_file (char **argv)
{
	int		c;
	char		*fn;

	/* Configura sinais */
	my_pid = getpid ();
	signal (SIGINT, catch_sig);
	signal (SIGTERM, quit_sig);
	signal (SIGHUP, quit_sig);
	signal (SIGWINCH, winch_sig);

	/* Abre primeiro arquivo */
	fn = *argv;

	if (fn == NULL)
	{
		/* Sem arquivo - buffer vazio */
		text_size = 8192;
		text = xzalloc (text_size);
		end = text;
		text_hole_make (text, 1);
		*text = '\n';
		end = text + 1;
		dot = text;
		screenbegin = text;
		current_filename = NULL;
	}
	else
	{
		edit_file_by_name (fn);
	}

	/* Modo terminal bruto */
	rawmode ();

	/* Cria tela */
	createScreen ();
	redraw (TRUE);

	/* Loop principal */
	editing = 1;

	while (editing)
	{
		refresh (FALSE);

		c = get_one_char ();

		if (c < 0)
			break;

		if (cmd_mode)
		{
			do_insert (c);
		}
		else
		{
			do_cmd (c);
		}
	}

	/* Restaura terminal */
	cookmode ();

	place_cursor (rows - 1, 0, FALSE);
	clear_to_eol ();
	printf ("\n");

}	/* end edit_file */

/*
 ****************************************************************
 *	Programa principal					                        *
 ****************************************************************
 */
int
main (int argc, char **argv)
{
	int		opt;
	const char	*initial_cmd = NULL;
	const char	**cargv;

	/* Inicializacoes */
	rows = 24;
	columns = 80;
	tabstop = 8;
	terminfo_ok = 0;
	window_changed = 0;
	init_terminfo ();

	/* Analisa opcoes */
	cargv = (const char **)argv;
	while ((opt = getopt (argc, cargv, "Rc:")) != -1)
	{
		switch (opt)
		{
		    case 'R':
			readonly_mode = 1;
			break;

		    case 'c':
			initial_cmd = optarg;
			break;

		    default:
			show_help ();
			return (1);
		}
	}

	argc -= optind;
	argv += optind;

	/* Edita arquivo */
	edit_file (argv);

	/* Executa comando inicial se especificado */
	if (initial_cmd)
		colon (initial_cmd);

	return (0);

}	/* end main */
