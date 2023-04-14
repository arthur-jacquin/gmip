// see LICENSE file for copyright and license details

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define TB_IMPL
#include "termbox.h"

#define ERR_FILE_CONNECTION         1
#define ERR_UNICODE_OR_UTF8         2
#define ERR_MALLOC                  3
#define ERR_TERM_NOT_BIG_ENOUGH     4

#define VERSION                     "0.1.0"
#define HELP_MESSAGE                "Help available at https://jacquin.xyz/gmip"

#define DEFAULT_TITLE               argv[1]
#define DEFAULT_AUTHOR              ""
#define PADDING                     0
#define MIN_WIDTH                   8
#define MAX_WIDTH                   50
#define MIN_HEIGHT                  8
#define DEFAULT_BUF_SIZE            4
#define DEFAULT_CHARS_SIZE          (1 << 7)

#define TERM_256_COLORS_SUPPORT

// 256 colors mode: available colors are listed at https://jacquin.xyz/colors
#ifdef TERM_256_COLORS_SUPPORT
#define OUTPUT_MODE                 TB_OUTPUT_256
#define COLOR_DEFAULT               15
#define COLOR_METADATA              213
#define COLOR_LINK                  32
#define COLOR_HEADING               99
#define COLOR_LIST                  COLOR_LINK
#define COLOR_QUOTE                 172
#define COLOR_BG                    TB_DEFAULT

// 8 colors mode: available colors are TB_BLACK, TB_RED, TB_GREEN, TB_YELLOW,
// TB_BLUE, TB_MAGENTA, TB_CYAN, and TB_WHITE
#else
#define OUTPUT_MODE                 TB_OUTPUT_NORMAL
#define COLOR_DEFAULT               TB_WHITE
#define COLOR_METADATA              COLOR_DEFAULT
#define COLOR_LINK                  TB_YELLOW
#define COLOR_HEADING               TB_YELLOW
#define COLOR_LIST                  TB_YELLOW
#define COLOR_QUOTE                 TB_YELLOW
#define COLOR_BG                    TB_DEFAULT
#endif

#define MIN(A, B)                   (((A) < (B)) ? (A) : (B))
#define MAX(A, B)                   (((A) > (B)) ? (A) : (B))
#define MAP(A, B, C)                (MAX(B, MIN(A, C)))


// TYPES

struct line {                       // list of lines
    char *chars;                    // pointer to UTF-8, NULL-ended string
    struct line *next;              // pointer to next line
};

struct slide {
    struct line *start;
    int nb_parts;
};


// FUNCTIONS DECLARATIONS

int utf8_char_length(char c);
uint32_t unicode(const char *chars, int k, int len);
void *_malloc(int size);
void resize(int w, int h);
struct slide *parse_file(const char *filename);
void display_slide(const struct slide s, int index, int nb_parts);


// GLOBALS VARIABLES

int nb_slides;
char title[4*MAX_WIDTH + 1], author[4*MAX_WIDTH + 1];
int width, height;                  // terminal size
int offset, dw;                     // offset, displayed width
char utf8_start[4] = {0, 0xc0, 0xe0, 0xf0};
char masks[4] = {0x7f, 0x1f, 0x0f, 0x07};


// FUNCTION DEFINITIONS, MAIN

int
utf8_char_length(char c)
{
    // compute the length in bytes of UTF8 character starting by byte c

    if ((char) (c & (char) 0x80) == utf8_start[0]) {
        return 1;
    } else if ((char) (c & (char) 0xc0) == utf8_start[1]) {
        return 2;
    } else if ((char) (c & (char) 0xe0) == utf8_start[2]) {
        return 3;
    } else if ((char) (c & (char) 0xf0) == utf8_start[3]) {
        return 4;
    } else {
        exit(ERR_UNICODE_OR_UTF8);
        return ERR_UNICODE_OR_UTF8;
    }
}

uint32_t
unicode(const char *chars, int k, int len)
{
    // compute the Unicode codepoint associated with UTF-8 encoded char in chars

    uint32_t res;
    int j;

    res = masks[len - 1] & chars[k++];
    for (j = 1; j < len; j++) {
        res <<= 6;
        res |= chars[k++] & (char) 0x3f;
    }

    return res;
}

void *
_malloc(int size)
{
    // wrap a malloc call with error detection

    void *res;

    if ((res = malloc(size)) == NULL) {
        tb_shutdown();
        exit(ERR_MALLOC);
    }

    return res;
}

void
resize(int w, int h)
{
    // try to apply new size

    if ((width = w) < (2*PADDING + MIN_WIDTH) || (height = h) < MIN_HEIGHT) {
        tb_shutdown();
        exit(ERR_TERM_NOT_BIG_ENOUGH);
    }
    dw = MIN(width - 2*PADDING, MAX_WIDTH);
    offset = (width - dw) >> 1;
}

struct slide *
parse_file(const char *filename)
{
    // read the file, return its parsed content

    FILE *file = NULL;
    struct slide *buf, *new_buf;
    struct line *line, *last_line;
    char *chars, *new_chars;
    int reached_EOF, preformatted_mode, buf_size, chars_size;
    int c, ml, l, k;

    // init variables
    buf = _malloc(sizeof(struct slide) * (buf_size = DEFAULT_BUF_SIZE));
    chars = _malloc((chars_size = DEFAULT_CHARS_SIZE));
    nb_slides = 0;
    reached_EOF = 0;
    preformatted_mode = 0;
    buf[nb_slides].start = NULL;
    buf[nb_slides].nb_parts = 1;

    // open connection to file
    if ((file = fopen(filename, "r")) == NULL)
        exit(ERR_FILE_CONNECTION);

    // read content into memory
    while (!reached_EOF) {
        ml = 0;
        while (1) { // store characters of a line in chars
            if ((c = getc(file)) == EOF) {
                reached_EOF = 1;
                break;
            } else if (c == '\n') {
                break;
            } else {
                // potentially resize chars
                l = utf8_char_length(c);
                if (ml + l > chars_size) {
                    while (ml + l > chars_size)
                        chars_size <<= 1;
                    new_chars = _malloc(chars_size);
                    strncpy(new_chars, chars, ml);
                    free(chars);
                    chars = new_chars;
                }
                // store bytes with UTF-8 compliance check
                chars[ml] = (char) c;
                for (k = 1; k < l; k++) {
                    if (((c = getc(file)) == EOF) ||
                        (((char) c & (char) 0xc0) != (char) 0x80))
                        exit(ERR_UNICODE_OR_UTF8);
                    chars[ml + k] = (char) c;
                }
                ml += l;
            }
        }
        
        if (ml >= 3 && chars[0] == '`' && chars[1] == '`' && chars[2] == '`')
            preformatted_mode ^= 1;
        if (!preformatted_mode && (ml >= 3 && chars[0] == '-' && chars[1] == '-'
            && chars[2] == '-') || reached_EOF) {
            // closing the slide
            if (nb_slides + 1 >= buf_size) {
                while (nb_slides + 1 >= buf_size)
                    buf_size <<= 1;
                new_buf = _malloc(sizeof(struct slide) * buf_size);
                for (k = 0; k < nb_slides; k++) {
                    new_buf[k].start = buf[k].start;
                    new_buf[k].nb_parts = buf[k].nb_parts;
                }
                free(buf);
                buf = new_buf;
            }
            nb_slides++;
            buf[nb_slides].start = NULL;
            buf[nb_slides].nb_parts = 1;
        } else if (!preformatted_mode && !strncmp("%title:", chars, 7)) {
            strcpy(title, &(chars[7]));
        } else if (!preformatted_mode && !strncmp("%author:", chars, 8)) {
            strcpy(author, &(chars[8]));
        } else if (!preformatted_mode && !strncmp("%date:", chars, 6)) {
        } else {
            // append the new line
            line = _malloc(sizeof(struct line));
            line->chars = _malloc(ml + 1);
            strncpy(line->chars, chars, ml);
            line->chars[ml] = '\0';
            line->next = NULL;
            if (buf[nb_slides].start == NULL) {
                buf[nb_slides].start = last_line = line;
            } else {
                last_line->next = line;
                last_line = line;
            }
            if (!preformatted_mode && ml >= 1 && chars[0] == '^')
                buf[nb_slides].nb_parts++;
        }
    }

    // close connection to file
    if (fclose(file) == EOF)
        exit(ERR_FILE_CONNECTION);

    // copy to correct size buffer, free everything
    new_buf = _malloc(sizeof(struct slide) * nb_slides);
    for (k = 0; k < nb_slides; k++) {
        new_buf[k].start = buf[k].start;
        new_buf[k].nb_parts = buf[k].nb_parts;
    }
    free(buf);
    free(chars);

    return new_buf;
}

void
display_slide(const struct slide s, int index, int nb_parts)
{
    // display slide s on the screen

    uint32_t *ch = _malloc(sizeof(uint32_t) * dw * (height - 2));
    uint16_t *fg = _malloc(sizeof(uint16_t) * 2 * (height - 2));
    struct line *l;
    char ruler[8];
    int nb_lines, parts, nb_displayed_lines, h_offset;
    int preformatted_mode = 0;
    int accent, color, lvl;
    int i, j, jlw, k, kc, kclw, len, w_offset;

    // decompress slide
    l = s.start;
    k = nb_lines = 0;
    parts = 0;

    // create buffer of decompressed lines
    while (nb_lines < height - 2 && l != NULL) {
        if (l->chars[0] == '`' && l->chars[1] == '`' && l->chars[2] == '`') {
            preformatted_mode ^= 1;
        } else if (!preformatted_mode && l->chars[0] == '^') {
            if (++parts <= nb_parts)
                nb_displayed_lines = nb_lines;
        } else {
            lvl = 0;
            if (preformatted_mode) {
                accent = color = COLOR_DEFAULT;
            } else if (l->chars[0] == '=' && l->chars[1] == '>') {
                accent = color = COLOR_LINK;
            } else if (l->chars[0] == '#') {
                lvl = (l->chars[1] == '#') ? ((l->chars[2] == '#') ? 3 : 2) : 1;
                accent = color = COLOR_HEADING | ((lvl==1) ? TB_UNDERLINE : 0);
            } else {
                accent = color = COLOR_DEFAULT;
                if (l->chars[0] == '*' && l->chars[1] == ' ') {
                    accent = COLOR_LIST;
                } else if (l->chars[0] == '>') {
                    accent = COLOR_QUOTE;
                }
            }
            kc = 0;
            // hide '#' for headings
            while (lvl && l->chars[kc] == '#')
                kc++;
            while (nb_lines < height - 2) {
                // decompress to UTF-8, move to next character
                while (l->chars[kc] == ' ')
                    kc++;
                kclw = kc;
                j = jlw = 0;
                while (l->chars[kc]) {
                    if (j == dw) {
                        if (l->chars[kc] != ' ' && jlw > 0) {
                            // unprint word
                            j = jlw;
                            kc = kclw;
                        }
                        break;
                    }
                    if (l->chars[kc] == ' ') {
                        // identify next start of word
                        kclw = kc;
                        while (l->chars[kclw] == ' ')
                            kclw++;
                        if (!(l->chars[kclw])) {
                            kc = kclw;
                            break;
                        } else if (j + kclw - kc >= dw) {
                            break;
                        } else {
                            // print spaces
                            while (kc < kclw) {
                                ch[k + (j++)] = ' ';
                                kc++;
                            }
                        }
                    } else {
                        if (kc == kclw)
                            jlw = j;
                        len = utf8_char_length(l->chars[kc]);
                        ch[k + (j++)] = unicode(l->chars, kc, len);
                        kc += len;
                    }
                }

                // center
                if (((lvl == 1) || (lvl == 2)) && j < dw) {
                    j += (w_offset = (dw - j)/2);
                    for (i = j - 1; i >= w_offset; i--)
                        ch[k + i] = ch[k + i - w_offset];
                    while (i >= 0)
                        ch[k + (i--)] = ' ';
                }

                // fill with ' '
                while (j < dw)
                    ch[k + (j++)] = ' ';

                // colors, nb_lines increment
                fg[2*nb_lines] = accent;
                accent = fg[2*nb_lines + 1] = color;
                nb_lines++;
                k += dw;
                if (!(l->chars[kc]))
                    break;
            }
        }
        l = l->next;
    }
    if (++parts <= nb_parts)
        nb_displayed_lines = nb_lines;

    // actual content printing
    tb_clear();
    h_offset = 1 + ((height - 2 - nb_lines) >> 1);
    for (k = i = 0; i < nb_displayed_lines; i++) {
        for (j = 0; j < dw; j++) {
            tb_set_cell(offset + j, h_offset + i, ch[k++],
                fg[2*i + ((j == 0) ? 0 : 1)], COLOR_BG);
        }
    }

    // metadata printing
    tb_printf((width - strlen(title))/2, 0, COLOR_METADATA, COLOR_BG,
        "%s", title);
    tb_printf(0, height - 1, COLOR_METADATA, COLOR_BG, "%s", author);
    sprintf(ruler, "%d/%d", index, nb_slides);
    tb_printf(width - strlen(ruler), height - 1, COLOR_METADATA, COLOR_BG,
        "%s", ruler);

    free(ch);
    free(fg);
}

int
main(int argc, char *argv[])
{
    struct slide *buf;              // buffer of pointers to slides
    struct tb_event ev;             // struct to retrieve events
    int m = 0;                      // multiplier
    int index = 0;
    int displayed_parts = 1;
    int di;

    // parsing arguments
    if (argc < 2 || !(strcmp(argv[1], "--help") && strcmp(argv[1], "-h"))) {
        printf("%s\n", HELP_MESSAGE);
        return 0;
    } else if (!(strcmp(argv[1], "--version") && strcmp(argv[1], "-v"))) {
        printf("%s\n", VERSION);
        return 0;
    } else {
        strcpy(title, DEFAULT_TITLE);
        strcpy(author, DEFAULT_AUTHOR);
        buf = parse_file(argv[1]);
    }

    // init termbox
    tb_init();
    tb_set_output_mode(OUTPUT_MODE);
    tb_set_clear_attrs(COLOR_DEFAULT, COLOR_BG);
    resize(tb_width(), tb_height());

    // main loop
    while (1) {
        display_slide(buf[index], index + 1, displayed_parts);
        tb_present();
        tb_poll_event(&ev);

        if (ev.type == TB_EVENT_RESIZE)
            resize(ev.w, ev.h);
        if (ev.type != TB_EVENT_KEY)
            continue;
        if ((m && ev.ch == '0') || ('1' <= ev.ch && ev.ch <= '9')) {
            m = 10*m + ev.ch - '0';
            continue;
        }
        if (m == 0)
            m = 1;
        if (ev.ch) {
            switch (ev.ch) {
            case 'q':
                tb_shutdown();
                return 0;
            case ' ':
            case 'j':
            case 'l':
                di = m;
                break;
            case 'k':
            case 'h':
                di = -m;
                break;
            case 'g':
            case 'G':
                index = ((ev.ch == 'g') ? MIN(m, nb_slides) : nb_slides) - 1;
                displayed_parts = 1;
                m = 0;
                continue;
            }
        } else if (ev.key) {
            switch (ev.key) {
            case TB_KEY_ENTER:
            case TB_KEY_ARROW_DOWN:
            case TB_KEY_ARROW_RIGHT:
                di = m;
                break;
            case TB_KEY_BACKSPACE:
            case TB_KEY_BACKSPACE2:
            case TB_KEY_ARROW_UP:
            case TB_KEY_ARROW_LEFT:
                di = -m;
                break;
            case TB_KEY_ESC:
                m = 0;
                continue;
            }
        }
        m = 0;
        if ((di > 0 && displayed_parts < buf[index].nb_parts) ||
            (di < 0 && displayed_parts > 1)) {
            displayed_parts = MAP(displayed_parts + di, 1, buf[index].nb_parts);
        } else if ((di > 0 && index < nb_slides - 1) ||
            (di < 0 && index > 0)) {
            index = MAP(index + di, 0, nb_slides - 1);
            displayed_parts = (di > 0) ? 1 : buf[index].nb_parts;
        }
    }
}
