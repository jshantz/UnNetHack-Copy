#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "cursmisc.h"
#include "func_tab.h"
#include "dlb.h"
#include "patchlevel.h"

#include <ctype.h>

/* Misc. curses interface functions */

/* Private declarations */

#define NETHACK_CURSES      1
#define SLASHEM_CURSES      2
#define UNNETHACK_CURSES    3

/* Banners used for an optional ASCII splash screen */

#define NETHACK_SPLASH_A \
"         _   _        _    _    _               _    "

#define NETHACK_SPLASH_B \
"        | \\ | |      | |  | |  | |             | |   "

#define NETHACK_SPLASH_C \
"        |  \\| |  ___ | |_ | |__| |  __ _   ___ | | __"

#define NETHACK_SPLASH_D \
"        | . ` | / _ \\| __||  __  | / _` | / __|| |/ /"

#define NETHACK_SPLASH_E \
"        | |\\  ||  __/| |_ | |  | || (_| || (__ |   < "

#define NETHACK_SPLASH_F \
"        |_| \\_| \\___| \\__||_|  |_| \\__,_| \\___||_|\\_\\"

#define SLASHEM_SPLASH_A \
"                _____  _              _     _  ______  __  __ "

#define SLASHEM_SPLASH_B \
"               / ____|| |            | |   ( )|  ____||  \\/  |"

#define SLASHEM_SPLASH_C \
"              | (___  | |  __ _  ___ | |__  \\|| |__   | \\  / |"

#define SLASHEM_SPLASH_D \
"               \\___ \\ | | / _` |/ __|| '_ \\   |  __|  | |\\/| |"

#define SLASHEM_SPLASH_E \
"               ____) || || (_| |\\__ \\| | | |  | |____ | |  | |"

#define SLASHEM_SPLASH_F \
"              |_____/ |_| \\__,_||___/|_| |_|  |______||_|  |_|"

#define UNNETHACK_SPLASH_A \
"     _    _         _   _        _    _    _               _"

#define UNNETHACK_SPLASH_B \
"    | |  | |       | \\ | |      | |  | |  | |             | |"

#define UNNETHACK_SPLASH_C \
"    | |  | | _ __  |  \\| |  ___ | |_ | |__| |  __ _   ___ | | __"

#define UNNETHACK_SPLASH_D \
"    | |  | || '_ \\ | . ` | / _ \\| __||  __  | / _` | / __|| |/ /"

#define UNNETHACK_SPLASH_E \
"    | |__| || | | || |\\  ||  __/| |_ | |  | || (_| || (__ |   <"

#define UNNETHACK_SPLASH_F \
"     \\____/ |_| |_||_| \\_| \\___| \\__||_|  |_| \\__,_| \\___||_|\\_\\"

static int curs_x = -1;
static int curs_y = -1;
static winid curs_win = 0;

/* Macros for Control and Alt keys */

#ifndef M
# ifndef NHSTDC
#  define M(c)		(0x80 | (c))
# else
#  define M(c)		((c) - 128)
# endif /* NHSTDC */
#endif
#ifndef C
#define C(c)		(0x1f & (c))
#endif


/* Read a character of input from the user */

int curses_read_char()
{
    int ch, tmpch;

    ch = getch();
    tmpch = ch;
    if (!ch)
    {
        ch = '\033'; /* map NUL to ESC since nethack doesn't expect NUL */
    }

#if defined(ALT_0) && defined(ALT_9)    /* PDCurses, maybe others */    
    if ((ch >= ALT_0) && (ch <= ALT_9))
    {
        tmpch = (ch - ALT_0) + '0';
        ch = M(tmpch);
    }
#endif

#if defined(ALT_A) && defined(ALT_Z)    /* PDCurses, maybe others */    
    if ((ch >= ALT_A) && (ch <= ALT_Z))
    {
        tmpch = (ch - ALT_A) + 'a';
        ch = M(tmpch);
    }
#endif

#ifdef KEY_RESIZE
    /* Handle resize events via get_nh_event, not this code */
    if (ch == KEY_RESIZE)
    {
        ch = '\033'; /* NetHack doesn't know what to do with KEY_RESIZE */
    }
#endif

    /* Handle arrow keys */
    
    switch (ch)
    {
        case KEY_LEFT:
        {
            if (iflags.num_pad)
            {
                ch = '4';
            }
            else
            {
                ch = 'h';
            }
            break;
        }
        case KEY_RIGHT:
        {
            if (iflags.num_pad)
            {
                ch = '6';
            }
            else
            {
                ch = 'l';
            }
            break;
        }
        case KEY_UP:
        {
            if (iflags.num_pad)
            {
                ch = '8';
            }
            else
            {
                ch = 'k';
            }
            break;
        }
        case KEY_DOWN:
        {
            if (iflags.num_pad)
            {
                ch = '2';
            }
            else
            {
                ch = 'j';
            }
            break;
        }
    }

    return ch;
}

/* Turn on or off the specified color and / or attribute */

void curses_toggle_color_attr(WINDOW *win, int color, int attr, int onoff)
{
#ifdef TEXTCOLOR
    int curses_color;

    /* Map color disabled */
    if ((!iflags.wc_color) && (win == mapwin))
    {
        return;
    }
    
    /* GUI color disabled */
    if ((!iflags.wc2_guicolor) && (win != mapwin))
    {
        return;
    }
    
    if (color == 0) /* make black fg visible */
    {
#ifdef USE_DARKGRAY
        color = 8;
#else
        if (iflags.use_inverse)
        {
            wattron(win, A_REVERSE);
        }
        else
        {
            color = CLR_BLUE;
        }
#endif
    }
    curses_color = color + 1;
    if (curses_color > 8)
        curses_color -= 8;
    if (onoff == ON)    /* Turn on color/attributes */
    {
        if (color != NONE)
        {
            if (color > 7)
            {
                wattron(win, A_BOLD);
            }
            wattron(win, COLOR_PAIR(curses_color));
        }
        
        if (attr != NONE)
        {
            wattron(win, attr);
        }
    }
    else                /* Turn off color/attributes */
    {
        if (color != NONE)
        {
            if (color > 7)
            {
                wattroff(win, A_BOLD);
            }
#ifndef USE_DARKGRAY
            if ((color == 0) && iflags.use_inverse)
            {
                wattroff(win, A_REVERSE);
            }
#endif  /* DARKGRAY */
            wattroff(win, COLOR_PAIR(curses_color));
        }
        
        if (attr != NONE)
        {
            wattroff(win, attr);
        }
    }
#endif  /* TEXTCOLOR */
}


/* clean up and quit - taken from tty port */

void curses_bail(const char *mesg)
{
    clearlocks();
    curses_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
}


/* Return a winid for a new window of the given type */

winid curses_get_wid(int type)
{
	winid ret;
    static winid menu_wid = 20; /* Always even */
    static winid text_wid = 21; /* Always odd */

	switch (type)
	{
		case NHW_MESSAGE:
		{
			return MESSAGE_WIN;
			break;
		}
		case NHW_MAP:
		{
			return MAP_WIN;
			break;
		}
		case NHW_STATUS:
		{
			return STATUS_WIN;
			break;
		}
		case NHW_MENU:
		{
			ret = menu_wid;
			break;
		}
		case NHW_TEXT:
		{
			ret = text_wid;
			break;
		}
		default:
		{
			panic("curses_get_wid: unsupported window type");
			ret = -1;   /* Not reached */
		}
	}

	while (curses_window_exists(ret))
	{
	    ret += 2;
	    if ((ret + 2) > 10000)    /* Avoid "wid2k" problem */
	    {
	        ret -= 9900;
	    }
	}
	
	if (type == NHW_MENU)
	{
	    menu_wid += 2;
	}
	else
	{
	    text_wid += 2;
	}

	return ret;
}


/*
 * Allocate a copy of the given string.  If null, return a string of
 * zero length.
 *
 * This is taken from copy_of() in tty/wintty.c.
 */

char *curses_copy_of(const char *s)
{
    if (!s) s = "";
    return strcpy((char *) alloc((unsigned) (strlen(s) + 1)), s);
}


/* Determine the number of lines needed for a string for a dialog window
of the given width */

int curses_num_lines(const char *str, int width)
{
    int last_space, count;
    int curline = 1;
    char substr[BUFSZ];
    char tmpstr[BUFSZ];
    
    strcpy(substr, str);
    
    while (strlen(substr) > width)
    {
        last_space = 0;
        
        for (count = 0; count <= width; count++)
        {
            if (substr[count] == ' ')
            last_space = count;
        }
        if (last_space == 0)    /* No spaces found */
        {
            last_space = count - 1;
        }
        for (count = (last_space + 1); count < strlen(substr); count++)
        {
            tmpstr[count - (last_space + 1)] = substr[count];
        }
        tmpstr[count - (last_space + 1)] = '\0';
        strcpy(substr, tmpstr);
        curline++;
    }
    
    return curline;
}


/* Break string into smaller lines to fit into a dialog window of the
given width */

char *curses_break_str(const char *str, int width, int line_num)
{
    int last_space, count;
    char *retstr;
    int curline = 0;
    int strsize = strlen(str);
    char substr[strsize];
    char curstr[strsize];
    char tmpstr[strsize];
    
    strcpy(substr, str);
    
    while (curline < line_num)
    {
        if (strlen(substr) == 0 )
        {
            break;
        }
        curline++;
        last_space = 0;       
        for (count = 0; count <= width; count++)
        {
            if (substr[count] == ' ')
            {
                last_space = count;
            }
            else if (substr[count] == '\0')           
            {
                last_space = count;
                break;
            }
        }
        if (last_space == 0)    /* No spaces found */
        {
            last_space = count - 1;
        }
        for (count = 0; count < last_space; count++)
        {
            curstr[count] = substr[count];
        }
        curstr[count] = '\0';
        if (substr[count] == '\0')
        {
            break;
        }
        for (count = (last_space + 1); count < strlen(substr); count++)
        {
            tmpstr[count - (last_space + 1)] = substr[count];
        }
        tmpstr[count - (last_space + 1)] = '\0';
        strcpy(substr, tmpstr);
    }
    
    if (curline < line_num)
    {
        return NULL;
    }
    
    retstr = curses_copy_of(curstr);
    
    return retstr;
}


/* Return the remaining portion of a string after hacking-off line_num lines */

char *curses_str_remainder(const char *str, int width, int line_num)
{
    int last_space, count;
    char *retstr;
    int curline = 0;
    int strsize = strlen(str);
    char substr[strsize];
    char curstr[strsize];
    char tmpstr[strsize];
    
    strcpy(substr, str);
    
    while (curline < line_num)
    {
        if (strlen(substr) == 0 )
        {
            break;
        }
        curline++;
        last_space = 0;       
        for (count = 0; count <= width; count++)
        {
            if (substr[count] == ' ')
            {
                last_space = count;
            }
            else if (substr[count] == '\0')           
            {
                last_space = count;
                break;
            }
        }
        if (last_space == 0)    /* No spaces found */
        {
            last_space = count - 1;
        }
        for (count = 0; count < last_space; count++)
        {
            curstr[count] = substr[count];
        }
        curstr[count] = '\0';
        if (substr[count] == '\0')
        {
            break;
        }
        for (count = (last_space + 1); count < strlen(substr); count++)
        {
            tmpstr[count - (last_space + 1)] = substr[count];
        }
        tmpstr[count - (last_space + 1)] = '\0';
        strcpy(substr, tmpstr);
    }
    
    if (curline < line_num)
    {
        return NULL;
    }
    
    retstr = curses_copy_of(substr);
    
    return retstr;
}


/* Determine if the given NetHack winid is a menu window */

boolean curses_is_menu(winid wid)
{
    if ((wid > 19) && !(wid % 2))   /* Even number */
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/* Determine if the given NetHack winid is a text window */

boolean curses_is_text(winid wid)
{
    if ((wid > 19) && (wid % 2))   /* Odd number */
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/* Replace certain characters with portable drawing characters if
cursesgraphics option is enabled */

int curses_convert_glyph(int ch, int glyph)
{
    int symbol;
    
    /* Save some processing time by returning if the glyph represents
    an object that we don't have custom characters for */
    if (!glyph_is_cmap(glyph))
    {
        return ch;
    }
    
    symbol = glyph_to_cmap(glyph);
    
    /* If user selected a custom character for this object, don't
    override this. */
    if (((glyph_is_cmap(glyph)) && (ch != showsyms[symbol])))
    {
        return ch;
    }

    switch (symbol)
    {
        case S_vwall:
            return ACS_VLINE;
        case S_hwall:
            return ACS_HLINE;
        case S_tlcorn:
            return ACS_ULCORNER;
        case S_trcorn:
            return ACS_URCORNER;
        case S_blcorn:
            return ACS_LLCORNER;
        case S_brcorn:
            return ACS_LRCORNER;
        case S_crwall:
            return ACS_PLUS;
        case S_tuwall:
            return ACS_BTEE;
        case S_tdwall:
            return ACS_TTEE;
        case S_tlwall:
            return ACS_RTEE;
        case S_trwall:
            return ACS_LTEE;
        case S_tree:
            return ACS_PLMINUS;
        case S_corr:
            return ACS_CKBOARD;
        case S_litcorr:
            return ACS_CKBOARD;
    }

	return ch;
}


/* Move text cursor to specified coordinates in the given NetHack window */

void curses_move_cursor(winid wid, int x, int y)
{
#ifndef PDCURSES
    WINDOW *win = curses_get_nhwin(curs_win);
#endif

    if (wid != MAP_WIN)
    {
        return;
    }

#ifdef PDCURSES
    /* PDCurses seems to not handle wmove correctly, so we use move and
    physical screen coordinates instead */
    curses_get_window_xy(wid, &curs_x, &curs_y);
    curs_x += x;
    curs_y += y;
#else
    curs_x = x;
    curs_y = y;
#endif    
    curs_win = wid;
#ifdef PDCURSES
    move(curs_y, curs_x);
#else
    wmove(win, curs_y, curs_x);
#endif
}


/* Perform actions that should be done every turn before nhgetch() */

void curses_prehousekeeping()
{
#ifndef PDCURSES
    WINDOW *win = curses_get_nhwin(curs_win);
#endif

    if ((curs_x > -1) && (curs_y > -1))
    {
        curs_set(1);
#ifdef PDCURSES
        /* PDCurses seems to not handle wmove correctly, so we use move
        and physical screen coordinates instead */
        move(curs_y, curs_x);
#else
        wmove(win, curs_y, curs_x);
#endif
        curses_refresh_nhwin(curs_win);
    }
}


/* Perform actions that should be done every turn after nhgetch() */

void curses_posthousekeeping()
{
    curs_set(0);
    curses_decrement_highlight();
    curses_clear_unhighlight_message_window();
}


void curses_view_file(const char *filename, boolean must_exist)
{
    winid wid;
    anything *identifier;
    char buf[BUFSZ];
    menu_item *selected = NULL;
    dlb *fp = dlb_fopen(filename, "r");
    
    if ((fp == NULL) && (must_exist))
    {
        pline("Cannot open %s for reading!", filename);
    }

    if (fp == NULL)
    {
        return;
    }
    
    wid = curses_get_wid(NHW_MENU);
    curses_create_nhmenu(wid);
    identifier = malloc(sizeof(anything));
    identifier->a_void = NULL;
    
    while (dlb_fgets(buf, BUFSZ, fp) != NULL)
    {
        curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE, buf,
         FALSE);
    }
    
    dlb_fclose(fp);
    curses_end_menu(wid, "");
    curses_select_menu(wid, PICK_NONE, &selected);
}
/* Display an ASCII splash screen for up to 2 seconds if the
splash_screen option is set.  The splash screen may be
dismissed sooner with a keystroke. */

void curses_display_splash_window()
{
    winid wid;
    anything *identifier;
    menu_item *selected = NULL;
    int which_variant = NETHACK_CURSES;  /* Default to NetHack */
    
    wid = curses_get_wid(NHW_MENU);
    curses_create_nhmenu(wid);
    identifier = malloc(sizeof(anything));
    identifier->a_void = NULL;
    
    timeout(2000);  /* Pause for 2 seconds or a key is hit */
#ifdef DEF_GAME_NAME
    if (strcmp(DEF_GAME_NAME, "SlashEM") == 0)
    {
        which_variant = SLASHEM_CURSES;
    }
#endif

#ifdef GAME_SHORT_NAME
    if (strcmp(GAME_SHORT_NAME, "UNH") == 0)
    {
        which_variant = UNNETHACK_CURSES;
    }
#endif

    switch (which_variant)
    {
        case NETHACK_CURSES:
        {
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             NETHACK_SPLASH_A, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             NETHACK_SPLASH_B, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             NETHACK_SPLASH_C, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             NETHACK_SPLASH_D, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             NETHACK_SPLASH_E, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             NETHACK_SPLASH_F, FALSE);
            break;
        }
        case SLASHEM_CURSES:
        {
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             SLASHEM_SPLASH_A, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             SLASHEM_SPLASH_B, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             SLASHEM_SPLASH_C, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             SLASHEM_SPLASH_D, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             SLASHEM_SPLASH_E, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             SLASHEM_SPLASH_F, FALSE);
            break;
        }
        case UNNETHACK_CURSES:
        {
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             UNNETHACK_SPLASH_A, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             UNNETHACK_SPLASH_B, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             UNNETHACK_SPLASH_C, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             UNNETHACK_SPLASH_D, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             UNNETHACK_SPLASH_E, FALSE);
            curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
             UNNETHACK_SPLASH_F, FALSE);
            break;
        }
        default:
        {
            impossible("which_variant number %d out of range",
             which_variant);
        }
    }

#ifdef COPYRIGHT_BANNER_A
    curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE, "",
     FALSE);
    curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
     COPYRIGHT_BANNER_A, FALSE);
#endif

#ifdef COPYRIGHT_BANNER_B
    curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
     COPYRIGHT_BANNER_B, FALSE);
#endif

#ifdef COPYRIGHT_BANNER_C
    curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
     COPYRIGHT_BANNER_C, FALSE);
#endif

#ifdef COPYRIGHT_BANNER_D   /* Just in case */
    curses_add_menu(wid, NO_GLYPH, identifier, 0, 0, A_NONE,
     COPYRIGHT_BANNER_D, FALSE);
#endif

    curses_end_menu(wid, "");
    curses_select_menu(wid, PICK_NONE, &selected);
    timeout(-1);
}


void curses_rtrim(char *str)
{
    char *s;

    for(s = str; *s != '\0'; ++s);
    for(--s;isspace(*s) && s > str; --s);
    if(s == str) *s = '\0';
    else *(++s) = '\0';
}


/* Read numbers until non-digit is encountered, and return number
in int form. */

int curses_get_count(int first_digit)
{
    long current_count = first_digit;
    int current_char;
    
    current_char = curses_read_char();
    
    while (isdigit(current_char))
    {
        current_count = (current_count * 10) + (current_char - '0');
        if (current_count > 32768)  /* Could go higher, but no need */
        {
            current_count = 32768;
        }
        
        pline("Count: %ld", current_count);
        current_char = curses_read_char();
    }
    
    return current_count;
}


/* Convert the given NetHack text attributes into the format curses
understands, and return that format mask. */

int curses_convert_attr(int attr)
{
    int curses_attr;
    
    switch (attr)
    {
        case ATR_NONE:
        {
            curses_attr = A_NORMAL;
            break;
        }
        case ATR_ULINE:
        {
            curses_attr = A_UNDERLINE;
            break;
        }
        case ATR_BOLD:
        {
            curses_attr = A_BOLD;
            break;
        }
        case ATR_BLINK:
        {
            curses_attr = A_BLINK;
            break;
        }
        case ATR_INVERSE:
        {
            curses_attr = A_REVERSE;
            break;
        }
        default:
        {
            curses_attr = A_NORMAL;
        }
    }
        
    return curses_attr;
}


/* Map letter attributes to bitmask, and store result in
iflags.wc2_petattr, setting a default of underlined if no valid
option is chosen.  Return mask on success, or 0 if not found */

int curses_read_attrs(char *attrs)
{
    if (strchr(attrs, 'b') || strchr(attrs, 'B'))
    {
	    iflags.wc2_petattr = iflags.wc2_petattr|A_BOLD;
    }
    if (strchr(attrs, 'i') || strchr(attrs, 'I'))
    {
	    iflags.wc2_petattr = iflags.wc2_petattr|A_REVERSE;
    }
    if (strchr(attrs, 'u') || strchr(attrs, 'U'))
    {
	    iflags.wc2_petattr = iflags.wc2_petattr|A_UNDERLINE;
    }
    if (strchr(attrs, 'k') || strchr(attrs, 'K'))
    {
	    iflags.wc2_petattr = iflags.wc2_petattr|A_BLINK;
    }
#ifdef A_ITALIC
    if (strchr(attrs, 't') || strchr(attrs, 'T'))
    {
	    iflags.wc2_petattr = iflags.wc2_petattr|A_ITALIC;
    }
#endif
#ifdef A_RIGHTLINE
    if (strchr(attrs, 'r') || strchr(attrs, 'R'))
    {
	    iflags.wc2_petattr = iflags.wc2_petattr|A_RIGHTLINE;
    }
#endif
#ifdef A_LEFTLINE
    if (strchr(attrs, 'l') || strchr(attrs, 'L'))
    {
	    iflags.wc2_petattr = iflags.wc2_petattr|A_LEFTLINE;
    }
#endif
    return iflags.wc2_petattr;
}


/* Use nethack wall symbols for drawing unless cursesgraphics is
defined, in which case we use the standard curses ones.  Not sure if
this is desirable behavior since one may prefer regular lines for
borders but traditional symbols to draw rooms.  Commenting it out for
now. */

/*
 * void curses_border(WINDOW *win)
 * {
 *     if (iflags.cursesgraphics)
 *     {
 *         box(win, 0, 0);
 *     }
 *     else
 *     {
 *         wborder(win, showsyms[S_vwall], showsyms[S_vwall],
 *          showsyms[S_hwall], showsyms[S_hwall], showsyms[S_tlcorn],
 *          showsyms[S_trcorn], showsyms[S_blcorn], showsyms[S_brcorn]);
 *     }
 * }
 */

