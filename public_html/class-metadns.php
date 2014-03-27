<?php

/* These need to be configured for the service to function correctly */
define( 'NOTIFICATION_EMAIL_RECIPIENT', '<email address>' );
define( 'NOTIFICATION_EMAIL_RECIPIENT_DEBUG', '<email address>' );

define( 'IRC_CHANNEL_OR_USER', '@someone or #someroom' );
define( 'IRC_CHANNEL_OR_USER_DEBUG', '@someone or #someroom' );
/* end config */

define( 'METADNS_DEBUG'   , 0 );

define( 'POOL_TAKEN_OUT'  , 1 );
define( 'POOL_PUT_IN'     , 2 );
define( 'ADJUSTED_WEIGHT' , 3 );
define( 'POOL_FETCH'      , 4 );

require_once './class-database.php';
require_once './class-healthmonitor.php';
require_once './class-irc-message.php';
require_once './class-email-message.php';
require_once './class-pool.php';

if ( ! class_exists( 'Meta_DNS' ) ) {
	class Meta_DNS {

		private $request;
		private $health_monitor;
		private $irc_messenger;
		private $postman;

		function __construct() {
			$this->request = NULL;
			$this->health_monitor = NULL;

			$this->irc_messagenger = new IRC_Message();
			$this->irc_messagenger->irc_bot_name = 'metadns-watcher';
			if ( METADNS_DEBUG )
				$this->irc_messagenger->irc_channel_or_user = IRC_CHANNEL_OR_USER_DEBUG;
			else
				$this->irc_messagenger->irc_channel_or_user = IRC_CHANNEL_OR_USER;

			$this->postman = new Email_Message();
			if ( METADNS_DEBUG )
				$this->postman->email_to = NOTIFICATION_EMAIL_RECIPIENT_DEBUG;
			else
				$this->postman->email_to = NOTIFICATION_EMAIL_RECIPIENT;
		}

		function __destruct() {
			global $db_h;
			if ( $db_h )
				$db_h->close();
		}

		public function validate_client() {
			$this->request = json_decode( $_REQUEST[ 'data' ] );

			if ( ! ( is_object( $this->request ) ) || ( 0 == strlen( $this->request->auth ) ) )
				return false;
			else {
				$this->health_monitor = new Health_Monitor();
				return $this->health_monitor->validate( $this->request->auth, $this->get_client_ip() );
			}
		}

		public function is_secure_conn() {
			if ( isset( $_SERVER['HTTPS'] ) && ( 'on' == $_SERVER['HTTPS'] ) )
				return true;
			elseif ( isset($_SERVER['HTTP_X_FORWARDED_PROTO'] ) &&
					( 'https' == $_SERVER['HTTP_X_FORWARDED_PROTO'] ) )
				return true;
			elseif ( isset($_SERVER['HTTP_X_FORWARDED_SSL'] ) &&
					( 'on' == $_SERVER['HTTP_X_FORWARDED_SSL'] ) )
				return true;
			else
				return false;
		}

		public function parse_request() {
			switch ( $this->request->action ) {
				case '/pool/get-list':
					$this->send_pool_list();
					break;
				case '/pool/set-in_pool':
					$this->pool_back_in();
					break;
				case '/pool/set-out_pool':
					$this->pool_take_out();
					break;
				case '/pool/set-pool_weight':
					$this->pool_set_weight();
					break;
				default:
					$this->display_error();
			}
		}

		public function security_error() {
			header( 'Content-Type: text/plain' );
			header( 'HTTP/1.1 403 Forbidden' );
			header( 'Expires: ' . gmdate( 'D, d M Y H:i:s', 0) . ' GMT' );
			header( 'Connection: close' );
			die( 'You do not have the required permissions to access this service.' );
		}

		private function display_error() {
			header( 'Content-Type: text/plain' );
			header( 'HTTP/1.1 404 Not Found' );
			header( 'Expires: ' . gmdate( 'D, d M Y H:i:s', 0) . ' GMT' );
			header( 'Connection: close' );
			die( 'Incorrect parameters.' );
		}

		private function send_pool_list() {

			$res_array = $this->health_monitor->get_ip_pools();

			if ( is_array( $res_array ) ) {
				$this->send_data( $res_array );
				$this->health_monitor->log( POOL_FETCH );
			} else {
				$this->send_response( 'no pools' );
			}
		}

		private function pool_back_in() {

			$o_pool = new Pool( $this->request->pool_id );

			if ( is_object( $o_pool ) && ( NULL != $o_pool->pool_id ) ) {
				if ( ! $o_pool->is_in_pool() ) {
					if ( $o_pool->put_back_into_pool() ) {
						$this->send_response( 'success' );
						$this->health_monitor->log( POOL_PUT_IN, $this->request->pool_id );

						$this->irc_messagenger->irc_message_type = IRC_Message::RECOVERY;
						$this->irc_messagenger->irc_message = $o_pool->content . ' has been added back into the active DNS IP Pool.';
						$this->irc_messagenger->send_message();

						$this->postman->email_from = $this->health_monitor->get_monitor_name();
						$this->postman->email_message_type = Email_Message::RECOVERY;
						$this->postman->email_message = $o_pool->content . ' has been added back into the active DNS IP Pool.';
						$this->postman->send_email();
					} else
						$this->send_response( 'failed' );
				} else
					$this->send_response( 'notreq' );
			} else
				$this->send_response( 'failed' );
		}

		private function pool_take_out() {

			$o_pool = new Pool( $this->request->pool_id );

			if ( is_object( $o_pool ) && ( NULL != $o_pool->pool_id ) ) {
				if ( $o_pool->is_in_pool() ) {
					if ( $o_pool->take_out_of_pool() ) {
						$this->send_response( 'success' );
						$this->health_monitor->log( POOL_TAKEN_OUT, $this->request->pool_id );

						$this->irc_messagenger->irc_message_type = IRC_Message::PROBLEM;
						$this->irc_messagenger->irc_message = $o_pool->content . ' has been removed from the DNS IP Pool.';
						$this->irc_messagenger->send_message();

						$this->postman->email_from = $this->health_monitor->get_monitor_name();
						$this->postman->email_message_type = Email_Message::PROBLEM;
						$this->postman->email_message = $o_pool->content . ' has been removed from the DNS IP Pool.';
						$this->postman->send_email();
					} else
						$this->send_response( 'failed' );
				} else
					$this->send_response( 'notreq' );
			} else
				$this->send_response( 'failed' );
		}

		private function pool_set_weight() {

			$o_pool = new Pool( $this->request->pool_id );

			if ( is_object( $o_pool ) && ( NULL != $o_pool->pool_id ) && ( $o_pool->update_weight( $weight ) ) ) {
				$this->send_response( 'success' );
				$this->health_monitor->log( ADJUSTED_WEIGHT, $this->request->pool_id, $this->request->hw );

				$this->irc_messagenger->irc_message_type = IRC_Message::INFORMATION;
				$this->irc_messagenger->irc_message = $o_pool->content . "'s hash ring weighting has been adjusted.";
				$this->irc_messagenger->send_message();

				$this->postman->email_from = $this->health_monitor->get_monitor_name();
				$this->postman->email_message_type = Email_Message::INFORMATION;
				$this->postman->email_message = $o_pool->content . "'s hash ring weighting has been adjusted.";
				$this->postman->send_email();
			} else
				$this->send_response( 'failed' );
		}

		private function send_response( $action_result ) {
			$response = array (
							'result'=>'' . $action_result . '',
							'pool_id'=>'' . ( isset( $this->request->pool_id ) ? $this->request->pool_id : '0' ) . '',
						);
			$this->send_data( $response );
		}

		private function send_data( $send_array ) {
			$body = json_encode( $send_array );
			header( 'Content-Type: application/json' );
			header( 'Content-Length: ' . strlen( $body ) );
			header( 'Connection: close' );
			echo $body;
		}

		private function get_client_ip() {
			 $ipaddress = '0.0.0.0';
			 if ( isset( $_SERVER['HTTP_CLIENT_IP'] ) && ( 0 != strlen( $_SERVER['HTTP_CLIENT_IP'] ) ) )
				 $ipaddress = $_SERVER['HTTP_CLIENT_IP'];
			 else if ( isset( $_SERVER['HTTP_X_FORWARDED_FOR'] ) && ( 0 != strlen( $_SERVER['HTTP_X_FORWARDED_FOR'] ) ) )
				 $ipaddress = $_SERVER['HTTP_X_FORWARDED_FOR'];
			 else if ( isset( $_SERVER['HTTP_X_FORWARDED'] ) && ( 0 != strlen( $_SERVER['HTTP_X_FORWARDED'] ) ) )
				 $ipaddress = $_SERVER['HTTP_X_FORWARDED'];
			 else if ( isset( $_SERVER['HTTP_FORWARDED_FOR'] ) && ( 0 != strlen( $_SERVER['HTTP_FORWARDED_FOR'] ) ) )
				 $ipaddress = $_SERVER['HTTP_FORWARDED_FOR'];
			 else if ( isset( $_SERVER['HTTP_FORWARDED'] ) && ( 0 != strlen( $_SERVER['HTTP_FORWARDED'] ) ) )
				 $ipaddress = $_SERVER['HTTP_FORWARDED'];
			 else if ( isset( $_SERVER['REMOTE_ADDR'] ) && ( 0 != strlen( $_SERVER['REMOTE_ADDR'] ) ) )
				 $ipaddress = $_SERVER['REMOTE_ADDR'];
			 return $ipaddress;
		}
	}
}
