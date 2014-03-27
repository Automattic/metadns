
process.title = 'watcher.JS';

global.config = require( './config' );

o_log4js = require( 'log4js' );
o_log4js.configure( {
  appenders: [ {
		'type'      : 'file',
		'filename'  : 'logs/watcher.log',
		'maxLogSize': 10485760,
		'backups'   : 10,
		'category'  : 'flog',
		'levels'    : 'DEBUG',
		}
	]
});

global.logger = o_log4js.getLogger( 'flog' );
global.logger.PatternLayout = '%d{HH:mm:ss,SSS} p m';
global.arrServers = new Array();

var _stats    = require( './stats'    );
var _listener = require( './listener' );
var _scribe   = require( './scribe'   );

var s_config_file = 'config/watcher.json';
var args = process.argv.slice(2);

for ( var i_loop = 0; i_loop < args.length; i_loop++ ) {
	if ( '-c' == args[i_loop] ) {
		i_loop++;
		if ( i_loop < args.length ) {
			s_config_file = args[i_loop];
		}
	}
}

function graceful_shutdown() {
	logger.debug( 'shutting down.' );
	process.exit( 0 );
}

global.logger.debug( 'booting watcher.JS' );

process.on( 'SIGINT', graceful_shutdown );
process.on( 'SIGHUP', function () { config.load( s_config_file ); } );

process.on( 'uncaughtException', function( err_desc ) {
	logger.error( 'uncaughtException error: ' + err_desc );
});

config.load( s_config_file );

setTimeout( _scribe.update_pool_list, 1000 );
setTimeout( _scribe.check_servers, 3000 );
