-- comment
SELECT v.val -- tha value
	, 'omg' AS "OMG"
	, 'wtf-omg' AS "wtf-omg"
	, 'embedded\'quote' AS "embedded\"quote"
	/* inner comment -- ... */
FROM (select 1 AS val) v
/* JOIN pg_catalog.pg_class 
	ON v."val" = c.cid AND c.name = 'OmG';
	*/
	;
