WITH tok_s AS (
	SELECT seq, rcnt, ncnt
	FROM brein.token t
	WHERE t.ttext = $2::varchar
	)
, nod_u AS (
	UPDATE brein.node n
	SET myval = n.myval +1
	FROM tok_s t
	WHERE n.par = $1::BIGINT AND n.tok = t.seq
	RETURNING n.seq, n.par, n.tok, (n.myval-n.myval) as myval
	)
, nod_i AS (
	INSERT INTO brein.node (par, tok, myval)
	SELECT $1::BIGINT, t.seq, 1
	FROM tok_s t
	WHERE NOT EXISTS (SELECT * FROM nod_u x
		WHERE x.par = $1::BIGINT AND x.tok = t.seq
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
        , COALESCE(u.seq,s.seq) AS tok
        , COALESCE(u.ncnt,s.ncnt) AS ncnt
        , COALESCE(u.rcnt,s.rcnt) AS rcnt
FROM nod_a n
LEFT JOIN upd_2 u ON n.tok = u.seq
LEFT JOIN tok_s s ON n.tok = s.seq ;
