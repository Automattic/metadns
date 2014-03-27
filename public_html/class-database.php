<?php
define( 'DB_NAME', '<db name>' );
define( 'DB_HOST', '<db host>' );
define( 'DB_USER', '<db username>' );
define( 'DB_PASSWORD', '<db password>' );

define( 'USE_MYSQLI', 0 );

if ( ! class_exists( 'Meta_Database' ) ) {
	class Meta_Database {

		private $sql_obj;
		private $result;

		function __construct() {

			if ( USE_MYSQLI ) {
				$this->sql_obj = mysqli_init();
				$this->sql_obj->real_connect( DB_HOST, DB_USER, DB_PASSWORD, DB_NAME );

				if ( 0 != $this->sql_obj->connect_errno ) {
					error_log( date( 'Y-m-d H:i:s' ) . ' Connection Error [' . $this->$sql_i->connect_errno .
													'] - ' . $this->$sql_i->connect_error . "\n", 3, '/tmp/metadns_debug_log' );
				}

				if ( METADNS_DEBUG )
					error_log( date( 'Y-m-d H:i:s' ) . ' ' . $this->sql_obj->host_info . "\n", 3, '/tmp/metadns_debug_log' );

			} else {
				$this->sql_obj = mysql_connect( DB_HOST, DB_USER, DB_PASSWORD );

				if ( false == $this->sql_obj ) {
					error_log( date( 'Y-m-d H:i:s' ) . ' Connection Error to ' . DB_HOST . ' for user ' . DB_USER . "\n", 3, '/tmp/metadns_debug_log' );
					die( 'Connection Error to ' . DB_HOST . ' for user ' . DB_USER );
				} else
					mysql_select_db( DB_NAME );
			}
			$this->result = NULL;
		}

		function __destruct() {
			;
		}

		public function query_exec( $update_cmd ) {
			if ( USE_MYSQLI )
				$this->result = $this->sql_obj->real_query( $update_cmd );
			else
				$this->result = mysql_query( $update_cmd );

			if ( $this->result ) {
				if ( METADNS_DEBUG )
					error_log( date( 'Y-m-d H:i:s' ) . ' Command Success: (' . $update_cmd . ")\n", 3, '/tmp/metadns_debug_log' );

				$this->result = NULL;
				return true;
			} else {
				if ( METADNS_DEBUG )
					error_log( date( 'Y-m-d H:i:s' ) . ' Command Failed: (' . $update_cmd . ")\n", 3, '/tmp/metadns_debug_log' );
				return false;
			}
		}

		public function query_results( $query ) {
			if ( USE_MYSQLI )
				$this->result = $this->sql_obj->query( $query );
			else
				$this->result = mysql_query( $query );

			if ( $this->result ) {
				if ( METADNS_DEBUG )
					error_log( date( 'Y-m-d H:i:s' ) . ' Query Success: (' . $query . ")\n", 3, '/tmp/metadns_debug_log' );

				$num_rows = 0;
				if ( USE_MYSQLI ) {
					$num_rows = $this->result->num_rows;
					$this->result->free();
				} else {
					$num_rows = mysql_num_rows( $this->result );
					mysql_free_result( $this->result );
				}
				$this->result = NULL;
				return $num_rows;
			} else {
				if ( METADNS_DEBUG )
					error_log( date( 'Y-m-d H:i:s' ) . ' Query Failed: (' . $query . ")\n", 3, '/tmp/metadns_debug_log' );
				return -1;
			}
		}

		public function query_records( $query ) {
			if ( USE_MYSQLI )
				$this->result = $this->sql_obj->query( $query );
			else
				$this->result = mysql_query( $query );

			if ( $this->result ) {
				if ( METADNS_DEBUG )
					error_log( date( 'Y-m-d H:i:s' ) . ' Query Success: (' . $query . ")\n", 3, '/tmp/metadns_debug_log' );
				return $this->get_results();
			} else {
				if ( METADNS_DEBUG )
					error_log( date( 'Y-m-d H:i:s' ) . ' Query Failed: (' . $query . ")\n", 3, '/tmp/metadns_debug_log' );
				return NULL;
			}
		}

		public function close() {
			if ( USE_MYSQLI )
				$this->sql_obj->close();
			else
				mysql_close( $this->sql_obj );
		}

		private function get_results() {
			if ( NULL != $this->result ) {
				$rows = array();
				if ( USE_MYSQLI ) {
					while ( $row = $this->result->fetch_assoc() ) {
						$rows[] = $row;
					}
					$this->result->free();
				} else {
					while ( $row = mysql_fetch_array( $this->result ) ) {
						$rows[] = $row;
					}
					mysql_free_result( $this->result );
				}
				$this->result = NULL;
				return $rows;
			} else
				return NULL;
		}
	}

	$db_h = new Meta_Database();
	if ( ! $db_h )
		die( 'Connection Error to ' . DB_HOST . ' for user ' . DB_USER );
}
