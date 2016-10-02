
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <libpq-fe.h>
#include "pgstuff.h"

#ifndef DUMP_PQ
#define DUMP_PQ 0
#define DUMP_NEST 0
#define STATIC static
#endif

extern size_t ndup;
static char *script_dir = NULL;
static void strip_comments(char *buff);

/*********************************************************/
void set_script_dir(char *name)
{
free (script_dir);
script_dir = name ? strdup(name): NULL;
}
/*********************************************************/
char *read_file(char *name)
{
FILE *fp;
char path[PATH_MAX];
if (script_dir) sprintf(path, "%s/%s", script_dir, name);
else sprintf(path, "%s", name);
fp = fopen(path, "r");
if (!fp) {
	fprintf(stderr, "read_file: could not open %s\n", path);
	return NULL;
	}
name = read_query(fp);
fclose (fp);
return name;
}

/*********************************************************/
char *read_query(FILE *fp)
{
char *result = NULL;
size_t size=0,used=0;
int ch;

for (size=used=0; (ch=fgetc(fp)) != EOF; ) {
	if (used+1 >= size) {
		size_t newsize;
		newsize = size ? size*2: 20;
		result = realloc(result, newsize);
		size = newsize;
		}
	result[used++] = ch;
	if (ch == ';') break;
	}
if (!size) return NULL;
result[used++] = 0;
strip_comments(result);

return result;
}

/*********************************************************/
/* libpq does not allow comments in the SQL that you feed
** it.
** This function removes both -- and \/\* comments,
** and attempts to respect 'strings' and "quoted identifiers"
*/
static void strip_comments(char *buff)
{
int state;
char *cp;
unsigned src,dst,len;

for (state=0,src=dst=0; buff[src] ;src++ ){
	// fprintf(stderr, "{%u <- %u} %d %c\n"
		// , dst, src, state, buff[src] 
		// );
	switch(state) {
	default: state = 0;
	case 0: if (buff[src] == '-') {state = 1; continue; }
		if (buff[src] == '\'') {state = 3; }
		if (buff[src] == '\"') {state = 5; }
		if (buff[src] == '/') {state = 8; continue; }
		break;
	case 1: if (buff[src] == '-') { state = 2; continue; }
		buff[dst++] = '-'; state = 0;
		break;
	case 2: if (buff[src] == '\n') { state = 0; break; }
		/* absorb ... */
		continue;
	case 3: /* inside 'string' */
		if (buff[src] == '\n') { state = 0; break; }
		if (buff[src] == '\'') {state = 0; break; }
		if (buff[src] == '\\') {state = 4; break; }
		break;
	case 4: /* inside 'stri\xng' */
		state = 3; break;
	case 5: /* inside "string" */
		if (buff[src] == '\n') { state = 0; break; }
		if (buff[src] == '\"') {state = 0; break; }
		if (buff[src] == '\\') {state = 6; break; }
		break;
	case 6: /* inside "stri\xng" */
		state = 5; break;
	case 7: /* react to comic noise */
		fprintf(stderr, "There is no state#7. You lose...\n" );
		break;
	case 8: /* after "/" */
		if (buff[src] == '*') {state = 9; continue; }
		buff[dst++] = '/'; state = 0;
		break;
	case 9: /* after "/*" */
		if (buff[src] == '*') {state = 10; continue; }
		continue;
	case 10:
		if (buff[src] == '*') {state = 10; continue; }
		if (buff[src] == '/') {state = 0; continue; }
		state = 9;
		continue;
		}
assign:
	if (dst != src) buff[dst] = buff[src];
	dst++;
	}
switch (state) {
case 8: buff[dst++] = '/'; break;
case 1: buff[dst++] = '-'; break;
case 0: 
default: break;
	}
buff[dst] = 0;
return;
}

/*********************************************************/
PGresult *do_the_prepare(PGconn *pgc, char *tag, char *stmt, int nparam)
{
PGresult *prep;

prep = PQprepare (pgc, tag, stmt, nparam, NULL);
#if DUMP_PQ
fprintf(stderr, "PQprepare = %p\n", prep);
#endif

show_pqerror("PQPrepare", prep);

return prep;
}

/********************************************************/
void do_the_execute(PGconn *pgc, char *tag, char **params, int nparam)
{
PGresult *exec;
int npar, nrow,ncol, cidx, ridx;
char *zval;

#if DUMP_PQ
fprintf(stderr, "do_the_execute nparam=%d\n", nparam);
#endif

exec = PQexecPrepared(pgc, tag, nparam, params, NULL, NULL, 0);
#if DUMP_PQ
fprintf(stderr, "PQexecPrepared = %p\n", exec);
#endif

show_pqerror("PQexecPrepared(err:=)", exec);

#if DUMP_PQ
{
PGresult *desc;
desc = PQdescribePrepared(pgc, tag );
fprintf(stderr, "PQdescribePrepared = %p\n", desc);
show_pqerror("PQdescribePrepared", desc);
PQclear(desc);
}
#endif

#if DUMP_PQ
npar = PQnparams(exec);
fprintf(stderr, "PQnparams = %d\n", npar);
#endif

// zz = PQparamtype(rp);

ncol = PQnfields(exec);
#if DUMP_PQ
fprintf(stderr, "PQdescribenfields = %d\n", ncol);
#endif

#if DUMP_PQ
nrow = PQntuples(exec);
fprintf(stderr, "PQntuples = %d\n", nrow);
#endif


for (cidx =0; cidx < ncol; cidx++) {
	zval = PQfname( exec, cidx);
	fprintf(stdout, "%s%c", zval, (cidx == ncol-1) ? '\n' : '\t' );
	}
for (ridx =0; ridx < PQntuples(exec); ridx++) {
	for (cidx =0; cidx < ncol; cidx++) {
		zval = PQgetvalue(exec, ridx, cidx);
		if (!zval) goto end;
		fprintf (stdout, "%s%c", zval , (cidx == ncol-1) ? '\n' : '\t' );
		}
	// if (ridx >= 10) break;
	}
end:
PQclear(exec);

#if DUMP_PQ
fprintf(stderr, "Nrow=%d\n", ridx);
#endif

}

/********************************************************/
char *do_fetch_one(PGconn *pgc, char *tag, char **pair)
{
return do_fetch_n(pgc, tag, 2, pair);
}

/********************************************************/
char *do_fetch_n(PGconn *pgc, char *tag, unsigned nvar, char **vars)
{
PGresult *result;
int nrow,ncol, cidx, ridx;
char *zval;

result = PQexecPrepared(pgc, tag, nvar, vars, NULL, NULL, 0);
#if DUMP_PQ
fprintf(stderr, "PQexecPrepared = %p\n", result);
#endif

show_pqerror("PQexecPrepared", result);

ncol = PQnparams(result);
#if DUMP_PQ
fprintf(stderr, "PQdescribePrepared nparams = %d\n", ncol);
#endif

// zz = PQparamtype(rp);

ncol = PQnfields(result);
#if DUMP_PQ
fprintf(stderr, "PQdescribePrepared ncol = %d\n", ncol);
#endif

nrow = PQntuples(result);
#if DUMP_PQ
fprintf(stderr, "PQnrow = %d\n", nrow);
#endif


for (ridx =0; ridx < nrow; ridx++) {
	for (cidx =0; cidx < ncol; cidx++) {
		zval = PQgetvalue(result, ridx, cidx);
		if (!zval) goto end;
		break;
		}
	// if (ridx >= 10) break;
	}
#if DUMP_PQ
fprintf (stdout, "Zval=%s\n", zval );
#endif

if (zval) { zval = strdup(zval); ndup += 1; }
end:
PQclear(result);

#if DUMP_PQ
fprintf(stderr, "Nrow=%d\n", ridx);
#endif

return zval;
}

/********************************************************/
void show_pqerror(char *msg, PGresult *rp)
{
int status;
if (!msg) msg = "show_pqerror";

if (!rp) {
	fprintf(stderr, "%s: result IS NULL.\n", msg );
	return;
	}
status = PQresultStatus(rp);
switch (status){
case PGRES_COMMAND_OK: 
#if DUMP_PQ
	fprintf(stderr, "%s (%p) := Command_Ok\n", msg, rp);
#endif
	break;
case PGRES_TUPLES_OK: 
#if DUMP_PQ
	fprintf(stderr, "%s (%p) := Tuples_Ok\n", msg, rp);
#endif
	break;
case PGRES_EMPTY_QUERY: 
	fprintf(stderr, "%s (%p) := PGRES_EMPTY_QUERY\n",msg,  rp);
	break;
case PGRES_BAD_RESPONSE: 
	fprintf(stderr, "%s (%p) := PGRES_BAD_RESPONSE\n", msg, rp);
	goto extra;
	break;
case PGRES_FATAL_ERROR: 
	fprintf(stderr, "%s (%p) := PGRES_FATAL_ERROR\n",msg,  rp);
	goto extra;
	break;
default:
	fprintf(stderr, "%s (%p) Status:=%d\n", msg, rp, status);
	break;
extra:
	{
	struct { char *msg; int code;} omg[] =
		{ { "SEVERITY", PG_DIAG_SEVERITY}
		, { "SQLSTATE", PG_DIAG_SQLSTATE}
		, { "PRIMARY", PG_DIAG_MESSAGE_PRIMARY}
		, { "DETAIL", PG_DIAG_MESSAGE_DETAIL}
		, { "HINT", PG_DIAG_MESSAGE_HINT}
		};
	int ii;
	char *err;
	fprintf(stderr, "%s\n" , PQresultErrorMessage(rp) );
	for (ii=0; ii < 5; ii++) {
		err =  PQresultErrorField(rp, omg[ii].code);
		fprintf(stderr, "%s:%s\n"
		, omg[ii].msg, err );
		}
	}}
}

/********************************************************/
int analyse_PQstatus (char *msg, PGconn *conn) 
{
int status;
if (!msg) msg = "show_pqstatus";
if (!conn) {
	fprintf(stderr, "%s: conn IS NULL.\n", msg );
	return -1;
	}
status = PQstatus (conn);

switch (status) {
case CONNECTION_STARTED:
	fprintf(stderr, "%s: Waiting for connection to be made.\n", msg );
	break;

case CONNECTION_MADE:
	fprintf(stderr, "%s: Connection OK; waiting to send.\n", msg );
	break;

case CONNECTION_AWAITING_RESPONSE:
	fprintf(stderr, "%s: Waiting for a response from the server.\n", msg );
	break;

case CONNECTION_AUTH_OK:
	fprintf(stderr, "%s: Received authentication; waiting for backend start-up to finish.\n", msg );
	break;

case CONNECTION_SSL_STARTUP:
	fprintf(stderr, "%s: Negotiating SSL encryption.\n", msg );
	break;

case CONNECTION_SETENV:
	fprintf(stderr, "%s: Negotiating environment-driven parameter settings.\n", msg );
	break;
case CONNECTION_OK:
// case 0 :
	fprintf(stderr, "%s: The state returned was Ok.\n", msg );
	break;
default :
	fprintf(stderr, "%s: connecting ... status code :%d\n", msg , status );
	} 
return status;
} 

/********************************************************/
