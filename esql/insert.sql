/*
** Insert or update a node and/or token into the markov tree
** Arguments:
** $1 := node number for parent number
** $2 := token (text)
*/
WITH tok_i AS (
	INSERT INTO brein.token (ttext,ncnt, rcnt)
	SELECT $2::varchar, 1 , 1
	WHERE NOT EXISTS (SELECT * FROM brein.token x WHERE x.ttext = $2::varchar)
	RETURNING seq, ncnt, rcnt
	)
, tok_s AS (
	SELECT seq
	FROM brein.token t
	WHERE t.ttext = $2::varchar
	AND NOT EXISTS (SELECT * FROM tok_i x WHERE x.seq = t.seq)
	)
, tok_a AS (
	SELECT seq FROM tok_i
	UNION ALL
	SELECT seq FROM tok_s
	)
, nod_u AS (
	UPDATE brein.node n
	SET myval = n.myval +1
	FROM tok_a t
	WHERE n.par = $1::BIGINT AND n.tok = t.seq
	RETURNING n.seq, n.par, n.tok, (n.myval-n.myval) as myval
	)
, nod_i AS (
	INSERT INTO brein.node (par, tok, myval)
	SELECT $1::BIGINT, a.seq, 1
	FROM tok_a a
	WHERE NOT EXISTS (SELECT * FROM nod_u x
		WHERE x.par = $1::BIGINT AND x.tok = a.seq
		)
	RETURNING seq,par,tok, myval
	)
, nod_a AS (
	SELECT seq, par, tok, myval FROM nod_u
	UNION ALL
	SELECT seq, par, tok, myval FROM nod_i
	)
, upd_2 AS (
	UPDATE brein.token dst
	SET ncnt = dst.ncnt+ n.myval
		, rcnt = dst.rcnt +1
	FROM nod_a n
	WHERE dst.ttext = $2::varchar
	AND n.par = $1::BIGINT AND n.tok = dst.seq
	RETURNING dst.seq, dst.rcnt, dst.ncnt
	)
SELECT n.seq AS nod
        , COALESCE(t.seq,i.seq) AS tok
        , COALESCE(t.ncnt,i.ncnt) AS ncnt
        , COALESCE(t.rcnt,i.rcnt) AS rcnt
FROM nod_a n
LEFT JOIN upd_2 t ON n.tok = t.seq
LEFT JOIN tok_i i ON n.tok = i.seq ;
