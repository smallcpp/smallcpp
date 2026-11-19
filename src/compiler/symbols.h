#ifndef SMALLCPP_SYMBOLS_H
#define SMALLCPP_SYMBOLS_H

/*
 * Names in one translation unit. The language server copies them
 * before the next call. Kinds match the server:
 *   0 use, 1 function, 2 method, 3 class, 4 variable,
 *   5 parameter, 6 field, 7 enumerator, 8 namespace, 9 tag.
 * defining is 1 when the name is introduced there.
 */

int scpp_index_file(const char *path);
int scpp_symbol_count(void);
const char *scpp_symbol_name(int index);
int scpp_symbol_line(int index);
int scpp_symbol_column(int index);
int scpp_symbol_length(int index);
int scpp_symbol_kind(int index);
int scpp_symbol_defining(int index);

#endif
