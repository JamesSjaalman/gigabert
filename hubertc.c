
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
extern char *optarg;
extern int optind, opterr, optopt;

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

STATIC size_t splitfields( char *arr[], char *str);
STATIC size_t splitit( char *arr[], char *str);
int addwords( PGconn *cp, char *tag, char **words, int dir);
int fakestuff (char **arr );

int add_post( PGconn *cp, char *tag, char **words);

void handler (int signum);

	/* Bitnask.
	** 1:= forward
	** 2:= backward
	** 4:= txt
	** 0 is silently folded into mode :=3
	*/
int mode = 0;
int keyfields = 0;
/*********************************************************/
#include "pgstuff.h"
	/* This is not so nice:
	** Including the tokeniser SOURCE here
	** because it is static (can be inlined)
	** , and it is shared with other programs
	*/
#include "tokenize.h"
#include "tokenize.c"
#define BIG_SIZE (16*1024)
#define MARKOV_ORDER 5
char globbuff[BIG_SIZE];
char *globwords[BIG_SIZE/2];
char *fields[16];
size_t nline=0;
size_t ndup=0;
size_t nfree=0;
/* Arguments:
** argv[1] linenumber to start with
**	(number of lines to skip)
** argv[2] := NULL
** Flags:
**	-m mode
*/
/*********************************************************/
int main(int argc, char **argv)
{
PGconn * conn; 
PGresult *prep;
int opt, status;
char *stmt1, *stmt2, *stmt3, *stmt4;
size_t sline=0,nword,idx;

while (1) {
	opt = getopt(argc, argv, "m:");
	switch (opt) {
	case -1: goto done;
	case 'm':
		opt = sscanf(optarg, "%d", &mode);
		if (opt < 1) goto err;
		break;
	case '?': fprintf(stderr, "No such option: %c\n", optopt);
		goto err;
	case ':': fprintf(stderr, "Missing option data:%s\n", optarg);
	err:
		exit(1);
		}
	}
done:

switch (mode) {
	case 0: keyfields =0; mode = 3; break;
	default: keyfields =2; break;
	}
fprintf(stderr, "Mode=%d Keyfields=%d optind=%d argv[1]=%s, argv[optind]=%s\n"
	, mode, keyfields, optind
	, argv[1] ,argv[optind] );

argv += optind;
argc -= optind;

signal(SIGHUP, handler);
signal(SIGTERM, handler);
signal(SIGQUIT, handler);

	/* insert2.sql is for the reversed Markov-chain;
	** because it is executed after insert.sql
	** it doesn't need to insert the tokens,
	** since that has already been done by insert.sql.
	*/
set_script_dir("esql" );
stmt1 = read_file("insert.sql");
stmt2 = read_file("insert2.sql");
stmt3 = read_file("insert3.sql");
stmt4 = read_file("insert4.sql");
// fprintf(stderr, "stmt4 := \"%s\"\n" , stmt4 );

if (argv[0] ) {
	sscanf(argv[0] ,"%zu", &sline);
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

prep = do_the_prepare(conn, "zzz3" , stmt3, 2);
show_pqerror("prep3", prep);
PQclear(prep);

prep = do_the_prepare(conn, "zzz4" , stmt4, 3);
show_pqerror("prep4", prep);
PQclear(prep);

globwords[0] = "<END>";
for (nline=0; fgets(globbuff, sizeof globbuff, stdin); nline++) {
	if (nline < sline) continue;
	if (nline %1000 ==0) fprintf(stdout, "%zu:\n", nline);
	if (nline %100 ==0) fputc('.', stdout);
		/* split fields */
	nword = splitfields(fields, globbuff);
	if (nword < keyfields) {
		fprintf(stderr, "Incorrect fieldcount %zu < %d\n"
			, nword, keyfields);
		goto quit;
		}
		/* tokenise */
	nword = 1+splitit(globwords+1, fields[keyfields] );
	globwords[nword++] = "<END>";
	globwords[nword] = NULL;
	if (mode & 4) {
		add_post( conn, "zzz3", fields );
		}

	if (mode & 1) addwords(conn, "zzz1", globwords, 1 );
		/* reverse the tokens and add the reverse chain */
	if (mode & 2) {
		for (idx=0; idx < --nword; idx++) {
	  	char *tmp;
	  	tmp = globwords[idx];
	  	globwords[idx] = globwords[nword];
	  	globwords[nword] = tmp; }
		addwords(conn, "zzz2", globwords, -1 );
		}
	}
quit:
fprintf(stdout, "Total: %zu\n",  nline);
// Clean exit
// PQclear(prep);
PQfinish (conn);
exit (0);
}

/*********************************************************/
size_t splitfields( char *arr[], char *str)
{
size_t idx, pos,len;

for (idx=pos=0; str[pos]; pos += len) {
	len = strcspn(str+pos, "\t\n" );
	arr[idx++] = str+pos;
	str[pos+len] = 0;
	len++;
	}
arr[idx] = NULL;
return idx;
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
		states[idx] = do_fetch_n(cp, tag, 2, pair);
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
int add_post( PGconn *cp, char *tag, char **words)
{
char *result = NULL;
char numbuff[14];
int rnk;
char *drie[3] = { result, numbuff, NULL };

result = do_fetch_n(cp, tag, 2, words);
if (!result) return 0;
drie[0] = result;

// fprintf(stderr, "Result := '%s'\n", result);

for(rnk=1;	; rnk++) {
	char *subres;
	sprintf(numbuff,"%d", rnk);
	if (!globwords[rnk]) break;
	drie[2] = globwords[rnk];
	subres = do_fetch_n ( cp, "zzz4", 3, drie);
	if(glob_error) {
		fprintf(stderr, "Words[] := {%s,%s} Drie := {%s,%s,'%s'}\n"
		, words[0], words[1]
		, drie[0], drie[1], drie[2] );
		// appears to be binary ..
		// fprintf(stderr, "Subres := '%s'\n", subres );
		}
	free (subres);
	}

free (result);
return 1;
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
