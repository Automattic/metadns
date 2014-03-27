
const NETWORK_TIMEOUT_MS     = 20000;

var _https   = require( 'https' );
var _fs      = require( 'fs'    );
var ssl_key  = _fs.readFileSync('certs/watcher.key');
var ssl_cert = _fs.readFileSync('certs/watcher.crt');

var speaker = {
	is_ipaddress: function ( check_str ) {
				var octet = '(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])';
				var ip    = '(?:' + octet + '\\.){3}' + octet;
				var quad  = '(?:\\[' + ip + '\\])|(?:' + ip + ')';
				var check_regex  = new RegExp( '(' + quad + ')' );
				if ( check_regex.test( check_str ) )
					return true;
				else
					return false;
			},

	ask_peer_question: function ( p_peer, p_question, p_pool_id, call_back_func ) {
				try {
					var o_request     = new Object();
					o_request.pool_id = p_pool_id;
					var request_str   = JSON.stringify( o_request );

					var options = {
						hostname: p_peer.host,
						port:     p_peer.port,
						key:      ssl_key,
						cert:     ssl_cert,
						path:     p_question + '?d=' + request_str,
						method:   'GET',
						rejectUnauthorized: false,
					};

					logger.trace( 'asking peer ' + p_question + ', for pool_id ' + p_pool_id +
								', from ' + p_peer.host + ':' + p_peer.port );

					var response_handler = function( res ) {
						res.setEncoding( 'utf8' );
						var s_data = '';
						res.on( 'data', function ( response_data ) {
							s_data += response_data;
						});
						res.on( 'end', function() {
							if ( 200 == res.statusCode ) {
								var reply_data = new Object();
								try {
									reply_data = JSON.parse( s_data );
								}
								catch ( Exception ) {
									reply_data.pool_id = -1;
									reply_data.reply = "PARSE_ERROR";
								}
								call_back_func( reply_data.pool_id, reply_data.reply );
							} else {
								call_back_func( p_pool_id, 'CODE_ERR: ' + res.statusCode );
							}
						});
					};

					var close_handler = function() {
						logger.trace( 'closing connection' );
					};

					var error_handler = function( err ) {
						logger.error( 'error performing request: ' + err );
						call_back_func( p_pool_id, 'ERR_REQUEST: ' + err );
					};

					var timeout_handler = function() {
						logger.error( 'timed out performing a request to peer ' + p_peer.host );
						call_back_func( p_pool_id, 'TIMEOUT' );
					};

					var _request = _https.request( options ).
											 addListener( 'response', response_handler )
											.addListener( 'close', close_handler )
											.addListener( 'error', error_handler )
											.addListener( 'timeout', timeout_handler );
					_request.setTimeout( NETWORK_TIMEOUT_MS );
					_request.end();
				}
				catch ( Exception ) {
					logger.error( 'speaker:ask_peer_question error: ' + Exception.toString() );
					call_back_func( p_pool_id, 'EXCEPTION: ' + Exception.toString() );
				}
			},

	ask_server_question: function( p_master_db, p_question, p_pool_id, call_back_func ) {
				try {
					var o_request     = new Object();
					o_request.action  = p_question;
					o_request.auth    = global.config.get( 'auth_token' );
					o_request.pool_id = p_pool_id;
					var request_str   = JSON.stringify( o_request );
					o_request = null;
					delete o_request;

					var options = {
						hostname: p_master_db.host,
						port:     p_master_db.port,
						key:      ssl_key,
						cert:     ssl_cert,
						path:     '/index.php?data=' + request_str,
						method:   'GET',
						rejectUnauthorized: false,
					};

					logger.trace( 'asking server ' + p_question + ', for pool_id ' + p_pool_id +
								', from ' + p_master_db.host + ':' + p_master_db.port );

					var response_handler = function( res ) {
						res.setEncoding( 'utf8' );
						var s_data = '';
						res.on( 'data', function( response_data ) {
							s_data += response_data;
						});
						res.on( 'end', function() {
							var reply_data = new Object();
							if ( 200 == res.statusCode ) {
								try {
									reply_data = JSON.parse( s_data );
								}
								catch ( Exception ) {
									reply_data.pool_id = -1;
									reply_data.reply = "PARSE_ERROR";
								}
							} else {
								reply_data.pool_id = -1;
								reply_data.reply = 'CODE_ERR: ' + res.statusCode;
							}
							call_back_func( reply_data );
							reply_data = null;
							delete reply_data;
						});
					};

					var close_handler = function() {
						logger.trace( 'closing connection' );
					};

					var error_handler = function( err ) {
						logger.error( 'error performing request: ' + err );
					};

					var timeout_handler = function() {
						logger.error( 'timed out performing a request to server ' + p_master_db.host );
					};

					var _request = _https.request( options ).
											 addListener( 'response', response_handler )
											.addListener( 'close', close_handler )
											.addListener( 'error', error_handler )
											.addListener( 'timeout', timeout_handler );
					_request.setTimeout( NETWORK_TIMEOUT_MS );
					_request.end();
					request_str = null;
					options = null;
					delete request_str;
					delete options;
				}
				catch ( Exception ) {
					logger.error( 'speaker:ask_server_question error: ' + Exception.toString() );
				}
			}
}

module.exports = speaker;

