/*
** insert /update posts
** Arguments:
** $1 := post_id
** $2 := post_date
** Return := seq
*/
WITH post_up AS (
	UPDATE posts p
	SET post_date = $2::TIMESTAMPTZ
	WHERE p.post_id = $1::integer
	RETURNING * -- AS 'aha!'
	)
, post_ins AS (
	INSERT INTO posts (post_id, post_date)
	SELECT $1,  $2
	WHERE NOT EXISTS (SELECT * 
		FROM post_up nx
		WHERE nx.post_id = $1::integer)
	RETURNING *
	)
SELECT seq -- , post_id, post_date
FROM post_up
UNION ALL
SELECT seq -- , post_id, post_date
FROM post_ins
	;
