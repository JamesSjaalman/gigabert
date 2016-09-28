
\i users.sch
DROP SCHEMA brein CASCADE;
CREATE SCHEMA brein;
GRANT usage on schema brein to public;

SET search_path = brein;


CREATE TABLE token
	( seq SERIAL NOT NULL PRIMARY KEY
	, ttext varchar(255) NOT NULL UNIQUE
	, rcnt integer NOT NULL DEFAULT 0
	, ncnt integer NOT NULL DEFAULT 0
	-- , color CHAR(1) NOT NULL DEFAULT '-'
	);

CREATE TABLE node
	( seq BIGSERIAL NOT NULL PRIMARY KEY
	, par bigint NOT NULL REFERENCES node(seq)
	, tok integer NOT NULL REFERENCES token(seq)
	, myval bigint NOT NULL DEFAULT 0
	, sumval bigint NOT NULL DEFAULT 0
	-- , color CHAR(1) NOT NULL DEFAULT '-'
	, UNIQUE (par,tok)
	);

ALTER TABLE brein.token OWNER TO tim;
ALTER TABLE brein.node OWNER TO tim;

INSERT INTO token(seq, ttext) VALUES (0, '');
INSERT INTO token(ttext) VALUES ( '<END>');
INSERT INTO token(ttext) VALUES ( '<BEG>');

INSERT INTO node (seq,par,tok) VALUES (0,0,0);	-- ROOT
INSERT INTO node (par,tok) VALUES (0,1);	-- FWD
INSERT INTO node (seq,par,tok) VALUES (-1,0,2); -- REV

GRANT select on brein.token to public;
GRANT select on brein.node to public;

