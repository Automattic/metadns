<?php
if ( ! class_exists( 'Health_Monitor' ) ) {
	class Health_Monitor {

		private $monitor_id;
		private $monitor_name;

		function __construct() {
			$this->monitor_id = 0;
			$this->monitor_name = '';
		}

		function __destruct() {
			;
		}

		public function get_monitor_name() {
			return $this->monitor_name;
		}

		public function validate( $p_auth, $p_ipaddress ) {
			global $db_h;

			$sql = "SELECT monitor_id, monitor_name FROM monitor WHERE secret_hash = '";
			$sql .= $p_auth ."' AND ip_address = '" . $p_ipaddress . "' AND ( active = true );";

			$res_array = $db_h->query_records( $sql );

			if ( is_array( $res_array ) && ( 1 == count( $res_array ) ) ) {
				$this->monitor_id   = $res_array[0]['monitor_id'];
				$this->monitor_name = $res_array[0]['monitor_name'];
				$this->update_last_contact();

				if ( METADNS_DEBUG )
					error_log( date( "Y-m-d H:i:s" ) . " validated monitor '" . $res_array[0]['monitor_name'] .
								 "' with IP " . $p_ipaddress . "\n", 3, "/tmp/metadns_debug_log" );
				return true;
			} else {
				if ( METADNS_DEBUG )
					error_log( date( "Y-m-d H:i:s" ) . " failed to validate monitor with IP " .
								$p_ipaddress . "\n", 3, "/tmp/metadns_debug_log" );
				return false;
			}
		}

		public function get_ip_pools() {
			global $db_h;

			$sql = "SELECT pool.monitor_type_id as 'mon_t', pool_id as 'id', content as 'ip', in_pool as 'in', hashring_weight as 'hw'
					FROM pool
					INNER JOIN monitor_type USING ( monitor_type_id )
					WHERE ( pool.active = true );";

			return $db_h->query_records( $sql );
		}

		public function log( $type, $poolid = 0, $data = '') {
			global $db_h;

			$sql = "INSERT INTO monitor_log ( monitor_log_type_id, monitor_id, pool_id, data ) VALUES ";
			$sql .= "( " . $type . ", " . $this->monitor_id . ", " . $poolid . ", '" . $data . "');";

			$db_h->query_exec( $sql );
		}

		private function update_last_contact() {
			global $db_h;

			$sql = 'UPDATE monitor SET last_contact = now()';
			$sql .= ' WHERE monitor_id = ' . $this->monitor_id . ';';

			$db_h->query_exec( $sql );
		}
	}
}
