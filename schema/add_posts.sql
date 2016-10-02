
SET search_path = brein, pg_catalog;

DROP TABLE brein.posts CASCADE;
CREATE TABLE brein.posts
	( seq bigserial NOT NULL PRIMARY KEY
	, post_id INTEGER UNIQUE
	, post_date TIMESTAMP WITH TIME ZONE
	);


ALTER TABLE brein.posts OWNER TO tim;
GRANT SELECT ON TABLE brein.posts TO public;


DROP TABLE brein.posts_token CASCADE;
CREATE TABLE brein.posts_token
	( post_id INTEGER REFERENCES brein.posts(seq)
	, rnk INTEGER NOT NULL DEFAULT 0
	, tok_id INTEGER REFERENCES brein.token(seq)
	, PRIMARY KEY (post_id, rnk)
	);

CREATE UNIQUE INDEX ON brein.posts_token (tok_id,post_id,rnk);
CREATE UNIQUE INDEX ON brein.posts_token (tok_id,rnk,post_id);


ALTER TABLE brein.posts_token OWNER TO tim;
GRANT SELECT ON TABLE brein.posts_token TO public;

