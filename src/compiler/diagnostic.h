#ifndef SMALLCPP_DIAGNOSTIC_H
#define SMALLCPP_DIAGNOSTIC_H

/* Remember the source so a later diagnostic can print the line. */
void smallcpp_diag_load(const char *path);

/*
 * Print path:line:column, the message, and that source line.
 * There is no caret and no pointer under the line.
 */
void smallcpp_diag_report(int line, int column, const char *message);

#endif
