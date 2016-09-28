
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <libpq-fe.h>

#define DUMP_PQ 0
#define DUMP_NEST 0

#define STATIC static

// Login info (in include file ...)
const char * keywords[] = {"host",   "dbname", "user", "password", NULL};
const char * values[] = /* this is a very silly way of storing credentials */
#include "secret/login.dat"
	;

int analyse_PQstatus (char *msg, PGconn *conn);
PGresult *do_the_prepare(PGconn *cp, char *tag, char *stmt, int nparam);
void do_the_execute(PGconn *cp, char *tag, char **params, int nparam);
void show_pqerror(char *msg, PGresult *rp);
STATIC size_t splitit( char *arr[], char *str);
char *do_fetch_one(PGconn *cp, char *tag, char **pair);
int addwords( PGconn *cp, char *tag, char **words, int dir);
int fakestuff (char **arr );

void handler (int signum);
/*********************************************************/
#include "pgstuff.h"

#include "tokenize.h"
#include "tokenize.c"
#define BIG_SIZE (16*1024)
#define MARKOV_ORDER 5
char globbuff[BIG_SIZE];
char *globwords[BIG_SIZE/2];
size_t nline=0;
size_t ndup=0;
size_t nfree=0;
/*********************************************************/
int main(int argc, char **argv)
{
PGconn * conn; 
PGresult *prep;
int status;
char *stmt1, *stmt2;
size_t sline=0,nword,idx;


signal(SIGHUP, handler);
signal(SIGTERM, handler);
signal(SIGQUIT, handler);

	/* insert2.sql is for the reversed Markov-chain;
	** because it is executed after insert.sql
	** it doesn't need to insert the tokens,
	** that has already been done by insert.sql.
	*/
set_script_dir("esql" );
stmt1 = read_file("insert.sql");
stmt2 = read_file("insert2.sql");

if (argv[1] ) {
	sscanf(argv[1] ,"%zu", &sline);
	fprintf(stderr, "Starting at line=%zu\n", sline );
	}

// Connect to the database
conn = PQconnectdbParams (keywords, values, 0);
fprintf(stderr, "\nMypid %d Conn= %p\n", (int) getpid(), (void*) conn );
while(1)	{
	status = analyse_PQstatus ("PQconnectdbParams", conn);
	if (status != 1) break;
	sleep(1);
	}

prep = PQexec(conn, "SET search_path = brein;" );
show_pqerror("searchpath", prep);
PQclear(prep);

// Prepare query
prep = do_the_prepare(conn, "zzz1" , stmt1, 2);
show_pqerror("prep1", prep);
PQclear(prep);

prep = do_the_prepare(conn, "zzz2" , stmt2, 2);
show_pqerror("prep2", prep);
PQclear(prep);

globwords[0] = "<END>";
for (nline=0; fgets(globbuff, sizeof globbuff, stdin); nline++) {
	if (nline < sline) continue;
	if (nline %1000 ==0) fprintf(stdout, "\n%zu:", nline);
	if (nline %100 ==0) fputc('.', stdout);
	nword = 1+splitit(globwords+1, globbuff);
	globwords[nword++] = "<END>";
	globwords[nword] = NULL;
	addwords(conn, "zzz1", globwords, 1 );
		/* reverse the tokens */
	for (idx=0; idx < --nword; idx++) {
	  char *tmp;
	  tmp = globwords[idx];
	  globwords[idx] = globwords[nword];
	  globwords[nword] = tmp; }
	addwords(conn, "zzz2", globwords, -1 );
	}

fprintf(stdout, "Total: %zu\n",  nline);
// Clean exit
// PQclear(prep);
PQfinish (conn);
exit (0);
}

/*********************************************************/
size_t splitit( char *arr[], char *str)
{
size_t idx, pos,len;
int sta =0;

for (idx=pos=0; str[pos]; pos += len) {
	len = tokenize(str+pos, &sta);
	if (len > 255) len = 255;
	if (!buffer_is_usable(str+pos, len)) continue;
	arr[idx++] = str+pos;
	str[pos+len] = 0;
	len++;
	}
arr[idx] = NULL;
return idx;
}

/********************************************************/
int addwords(PGconn *cp, char *tag, char **words, int dir)
{
char *wrd;
unsigned idx,lev,cnt;
char *states[MARKOV_ORDER+1];
char *pair[2];

#if NO_EXEC
fprintf(stderr, "%s:=", dir >=0 ? "Forward" : "Backward" );
return fakestuff (words );
#endif

if (dir >=0) { states[idx=0] = "1"; }
else	{ states[idx=0] = "-1"; }

lev =1;
cnt = 0;
for (; (wrd = *words++ ); ) {
	pair[1] = wrd;
	for(idx=lev; idx>0;idx-- ) {
		pair[0] = states[idx-1];
		states[idx] = do_fetch_one(cp, tag, pair);
#if DUMP_NEST
		fprintf(stderr, "[%u] [d=%zu f=%zu] {%s,%s} -> '%s'\n"
			, idx, ndup, nfree
			, pair[0], pair[1] , states[idx] );
#endif
		if (idx > 1) { free(pair[0]); nfree += 1; }
		}
	cnt += idx;
	if(++lev >= MARKOV_ORDER) {
		lev = MARKOV_ORDER-1;
#if DUMP_NEST
		fprintf(stderr, "Free %u: %s\n", lev, states[lev] );
#endif
		free( states[lev] );
		nfree += 1;
		}
	for(idx=lev; idx > 1;idx--) {
		states[lev] = states[lev-1];
		}
	}

for( ; lev-- > 1; ) {
#if DUMP_NEST
	fprintf(stderr, "Free %u: %s\n", lev, states[lev] );
#endif
	free( states[lev] );
	nfree += 1;
	}
return cnt;
}
/********************************************************/
void handler (int signum)
{
char buff[512];
char **cpp;
size_t len;
len = sprintf(buff, "Signal %d while on input line %zu: {dup=%zu,free=%zu}"
	, signum, nline
	, ndup, nfree);

write (STDERR_FILENO, buff, len);


len = 0;
for (cpp = globwords; *cpp; cpp++) {
	size_t chunk;
	chunk = strlen (*cpp);
	memcpy(buff, *cpp, chunk);
	memcpy( buff+chunk, " ", 1); chunk += 1;
	write (STDERR_FILENO, buff, chunk);
	}

write (STDERR_FILENO, "\n", 1);

if (signum == SIGHUP) return;

_exit(1);

}
/********************************************************/
int fakestuff (char **arr )
{
int ret=0;
char **cpp;

for (cpp = globwords; *cpp; cpp++) {
	fprintf(stderr, " %s", *cpp);
	ret++;
	}

fprintf(stderr, "\n");
return ret;
}
/********************************************************/
