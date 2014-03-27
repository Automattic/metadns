/*
 * MySQL Datbase Schema for the MetaDNS PowerDNS module
 * Contains standard DNS domain and records tables, as well as the
 * linking tables for fast Dynamic DNS changes of user defined groups.
 */

/*
 * Drop tables in the correct order, so as not to violate foreign keys
*/

DROP TABLE IF EXISTS record;
DROP TABLE IF EXISTS pool;
DROP TABLE IF EXISTS poolhouse;
DROP TABLE IF EXISTS `domain`;
DROP TABLE IF EXISTS domainsoa;

/*
 * Since our SOA is just masses of duplication, they are stored in a
 * seperate linked table, which will increase domain name searching
 * as we shouln't be getting nearly as many queries for SOA as record info.
 */

CREATE TABLE domainsoa (
	domainsoa_id	INT AUTO_INCREMENT,
	name_server		VARCHAR(128) DEFAULT NULL,
	host_master		VARCHAR(128) NOT NULL,
	serial_number 	INT DEFAULT 2005071858,
	refresh			INT DEFAULT 14400,
	retry			INT DEFAULT 7200,
	expire			INT DEFAULT 604800,
	default_ttl		INT DEFAULT 300,
	ttl				INT DEFAULT 86400,
	active			TINYINT(1) NOT NULL DEFAULT 1,
	PRIMARY KEY (domainsoa_id)
) Engine = InnoDB;


/*
 * Bare bones domain names table, since this is hit for every query,
 * irrespective of type.
 */

CREATE TABLE domain (
	domain_id		INT AUTO_INCREMENT,
	domainsoa_id	INT NOT NULL,
	domain_name		VARCHAR(255) NOT NULL,
	active			TINYINT(1) NOT NULL DEFAULT 1,
	PRIMARY KEY (domain_id),
	CONSTRAINT fk_domain_domainsoa_id
    FOREIGN KEY ( domainsoa_id )
    REFERENCES domainsoa ( domainsoa_id )
    ON DELETE NO ACTION
    ON UPDATE CASCADE)
) Engine = InnoDB;

CREATE UNIQUE INDEX uniq_idx_domainname ON domain (domain_name);
CREATE INDEX idx_domainname_active ON domain (domain_name, active);

/*
 * The pool IP's are grouped via this table. This enables groups such as:
 * WP.com load balancers, Google Email Servers, etc. to be created.
 */

CREATE TABLE poolhouse (
	poolhouse_id		INT AUTO_INCREMENT,
	poolhouse_name		VARCHAR(100) NOT NULL,
	hash_results		TINYINT(1) NOT NULL DEFAULT 0 	COMMENT "If this column is set it overrides the 'random_results' setting.",
	random_results		TINYINT(1) NOT NULL DEFAULT 1	COMMENT "When 'hash_results' is not set, then the num_results will be chosen from a random ordered list.",
	num_results			TINYINT(2) NOT NULL DEFAULT 3,
	ttl					INT4 NOT NULL DEFAULT 86400,
	active				TINYINT(1) NOT NULL DEFAULT 1,
	PRIMARY KEY (poolhouse_id)
) Engine = InnoDB;

CREATE UNIQUE INDEX uniq_idx_poolhousename ON poolhouse (poolhouse_name);

/*
 * The pools of content records.
 */

CREATE TABLE pool (
	pool_id			INT AUTO_INCREMENT,
	poolhouse_id	INT NOT NULL,
	content			VARCHAR(128) NOT NULL,
	priority		INT2 DEFAULT NULL,
	hashring_weight int(2) NOT NULL DEFAULT '100',
	monitor_type_id int(2) NOT NULL DEFAULT '0',
	in_pool			TINYINT(1) NOT NULL DEFAULT 1,
	active			TINYINT(1) NOT NULL DEFAULT 1,
	PRIMARY KEY (pool_id),
	CONSTRAINT fk_pool_poolhouse_id
    FOREIGN KEY ( poolhouse_id )
    REFERENCES poolhouse ( poolhouse_id )
    ON DELETE RESTRICT
    ON UPDATE CASCADE
) Engine = InnoDB;

CREATE INDEX idx_pool_poolhouseid ON pool (poolhouse_id);

/*
 * DNS Records, with grouped record types linked to pools of content
 * via the poolhouse_id.
 */

CREATE TABLE record (
	record_id		INT AUTO_INCREMENT,
	domain_id		INT NOT NULL,
	record_name		VARCHAR(128) NOT NULL,
	record_type		VARCHAR(15) NOT NULL,
	poolhouse_id	INT DEFAULT NULL,
	active			TINYINT(1) NOT NULL DEFAULT 1,
	PRIMARY KEY (record_id),
	CONSTRAINT fk_record_domain_id
    FOREIGN KEY ( domain_id )
    REFERENCES domain ( domain_id )
    ON DELETE RESTRICT
    ON UPDATE CASCADE,
  CONSTRAINT fk_record_poolhouse_id
    FOREIGN KEY ( poolhouse_id )
    REFERENCES poolhouse ( poolhouse_id )
    ON DELETE RESTRICT
    ON UPDATE CASCADE
) Engine = InnoDB;

CREATE INDEX idx_record_recordname ON record (record_name);
CREATE INDEX idx_recordname_recordtype ON record (record_name, record_type);
CREATE INDEX idx_record_domainid_recordtype ON record (domain_id, record_type);

