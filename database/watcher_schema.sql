
/*
 * Drop tables in the correct order, so as not to violate foreign keys
*/

DROP TABLE IF EXISTS `monitor_log`;
DROP TABLE IF EXISTS `monitor_log_type`;
DROP TABLE IF EXISTS `monitor`;
DROP TABLE IF EXISTS `monitor_type`;

CREATE TABLE `monitor` (
	`monitor_id` int(11) NOT NULL AUTO_INCREMENT,
	`monitor_name` varchar(50) NOT NULL,
	`secret_hash` varchar(50) NOT NULL,
	`last_contact` datetime DEFAULT NULL,
	`ip_address` varchar(30) NOT NULL,
	`active` tinyint(1) DEFAULT '1',
  PRIMARY KEY (`monitor_id`)
) ENGINE=InnoDB;

CREATE TABLE `monitor_log_type` (
	`monitor_log_type_id` int(11) NOT NULL AUTO_INCREMENT,
	`monitor_log_type_name` varchar(255) NOT NULL,
	`active` tinyint(1) NOT NULL DEFAULT '1',
	PRIMARY KEY (`monitor_log_type_id`)
) ENGINE=InnoDB;

CREATE UNIQUE INDEX `uniq_idx_monitor_log_typename` ON `monitor_log_type` ( `monitor_log_type_name` );

/* These monitor_log_type_id's are used in the PHP code as constants to log certain monitor actions */
INSERT INTO `metadns`.`monitor_log_type` VALUES (1, 'Removed from Pool', 1),(2, 'Placed back in the Pool', 1),
												(3, 'Pool hash ring weight changed', 1),(4, 'Pool list fetch', 1);

CREATE TABLE `monitor_log` (
	`monitor_log_id` int(11) NOT NULL AUTO_INCREMENT,
	`monitor_log_type_id` int(2) NOT NULL,
	`monitor_id` int(11) NOT NULL COMMENT 'One of the monitor IDs is saved as the auth ID responsible for the action',
	`pool_id` int(11) DEFAULT NULL,
	`data` varchar(10) DEFAULT '',
	`action_date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
	PRIMARY KEY (`monitor_log_id`),
	CONSTRAINT `fk_monitor_log_monitor`
		FOREIGN KEY ( `monitor_id` )
		REFERENCES `monitor` ( `monitor_id` )
		ON DELETE RESTRICT
		ON UPDATE CASCADE,
	CONSTRAINT `fk_monitor_log_monitor_log_type`
		FOREIGN KEY ( `monitor_log_type_id` )
		REFERENCES `monitor_log_type` ( `monitor_log_type_id` )
		ON DELETE RESTRICT
		ON UPDATE CASCADE
) ENGINE=InnoDB;


CREATE TABLE `monitor_type` (
	`monitor_type_id` int(2) NOT NULL AUTO_INCREMENT,
	`monitor_type_name` varchar(50) NOT NULL,
	`active` tinyint(1) DEFAULT '1',
	PRIMARY KEY (`monitor_type_id`)
) ENGINE=InnoDB;

/* These monitor_type_id's are used as constants in the watcher.JS code to know which type of monitor to initiate */
INSERT INTO `metadns`.`monitor_type` VALUES (1, 'Ping Monitor', 1), (2, 'HTTP Service Monitor', 1);

