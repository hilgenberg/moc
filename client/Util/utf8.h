#pragma once
# include <ncurses.h>
# include <iconv.h>

void utf8_init ();
void utf8_cleanup ();

size_t strwidth (const str &s);
str xstrtail (const str &s, int len);

void strdel(str &s, int i, int n = 1);
int  strins(str &s, int i, wchar_t c); // returns width of c

int xwaddstr (WINDOW *win, const str &s);
int xmvwaddstr (WINDOW *win, const int y, const int x, const str &s);

// format: r: cut off at the right:     [foo...]
//         l: cut off at the left:      [...foo]
//         c: cut off in the center:    [f...oo]
//         R: right-align or same as l: [   foo]
//         C: centered or r
void xwprintfield(WINDOW *win, const str &s, int field_width, char format = 'r');

str iconv_str (const iconv_t desc, const str &input);
str files_iconv_str (const str &s);

// replace tabs and such by spaces
void sanitize(str &s);
str sanitized(const str &s);