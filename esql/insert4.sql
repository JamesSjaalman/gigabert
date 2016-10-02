/*
** insert/update posts_token and/or token
** Arguments:
** $1 := post_id (must exist)
** $2 := rnk
** $3 := token text
** Return := seq
*/
WITH tok_i AS (
	INSERT INTO brein.token (ttext)
	SELECT $3::varchar
	WHERE NOT EXISTS (SELECT * FROM brein.token x
		WHERE x.ttext = $3::varchar)
	RETURNING seq
	)
, tok_s AS (
	SELECT seq
	FROM brein.token t
	WHERE t.ttext = $3::varchar
	AND NOT EXISTS (SELECT * FROM tok_i x
		WHERE x.seq = t.seq)
	)
, tok_a AS ( /* the token number */
	SELECT seq FROM tok_i
	UNION ALL
	SELECT seq FROM tok_s
	)
, pt_u AS (
	UPDATE brein.posts_token pt
	SET tok_id = t.seq
	FROM tok_a t
	WHERE pt.post_id = $1::integer
	AND pt.rnk = $2::integer
	RETURNING *
	)
, pt_i	AS (
	INSERT INTO brein.posts_token(post_id, rnk, tok_id)
	SELECT $1::integer, $2::integer, t.seq
	FROM tok_a t
	WHERE NOT EXISTS (SELECT * FROM pt_u x
		WHERE x.post_id = $1::integer
		AND x.rnk = $2::integer
		)
	RETURNING *
	)
SELECT post_id, rnk, tok_id
FROM pt_u
UNION ALL
SELECT post_id, rnk, tok_id
FROM pt_i
	;
