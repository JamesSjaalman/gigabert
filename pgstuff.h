#ifndef PGSTUFF_H
#define PGSTUFF_H 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libpq-fe.h>

int analyse_PQstatus (char *msg, PGconn *conn);
PGresult *do_the_prepare(PGconn *cp, char *tag, char *stmt, int nparam);
void do_the_execute(PGconn *cp, char *tag, char **params, int nparam);
void show_pqerror(char *msg, PGresult *rp);
char *do_fetch_one(PGconn *cp, char *tag, char **pair);

void set_script_dir(char *name);
char *read_file(char *name);
char *read_query(FILE *fp);

#endif /* PGSTUFF_H */
