<?php
require_once "./class-metadns.php";

if ( class_exists( 'Meta_DNS' ) ) {
	$o_metadns = new Meta_DNS();
	if ( $o_metadns->validate_client() ) {
		$o_metadns->parse_request();
	} else {
		$o_metadns->security_error();
	}
} else {
	die( "The MetaDNS class has not been defined." );
}
?>
