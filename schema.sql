-- ================================================================================
-- Data Tables
-- ================================================================================

CREATE TABLE public.ratings (
	ID SERIAL PRIMARY KEY,
	name TEXT UNIQUE NOT NULL
);


CREATE TABLE public.images (
	id SERIAL PRIMARY KEY,
	sha256 VARCHAR(65) UNIQUE NOT NULL,
	size BIGINT NOT NULL DEFAULT 0,
	extension TEXT,
	favourite BOOLEAN NOT NULL DEFAULT false,
	created TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT now(),
	updated TIMESTAMP WITHOUT TIME ZONE
);


CREATE TABLE public.tags (
	id SERIAL PRIMARY KEY,
	name TEXT UNIQUE NOT NULL,
	blacklisted BOOLEAN NOT NULL DEFAULT false,
	category_id INTEGER NOT NULL DEFAULT 1,
	created TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT now(),
	updated TIMESTAMP WITHOUT TIME ZONE
);


CREATE TABLE public.categories (
	id SERIAL PRIMARY KEY,
	name TEXT UNIQUE NOT NULL
);

ALTER TABLE public.tags ADD FOREIGN KEY (category_id) REFERENCES public.categories (id) ON DELETE RESTRICT;


CREATE TABLE public.pools (
	id SERIAL PRIMARY KEY,
	name TEXT UNIQUE NOT NULL,
	description TEXT,
	created TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT now(),
	updated TIMESTAMP WITHOUT TIME ZONE
);


CREATE TABLE public.pool_types (
	id SERIAL PRIMARY KEY,
	name TEXT UNIQUE NOT NULL
);


CREATE TABLE public.pools_pooltypes (
	pool_id INTEGER UNIQUE NOT NULL,
	type_id INTEGER NOT NULL
);

ALTER TABLE public.pools_pooltypes ADD FOREIGN KEY (pool_id) REFERENCES public.pools (id) ON DELETE CASCADE;
ALTER TABLE public.pools_pooltypes ADD FOREIGN KEY (type_id) REFERENCES public.pool_types (id) ON DELETE RESTRICT;


-- ================================================================================
-- Bridge Tables
-- ================================================================================

CREATE TABLE public.images_source (
	image_id INTEGER UNIQUE NOT NULL,
	source TEXT NOT NULL
);

ALTER TABLE public.images_source ADD FOREIGN KEY (image_id) REFERENCES public.images (id) ON DELETE CASCADE;


CREATE TABLE public.images_ratings (
	image_id INTEGER UNIQUE NOT NULL,
	rating_id INTEGER NOT NULL,
);

ALTER TABLE public.images_ratings ADD FOREIGN KEY (image_id) REFERENCES public.images (id) ON DELETE CASCADE;
ALTER TABLE public.images_ratings ADD FOREIGN KEY (rating_id) REFERENCES public.ratings (id) ON DELETE RESTRICT;


CREATE TABLE public.commentary (
	ID SERIAL PRIMARY KEY,
	image_id INTEGER NOT NULL,
	lang VARCHAR(2) NOT NULL,
	data TEXT,
	created TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT now(),
	updated TIMESTAMP WITHOUT TIME ZONE,
	isXML BOOLEAN NOT NULL DEFAULT false
);

ALTER TABLE public.commentary ADD FOREIGN KEY (image_id) REFERENCES public.images (id) ON DELETE CASCADE;


CREATE TABLE public.images_tags (
	image_id INTEGER NOT NULL,
	tag_id INTEGER NOT NULL
);

ALTER TABLE public.images_tags ADD FOREIGN KEY (image_id) REFERENCES public.images (id) ON DELETE CASCADE;
ALTER TABLE public.images_tags ADD FOREIGN KEY (tag_id) REFERENCES	public.tags (id) ON DELETE RESTRICT;


CREATE TABLE public.image_relations (
	master_id INTEGER NOT NULL,
	slave_id INTEGER UNIQUE NOT NULL
);

ALTER TABLE public.image_relations ADD FOREIGN KEY (master_id) REFERENCES public.images (id) ON DELETE CASCADE;
ALTER TABLE public.image_relations ADD FOREIGN KEY (slave_id) REFERENCES public.images (id) ON DELETE CASCADE;


CREATE TABLE public.info (
	tag_id INTEGER UNIQUE NOT NULL,
	description TEXT NOT NULL,
	created TIMESTAMP WITHOUT TIME ZONE NOT NULL DEFAULT now(),
	updated TIMESTAMP WITHOUT TIME ZONE
);

ALTER TABLE public.info ADD FOREIGN KEY (tag_id) REFERENCES public.tags (id) ON DELETE CASCADE;	-- Check if this should cascade.


CREATE TABLE public.images_pools (
	image_id INTEGER NOT NULL,
	pool_id INTEGER NOT NULL
);

ALTER TABLE public.images_pools ADD FOREIGN KEY (image_id) REFERENCES public.images (id) ON DELETE RESTRICT;
ALTER TABLE public.images_pools ADD FOREIGN KEY (pool_id) REFERENCES public.pools (id) ON DELETE RESTRICT;	-- Check if this should cascade.


CREATE TABLE public.aliases (
	tag_id INTEGER NOT NULL,
	alias TEXT UNIQUE NOT NULL
);

ALTER TABLE public.aliases ADD FOREIGN KEY (tag_id) REFERENCES public.tags (id) ON DELETE RESTRICT;


CREATE TABLE public.implications (
	independent_tag INTEGER NOT NULL,
	dependent_tag INTEGER NOT NULL
);

ALTER TABLE public.implications ADD FOREIGN KEY (independent_tag) REFERENCES public.tags (id) ON DELETE RESTRICT;
ALTER TABLE public.implications ADD FOREIGN KEY (dependent_tag) REFERENCES public.tags (id) ON DELETE RESTRICT;


CREATE TABLE public.translations (
	image_id INTEGER NOT NULL,
	language_id INTEGER NOT NULL,
	data TEXT NOT NULL,
	x INTEGER NOT NULL,
	y INTEGER NOT NULL,
	width INTEGER NOT NULL,
	height INTEGER NOT NULL
);

ALTER TABLE public.translations ADD FOREIGN KEY (image_id) REFERENCES public.images (id) ON DELETE CASCADE;
ALTER TABLE public.translations ADD FOREIGN KEY (language_id) REFERENCES public.languages (id) ON DELETE RESTRICT;


-- ================================================================================
-- Functions
-- ================================================================================
CREATE OR REPLACE FUNCTION recursive_implications(implicator_id INTEGER,
						  visited_implicators INTEGER[] DEFAULT '{}')
	RETURNS SETOF INTEGER AS $$
DECLARE
	implication_value INTEGER;
BEGIN
	-- Check if the current implicator has been visited in the current recursion chain
	IF implicator_id = ANY(visited_implicators) THEN
		RETURN;		-- Avoid reprocessing visited implicator
	END IF;

	-- Add the current implicator to the visited array
	visited_implicators := visited_implicators || implicator_id;

	-- Fetch the implications for the given implicator_id
	FOR implication_value IN
		SELECT implication FROM public.implications WHERE implicator = implicator_id
	LOOP
		-- Return the current implication_value
		RETURN NEXT implication_value;

		-- Recurse to find further implications
		-- I don't think we want implicator_id in here.
		RETURN QUERY SELECT * FROM recursive_implications(implication_value, visited_implicators) WHERE recursive_implications != implicator_id;
--		RETURN QUERY SELECT * FROM recursive_implications(implication_value, visited_implicators);
	END LOOP;

	RETURN;
END;
$$ LANGUAGE plpgsql;


-- ================================================================================
-- AND Query
-- ================================================================================
CREATE OR REPLACE FUNCTION and_query(tag_ids INTEGER[])
RETURNS TABLE (img_id INTEGER) AS
$$
BEGIN
	RETURN QUERY
	SELECT DISTINCT image_id
	FROM public.images_tags it1
	WHERE ARRAY(
		SELECT tag_id
		FROM public.images_tags it2
		WHERE it2.image_id = it1.image_id
		ORDER BY tag_id
	) @> tag_ids;
END;
$$
LANGUAGE plpgsql;


-- ================================================================================
-- OR Query
-- ================================================================================
CREATE OR REPLACE FUNCTION or_query(tag_ids INTEGER[])
RETURNS TABLE(id INTEGER)
LANGUAGE plpgsql AS
$$
BEGIN
	RETURN QUERY
	SELECT DISTINCT i.id
	FROM public.images AS i
	JOIN public.images_tags AS it ON i.id = it.image_id
	JOIN public.tags AS t ON t.id = it.tag_id
	WHERE t.id = ANY (tag_ids);
END
$$;


-- ================================================================================
-- Tag to ID
-- ================================================================================
CREATE OR REPLACE FUNCTION tag2id(_name TEXT)
	RETURNS SETOF INTEGER AS
$$
BEGIN
	RETURN QUERY SELECT id FROM public.tags WHERE tags.name = _name;
END;
$$ LANGUAGE plpgsql;


-- ================================================================================
-- ID to Tag
-- ================================================================================
CREATE OR REPLACE FUNCTION id2tag(_id INTEGER)
	RETURNS SETOF TEXT AS
$$
BEGIN
	RETURN QUERY SELECT name FROM public.tags WHERE id = _id;
END
$$ LANGUAGE plpgsql;



-- ================================================================================
-- Get Image Tags by Category
-- ================================================================================
CREATE OR REPLACE FUNCTION imgTagsCat(_sha256 TEXT, cat INTEGER)
	RETURNS TABLE(name TEXT, category INTEGER) AS
$$
BEGIN
	RETURN QUERY SELECT t.name, t.category_id
		FROM public.tags AS t
		JOIN public.images_tags AS it
		ON it.tag_id = t.id
		WHERE it.image_id = (SELECT public.images.id FROM public.images WHERE public.images.sha256 = _sha256)
		AND t.category_id = cat
		ORDER BY t.name ASC;
END $$ LANGUAGE plpgsql;
