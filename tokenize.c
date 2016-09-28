/*
 *		Function:	Tokenize
 *
 *		Purpose:		Find the length of the token started @ string
 * This tokeniser attempts to respect abbreviations such as R.W.A or R.w.a.
 * Also, numeric literals, such as 200.50 are left alone (almost the same for 200,50 or 200.000,00)
 * Maybe 10e-0.6 should also be recognised.
 * Multiple stretches of .,!? are kept intact, multiple stretches of
 * other non alphanumerc tokens (@#$%^&*) as well.
 * Brackets and braces are always chopped up to 1-character tokens.
 * Quoted strings are not respected, but broken up into separate tokens.
 * The quotes are treated as separate tokens too.
 */
#define T_INIT 0
#define T_ANY 1
#define T_WORD 2
#define T_WORDDOT 3
#define T_WORDDOTLET 4
#define T_AFKODOT 5
#define T_AFKO 6
#define T_NUM 7
#define T_NUMDOT 8
#define T_NUMDOTLET 9
#define T_MEUK 10
#define T_PUNCT 11
#define T_ATHASH 12
#define T_URL 13
#define T_URL_DANGLING1 (T_URL +0x100)
#define T_URL_DANGLING2 (T_URL +0x101)
STATIC size_t tokenize(char *str, int *sp)
{
    size_t pos ;

    for(pos=0; str[pos]; ) {
    switch(*sp) {
    case T_INIT: /* initial */
#if 0
	/* if (str[pos] == '\'' ) { *sp |= 16; return pos; }*/
	/* if (str[posi] == '"' ) { *sp |= 32; return pos; }*/
#endif
	if (myisalpha(str[pos])) {*sp = T_WORD; pos++; continue; }
	if (myisalnum(str[pos])) {*sp = T_NUM; pos++; continue; }
	if (strspn(str+pos, "#@")) { *sp = T_ATHASH; pos++; continue; }
	/* if (strspn(str+pos, "-+")) { *sp = T_NUM; pos++; continue; }*/
	*sp = T_ANY; continue;
	break;
    case T_ANY: /* either whitespace or meuk: eat it */
	pos += strspn(str+pos, " \t\n\r\f\b" );
	if (pos) {*sp = T_INIT; return pos; }
        *sp = T_MEUK; continue;
        break;
    case T_WORD: /* inside word */
	while ( myisalnum(str[pos]) ) pos++;
	if (str[pos] == '\0' ) { *sp = T_INIT;return pos; }
	if (str[pos] == '.' ) { *sp = T_WORDDOT; pos++; continue; }
	*sp = T_INIT; return pos;
        break;
    case T_WORDDOT: /* word. */
	if (myisalpha(str[pos]) ) { *sp = T_WORDDOTLET; pos++; continue; }
	*sp = T_INIT; return pos-1;
        break;
    case T_WORDDOTLET: /* word.A */
	if (str[pos] == '.') { *sp = T_AFKODOT; pos++; continue; }
	if (myisalpha(str[pos]) ) { *sp = T_INIT; return pos-2; }
	if (myisalnum(str[pos]) ) { *sp = T_NUM; continue; }
	*sp = T_INIT; return pos-2;
        break;
    case T_AFKODOT: /* A.B. */
        if (myisalpha(str[pos]) ) { *sp = T_AFKO; pos++; continue; }
        *sp = T_INIT; return pos >= 3 ? pos : pos -2;
        break;
    case T_AFKO: /* word.A.B */
	if (str[pos] == '.') { *sp = T_AFKODOT; pos++; continue; }
	if (myisalpha(str[pos]) ) { *sp = T_INIT; return pos - 2; }
	*sp = T_INIT; return pos-1;
        break;
    case T_NUM: /* number */
	if ( myisalnum(str[pos]) ) { pos++; continue; }
	if (strspn(str+pos, ".,")) { *sp = T_NUMDOT; pos++; continue; }
	*sp = T_INIT; return pos;
        break;
    case T_NUMDOT: /* number[.,] */
	if (myisalpha(str[pos])) { *sp = T_NUMDOTLET; pos++; continue; }
	if (myisalnum(str[pos])) { *sp = T_NUM; pos++; continue; }
	if (strspn(str+pos, "-+")) { *sp = T_NUM; pos++; continue; }
	*sp = T_INIT; return pos-1;
        break;
    case T_NUMDOTLET: /* number[.,][a-z] */
	if (myisalpha(str[pos])) { *sp = T_INIT; return pos-2; }
	if (myisalnum(str[pos])) { *sp = T_NUM; pos++; continue; }
	*sp = T_INIT; return pos-2;
        break;
    case T_MEUK: /* inside meuk */
	if (myisalnum(str[pos])) {*sp = T_INIT; return pos; }
	if (myiswhite(str[pos])) {*sp = T_INIT; return pos; }
	if (strspn(str+pos, ".,?!" )) { if (!pos) *sp = T_PUNCT; else { *sp = T_INIT; return pos; }}
	if (strspn(str+pos, "@'\"" )) { *sp = T_INIT; return pos ? pos : 1; }
	if (strspn(str+pos, ":;" )) { *sp = T_INIT; return pos ? pos : 1; }
	if (strspn(str+pos, "('\"){}[]" )) { *sp = T_INIT; return pos ? pos : 1; }
	pos++; continue; /* collect all garbage */
        break;
    case T_PUNCT: /* punctuation */
	if (strspn(str+pos, "@#" )) { *sp = T_MEUK; pos++; continue; }
	pos += strspn(str+pos, ".,?!" ) ;
        *sp = T_INIT; return pos ? pos : 1;
        break;
    case T_ATHASH: /* @ or # should be followed by an "identifier"  */
	while ( myisalpha(str[pos]) ) pos++;
	if (pos  < 2)  { *sp = T_MEUK; continue; }
        while ( myisalnum_extra(str[pos]) ) {pos++; }
	*sp = T_INIT;
	return pos ;
    case T_URL_DANGLING1: /* http: or https: looking for // */
	if ( str[pos] == '/' ) { pos++; *sp = T_URL_DANGLING2; }
	else { *sp = T_INIT; return pos-1; }
    case T_URL_DANGLING2: /* http: or https: looking for // */
	if ( str[pos] == '/' ) { pos++; *sp = T_URL;}
	else { *sp = T_INIT; return pos-2;}
    case T_URL: /* http:// or https:// should be followed by an "identifier"  */
	if ( !myisalpha(str[pos]) ) { *sp = T_INIT; return pos-3; }
        while ( myisalnum_html(str[pos]) ) {pos++; }
	*sp = T_INIT;
	return pos ;
    // case T_URL_DANGLING3: /* http://word. or https://word. looking for word */
        }
    }
    /* This is ugly. Depending on the state,
    ** we need to know how many lookahead chars we consumed */
    switch (*sp) {
    case T_INIT: break;
    case T_ANY: break;
    case T_WORD: break;
    case T_WORDDOT: pos -= 1; break;
    case T_WORDDOTLET: pos -= 2; break;
    case T_AFKODOT: if (pos < 3) pos -= 2; break;
    case T_AFKO: break;
    case T_NUM: break;
    case T_NUMDOT: pos-= 1; break;
    case T_NUMDOTLET: pos -= 2; break;
    case T_MEUK: break;
    case T_PUNCT: break;
    case T_ATHASH: break;
    default: break;
    }
    *sp = T_INIT; return pos;
}

STATIC int myisupper(int ch)
{
if (ch >= 'A' && ch <= 'Z') return 1;
else return 0;
}

STATIC int myislower(int ch)
{
if (ch >= 'a' && ch <= 'z') return 1;
else return 0;
}

STATIC int myisalnum(int ch)
{
int ret = myisalpha(ch);
if (ret) return ret;
if (ch >= '0' && ch <= '9') return 1;
return 0;
}

STATIC int myisalnum_extra(int ch)
{
int ret = myisalnum(ch);
if (ret) return ret;
if (ch == '_' || ch == '$') return 1;
return 0;
}

STATIC int myisalnum_html(int ch)
{
int ret = myisalnum_extra(ch);
if (ret) return ret;
if (ch == '#' || ch == '?' || ch == '%' ) return 1;
return 0;
}

STATIC int myisalpha(int ch)
{
if (myislower(ch) || myisupper(ch)) return 1;
	/* don't parse, just assume valid utf8*/
if (ch & 0x80) return 1;
return 0;
}

STATIC int myiswhite(int ch)
{
if (ch == ' ') return 1;
if (ch == '\t') return 1;
if (ch == '\n') return 1;
if (ch == '\r') return 1;
if (ch == '\f') return 1;
return 0;
}

STATIC int buffer_is_usable(char *buf, unsigned len)
{
unsigned idx;

if (len < 1) return 1;

for(idx = 0; idx < len; idx++) switch( buf[idx] ) {
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	case '\0': continue;
	default: return 1;
	}
return 0;
}

