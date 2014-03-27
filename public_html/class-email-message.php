<?php

if ( ! class_exists( 'Email_Message' ) ) {
	class Email_Message {

		const PROBLEM     = 0;
		const RECOVERY    = 1;
		const INFORMATION = 2;

		private $_email_from;
		private $_email_to;
		private $_email_message;
		private $_email_message_type;

		function __construct() {
			$this->_email_from = gethostname();
			$this->_email_to = '';
			$this->_email_message = '';
			$this->_email_message_type = Email_Message::INFORMATION;
		}

		function __destruct() {
			;
		}

		public function __set( $name, $value ) {
			$var = '_' . $name;
			$this->$var = $value;
		}

		public function __get( $name ) {
			$var = '_' . $name;
			if ( isset( $this->$var ) )
				return $this->$var;
			else
				return '';
		}

		public function send_email() {
			$msg_title = "MetaDNS - Watcher : ";
			switch ( $this->_email_message_type ) {
				case Email_Message::PROBLEM:
					$msg_title .= "PROBLEM";
					break;
				case Email_Message::RECOVERY:
					$msg_title .= "RECOVERY";
					break;
				case Email_Message::INFORMATION:
					$msg_title .= "INFO";
					break;
			}
			if ( METADNS_DEBUG )
				error_log( date( 'Y-m-d H:i:s' ) . " sending an email:\r\nTitle: " . $msg_title . "\r\n" . $this->generate_body(), 3, '/tmp/metadns_debug_log' );

			@mail( $this->_email_to, $msg_title, $this->generate_body() );
		}

		private function generate_body() {
			$body = "\r\nDate:\t\t" . date( 'r' ) . "\r\n";
			$body .= "Host:\t\t" . $this->_email_from . "\r\n";
			$body .= "Action:\t\t";
			switch ( $this->_email_message_type ) {
				case Email_Message::PROBLEM:
					$body .= "Removed from the Pool\r\n";
					break;
				case Email_Message::RECOVERY:
					$body .= "Placed back into the Pool\r\n";
					break;
				case Email_Message::INFORMATION:
					$body .= "HashRing weight adjusted\r\n";
					break;
			}
			$body .= "Description:\t" . $this->_email_message . "\r\n";
			$body .= "\r\nMetaDNS - Watchers\r\n";

			return $body;
		}
	}
}
