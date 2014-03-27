<?php
if ( ! class_exists( 'Pool' ) ) {
	class Pool {

		private $_pool_id;
		private $_content;

		function __construct( $p_pool_id ) {
			global $db_h;

			$sql = 'SELECT pool_id, content
					FROM poolhouse
					INNER JOIN pool USING ( poolhouse_id )
					WHERE ( (pool_id = ' . $p_pool_id . ' ) AND
							( poolhouse.active = true ) AND
							( pool.active = true ) );';

			$res_array = $db_h->query_records( $sql );

			if ( is_array( $res_array ) && ( 1 == count( $res_array ) ) ) {
				$this->_pool_id = $res_array[0]['pool_id'];
				$this->_content = $res_array[0]['content'];

				if ( METADNS_DEBUG )
					error_log( date( "Y-m-d H:i:s" ) . " loaded pool " . $p_pool_id . "\n", 3, "/tmp/metadns_debug_log" );
			} else {
				if ( METADNS_DEBUG )
					error_log( date( "Y-m-d H:i:s" ) . " failed to load pool " . $p_pool_id . "\n", 3, "/tmp/metadns_debug_log" );

				$this->_pool_id = NULL;
				$this->_content = '';
			}
		}

		function __destruct() {
			;
		}

		public function __set( $name, $value ) {
			$var = '_' . $name;
			if ( isset( $this->$var ) )
				$this->$var = $value;
		}

		public function __get( $name ) {
			$var = '_' . $name;
			if ( isset( $this->$var ) )
				return $this->$var;
			else
				return '';
		}

		public function is_in_pool() {
			global $db_h;

			$sql = 'SELECT in_pool
					FROM pool
					WHERE pool_id = ' . $this->_pool_id . ';';

			$res_array = $db_h->query_records( $sql );

			if ( is_array( $res_array ) && ( 1 == count( $res_array ) ) ) {
				if ( 1 == $res_array[0]['in_pool'] )
					return true;
				else
					return false;
			} else
				return false;
		}

		public function put_back_into_pool() {
			global $db_h;

			$sql = 'UPDATE pool SET in_pool = true
					WHERE pool_id = ' . $this->_pool_id . ';';

			if ( $db_h->query_exec( $sql ) )
				return true;
			else
				return false;
		}

		public function take_out_of_pool() {
			global $db_h;

			$sql = 'UPDATE pool SET in_pool = false
					WHERE pool_id = ' . $this->_pool_id . ';';

			if ( $db_h->query_exec( $sql ) )
				return true;
			else
				return false;
		}

		public function update_weight( $weight ) {
			global $db_h;

			$sql = 'UPDATE pool SET hashring_weight = ' . $weight .
					' WHERE pool_id = ' . $this->_pool_id . ';';

			if ( $db_h->query_exec( $sql ) )
				return true;
			else
				return false;
		}
	}
}
