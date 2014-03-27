<?php

define( 'IRC_CHAT_SERVER', '<chat server host>' );
define( 'IRC_CHAT_SERVER_PORT', '<chat server port>' );

if ( ! class_exists( 'IRC_Message' ) ) {
	class IRC_Message {

		const WARNING     = 0;
		const PROBLEM     = 1;
		const RECOVERY    = 2;
		const INFORMATION = 3;

		private $_irc_bot_name;
		private $_irc_channel_or_user;
		private $_irc_message;
		private $_irc_message_type;

		function __construct() {
			$this->_irc_bot_name = '';
			$this->_irc_channel_or_user = '';
			$this->_irc_message = '';
			$this->_irc_message_type = IRC_Message::WARNING;
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

		public function send_message() {
			$bot = @fsockopen( IRC_CHAT_SERVER, IRC_CHAT_SERVER_PORT, $errno, $errst, 0.1 );
			if ( ! $bot ) {
				error_log( "fsockopen() failed: send_message( '$this->_irc_channel_or_user', '$this->_irc_message' );" );
				return false;
			}
			$msg_prefix = "";
			switch ( $this->_irc_message_type ) {
				case IRC_Message::WARNING:
					$msg_prefix = "\x0307,01WARNING:\x03";
					break;
				case IRC_Message::PROBLEM:
					$msg_prefix = "\x0304,01PROBLEM:\x03";
					break;
				case IRC_Message::RECOVERY:
					$msg_prefix = "\x0303,01RECOVERY:\x03";
					break;
				case IRC_Message::INFORMATION:
					$msg_prefix = "\x0312,14INFO:\x03";
					break;
			}

			if ( METADNS_DEBUG )
				error_log( date( 'Y-m-d H:i:s' ) . ' sending "!' . $this->_irc_bot_name . ' ' . $this->_irc_channel_or_user .
							' ' . $msg_prefix . ' ' . $this->_irc_message . '"' . "\n", 3, '/tmp/metadns_debug_log' );

			@fputs( $bot, '!' . $this->_irc_bot_name . ' ' . $this->_irc_channel_or_user . ' ' . $msg_prefix . ' ' . $this->_irc_message );
			if ( ! @feof( $bot ) )
				@fclose( $bot );
		}
	}
}
