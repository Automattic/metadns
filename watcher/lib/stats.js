
const STATS_UPDATE_INTERVAL_MSECS = 15000;
const MAX_NO_RESPONSE_MSECS = 30000;

var fs = require( 'fs' );

var stats = {
	updateStats : function() {
			try {
				var counterFile = fs.createWriteStream( 'stats/counters.dat', { flags : "w" } );
				counterFile.once( 'open', function( fd ) {
					var now = new Date();
					var timed_out = 0;
					var total_rtt = 0;
					for ( var count in global.arrServers ) {
						if ( ( now - global.arrServers[count].last_contact ) > MAX_NO_RESPONSE_MSECS )
							timed_out++;
						else
							total_rtt += global.arrServers[count].last_rtt;
					}
					counterFile.write( 'hosts offline: ' + timed_out + '\n' );
					if ( ( global.arrServers.length - timed_out ) > 0 )
						counterFile.write( 'average rtt: ' + ( total_rtt / ( global.arrServers.length - timed_out ) / 1000 ) + '\n' );
					else
						counterFile.write( 'average rtt: 0\n' );

					counterFile.write( 'host\t\t\trtt (ms)\t\tlast_contact\n' );

					for ( var count in global.arrServers ) {
						counterFile.write( global.arrServers[count].ip + '\t\t' +
								( global.arrServers[count].last_rtt / 1000 ) + '\t\t' );

						if ( ( null == global.arrServers[count].last_contact ) || ( 0 == global.arrServers[count].last_contact ) )
							counterFile.write( "\t---\n" );
						else
							counterFile.write( global.arrServers[count].last_contact.toUTCString() + '\n' );
					}
					counterFile.end();
				});
			}
			catch ( Exception ) {
				logger.error( 'Error updating queue stats file: ' + Exception.toString() );
			}
			finally {
				setTimeout( stats.updateStats, ( STATS_UPDATE_INTERVAL_MSECS) );
			}
		}
}

setTimeout( stats.updateStats, STATS_UPDATE_INTERVAL_MSECS );
