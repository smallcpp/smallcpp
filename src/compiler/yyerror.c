/* Quiet syntax reports for a process that only indexes names.
 * scpp_cc defines its own yyerror, so this file stays in the
 * archive and is linked only when nothing else provides one. */

void yyerror(const char *message)
{
    (void)message;
}
