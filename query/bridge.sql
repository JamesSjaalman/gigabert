
-- psql -U postgres -h 10.224.60.102 krant

\connect krant
\i incl.sql
\d
\d posts
\d token
\d posts_token
-- select COUNT(*) FROM posts ;
-- select COUNT(*) FROM posts_token ;

SELECT pt1.*, t1.*
FROM posts_token pt1
JOIN token t1 ON t1.seq = pt1.tok_id
WHERE EXISTS (
	SELECT *
	FROM posts_token pt2
	JOIN token t2 ON t2.seq = pt2.tok_id
	WHERE pt2.post_id = pt1.post_id
	AND t2.ttext = '.'
	)
ORDER BY pt1.post_id, pt1.rnk
LIMIT 1000
	;

