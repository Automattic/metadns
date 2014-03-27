
var _https  = require( 'https'    );
var _url    = require( 'url'      );
var _fs     = require( 'fs'       );
var _scribe = require( './scribe' );

const PortNum = 7801;

var ssl_listner = function() {

	var routes = {
		'/' : function( request, response ) {
			response.writeHead( 404, { 'Content-Type': 'text/html' } );
			response.write( 'Unsupported call\n' );
			response.end();
		},

		'/get/confirm-host-status' : function( request, response ) {
			var _get = _url.parse( request.url, true ).query;
			if ( ( undefined != _get['d'] ) && ( "" != _get['d'] ) ) {
				var arrRequest = JSON.parse( _get['d'] );
				logger.trace( 'checking whether ' + arrRequest.pool_id + ' is online for peer' );
				response.writeHead( 200, { 'Content-Type': 'text/plain' } );
				response.write( '{"pool_id":"' + arrRequest.pool_id + '","reply":"' +
								_scribe.confirm_host_status_for_peer( arrRequest.pool_id ) + '"}' );
			} else {
				response.writeHead( 503, { 'Content-Type': 'text/plain' } );
				response.write( '{"pool_id":"' + arrRequest.pool_id + '","reply":"ERROR"}' );
				logger.error( 'malformed request from peer' );
			}
			response.end();
		},

		'/get/status' : function( request, response ) {
			logger.trace( 'status confirmation requested' );
			response.writeHead( 200, { 'Content-Type': 'text/plain' } );
			var error_msg = '';
			for ( check in global.arrServers ) {
				if ( 0 == global.arrServers[ check ].in ) {
					error_msg += global.arrServers[ check ].ip + ';'
				}
			}
			if ( '' != error_msg ) {
				error_msg = error_msg.substring( 0, error_msg.length - 1 );
				response.write( 'WARNING: The following pool servers are unreachable: ' + error_msg );
			} else {
				response.write( 'OK' );
			}
			response.end();
		}
	}

	var ssl_options = {
		key:  _fs.readFileSync('certs/watcher.key'),
		cert: _fs.readFileSync('certs/watcher.crt'),
	};

	var request_handler = function( request, response ) {
		var arr_req = request.url.toString().split( '?' );
		if ( arr_req instanceof Array ) {
			if( undefined === routes[ arr_req[0] ] ) {
				response.writeHead( 404, { 'Content-Type': 'text/plain' } );
				response.write( 'not found\n' );
				response.end();
			} else {
				routes[ arr_req[0] ].call( this, request, response );
			}
		} else {
			response.writeHead( 404, { 'Content-Type': 'text/plain' } );
			response.write( 'Unsupported call\n' );
			response.end();
			logger.debug( 'unsupported call: ' + request.url.toString() );
		}
	};

	var close_handler = function() {
		logger.debug( process.pid + ': HTTPS server has been shutdown.' );
	};

	var error_handler = function( err ) {
		logger.error( process.pid + ': HTTPS error encountered: ' + err );
	}

	var _server = _https.createServer( ssl_options ).
					 addListener( 'request', request_handler )
					.addListener( 'close', close_handler )
					.addListener( 'error', error_handler )
					.listen( PortNum );
};

new ssl_listner();
