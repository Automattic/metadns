
const PING_CHECK      = "1";
const PING_NUM_CHECKS = 5;
const PING_NUM_PASS   = 4;

const HTTP_CHECK      = "2";
const HTTP_PORT       = 80;

const CHECK_INTERVAL_MS      = 15000;
const MAX_NO_RESPONSE_MS     = 30000;
const POOL_FETCH_INTERVAL_MS = 120000;
const PEER_CONFIRM_LIMIT     = 3;

const HOST_ONLINE     = 0
const HOST_OFFLINE    = 1
const HOST_UNKNOWN    = 2
const HOST_NO_CONTACT = 3

var _watcher = require( './watcher.node' );
var _speaker = require( './speaker' );
var run_checks = true;

var scribe = {
	check_servers: function () {
				try {
					if ( run_checks ) {
						logger.trace( 'starting server service checking: ' );
						for ( var count in global.arrServers ) {
							switch ( global.arrServers[count].mon_t ) {
								case PING_CHECK: {
									_watcher.ping_check( global.arrServers[count].ip, PING_NUM_CHECKS, PING_NUM_PASS,
														Number( global.arrServers[count].id ), scribe.process_results_callback );
									break;
								}
								case HTTP_CHECK: {
									_watcher.http_check( global.arrServers[count].ip, HTTP_PORT,
														Number( global.arrServers[count].id ), scribe.process_results_callback );
									break;
								}
								default: {
									logger.error( global.arrServers[count].id + ' unsupported network check [' + global.arrServers[count].mon_t +
												'] for host ' + global.arrServers[count].ip );
									break;
								}
							}
						}
						logger.trace( 'finished service checks' );
					} else {
						logger.trace( 'checking server responses' );
						var now = new Date();
						for ( var count in global.arrServers ) {
							if ( ( null != global.arrServers[ count ].last_contact ) && ( 1 == global.arrServers[ count ].in ) ) {
								if ( ( now - global.arrServers[ count ].last_contact ) > MAX_NO_RESPONSE_MS )
									var checker = new scribe.check_host_down( global.arrServers[ count ].id );
							}
						}
					}
				}
				catch ( Exception ) {
					logger.error( 'error processing server array: ' + Exception.toString() );
				}
				finally {
					run_checks = ! run_checks;
					setTimeout( scribe.check_servers, CHECK_INTERVAL_MS );
				}
			},

	process_results_callback: function( server_id, rtt ) {
				logger.debug( server_id + ': rtt: ' + rtt );
				if ( rtt > 0 ) {
					for ( var count in global.arrServers ) {
						if ( global.arrServers[ count ].id == server_id ) {
							global.arrServers[ count ].last_rtt = rtt;
							global.arrServers[ count ].last_contact = new Date();

							if ( 0 == global.arrServers[ count ].in )
								var checker = new scribe.check_host_up( server_id );

							global.arrServers[ count ].peer_confirms_offline = 0;
							break;
						}
					}
				} else {
					for ( var count in global.arrServers ) {
						if ( global.arrServers[ count ].id == server_id ) {
							global.arrServers[ count ].last_rtt = 0;
							global.arrServers[ count ].peer_confirms_online = 0;
							break;
						}
					}
				}
			},

	process_results: function( oServer, result ) {
				logger.debug( oServer.id + ' result: ' + result );
				if ( result > 0 ) {
					oServer.last_rtt = result;
					oServer.last_contact = new Date();

					if ( 0 == oServer.in )
						var checker = new scribe.check_host_up( oServer.id );

					oServer.peer_confirms_offline = 0;
				} else {
					oServer.last_rtt = 0;
					oServer.peer_confirms_online = 0;
				}
			},

	update_pool_list: function () {
				try {
					var dbArray = global.config.get( 'master_db' );
					for ( var count in dbArray ) {
						logger.trace( 'requesting pool-list from ' + dbArray[ count ].host + ':' + dbArray[ count ].port );
						_speaker.ask_server_question( dbArray[ count ], '/pool/get-list', 0, scribe.update_pool_list_callback );
					}
				}
				catch ( Exception ) {
					logger.error( 'scribe update_pool_list error: ' + Exception.toString() );
				}
				finally {
					dbArray = null;
					delete dbArray;
					setTimeout( scribe.update_pool_list, POOL_FETCH_INTERVAL_MS );
				}
			},

	update_pool_list_callback: function ( p_data ) {
				if ( undefined == p_data.length )
					return;

				logger.trace( 'received ' + p_data.length + ' servers' );

				for ( check in global.arrServers )
					global.arrServers[ check ].active = false;

				for ( var count in p_data ) {
					var have = false;
					var check = -1;
					for ( check in global.arrServers ) {
						if ( global.arrServers[check].id == p_data[count].id ) {
							if ( p_data[count].mon_t != global.arrServers[ check ].mon_t ) {
								logger.debug( global.arrServers[check].id + ': changing monitor_type from ' + global.arrServers[ check ].mon_t + ' to ' + p_data[count].mon_t );
								global.arrServers[ check ].mon_t = p_data[count].mon_t;
							}
							if ( p_data[count].ip != global.arrServers[ check ].ip ) {
								logger.debug( global.arrServers[check].id + ': changing host from ' + global.arrServers[ check ].ip + ' to ' + p_data[count].ip );
								global.arrServers[ check ].ip = p_data[count].ip;
							}
							if ( p_data[count].in != global.arrServers[ check ].in ) {
								logger.debug( global.arrServers[check].id + ': changing in_pool from ' + global.arrServers[ check ].in + ' to ' + p_data[count].in );
								global.arrServers[ check ].in = p_data[count].in;
							}
							if ( p_data[count].hw != global.arrServers[ check ].hw ) {
								logger.debug( global.arrServers[check].id + ': changing hash_weight from ' + global.arrServers[ check ].hw + ' to ' + p_data[count].hw );
								global.arrServers[ check ].hw = p_data[count].hw;
							}
							have = true;
							global.arrServers[ check ].active = true;
							break;
						}
					}
					if ( ! have ) {
						logger.debug( 'adding ' + p_data[count].id + ' : ' + p_data[count].ip );
						p_data[count].last_contact = null;
						p_data[count].last_rtt = 0;
						p_data[count].peer_confirms_offline = 0;
						p_data[count].peer_confirms_online = 0;
						p_data[count].active = true;
						global.arrServers.push( p_data[count] );
					}
				}

				for ( check in global.arrServers ) {
					if ( false == global.arrServers[ check ].active ) {
						logger.debug( 'removing ' + global.arrServers[ check ].id + ' : ' + global.arrServers[ check ].ip );
						global.arrServers.splice( check, 1 );
					}
				}
			},

	check_host_down: function( p_pool_id ) {
				logger.debug( 'host pool_id : ' + p_pool_id + ' has timed out - confirming with peers' );
				var peerArray = global.config.get( 'peer' );

				for ( var count in peerArray ) {
					logger.debug( 'calling on peer : ' + peerArray[ count ].host + ':' + peerArray[ count ].port );
					_speaker.ask_peer_question( peerArray[ count ], '/get/confirm-host-status',
												p_pool_id, scribe.check_host_down_callback );
				}
			},

	check_host_down_callback: function( p_pool_id, p_status_id ) {
				logger.debug( 'peer msg - check_host_down_callback - pool_id: ' + p_pool_id + ', status_id: ' + p_status_id );

				if ( HOST_OFFLINE == p_status_id ) {
					for ( var count in global.arrServers ) {
						if ( p_pool_id == global.arrServers[ count ].id ) {
							if ( 1 == global.arrServers[ count ].in )
								global.arrServers[ count ].peer_confirms_offline++

							logger.debug( 'peer_confirms_offline: ' + global.arrServers[ count ].peer_confirms_offline + ' >= ' + PEER_CONFIRM_LIMIT );

							if ( global.arrServers[ count ].peer_confirms_offline >= PEER_CONFIRM_LIMIT ) {
								var dbArray = global.config.get( 'master_db' );

								for ( var count in dbArray )
									_speaker.ask_server_question( dbArray[ count ], '/pool/set-out_pool',
																p_pool_id, scribe.confirm_pool_out_callback );
							}
							break;
						}
					}
				} else
					logger.debug( 'peer does NOT confirm host is offline' );
			},

	confirm_pool_out_callback: function ( p_data ) {
				logger.debug( 'server msg - confirm_pool_out_callback - pool_id: ' + p_data.pool_id + ', response: ' + p_data.result );

				if ( 'success' == p_data.result ) {
					for ( var count in global.arrServers ) {
						if ( p_data.pool_id == global.arrServers[ count ].id ) {
							global.arrServers[ count ].in = 0;
							global.arrServers[ count ].peer_confirms_offline = 0;
							global.arrServers[ count ].peer_confirms_online = 0;
							logger.debug( 'host ' + p_data.pool_id + ' set as "out of pool"' );
							break;
						}
					}
				}
			},

	check_host_up: function( p_pool_id ) {
				logger.debug( 'host pool_id : ' + p_pool_id + ' is back online - confirming status with peers' );
				var peerArray = global.config.get( 'peer' );

				for ( var count in peerArray ) {
					logger.debug( 'calling on peer : ' + peerArray[ count ].host + ':' + peerArray[ count ].port );
					_speaker.ask_peer_question( peerArray[ count ], '/get/confirm-host-status',
												p_pool_id, scribe.check_host_up_callback );
				}
			},

	check_host_up_callback: function( p_pool_id, p_status_id ) {
				logger.debug( 'peer msg - check_host_up_callback - pool_id: ' + p_pool_id + ', status_id: ' + p_status_id );

				if ( HOST_ONLINE == p_status_id ) {
					for ( var count in global.arrServers ) {
						if ( p_pool_id == global.arrServers[ count ].id ) {
							global.arrServers[ count ].peer_confirms_online++

							logger.debug( 'peer_confirms_online: ' + global.arrServers[ count ].peer_confirms_online + ' >= ' + PEER_CONFIRM_LIMIT );

							if ( global.arrServers[ count ].peer_confirms_online >= PEER_CONFIRM_LIMIT ) {
								var dbArray = global.config.get( 'master_db' );

								for ( var count in dbArray )
									_speaker.ask_server_question( dbArray[ count ], '/pool/set-in_pool',
																p_pool_id, scribe.confirm_pool_in_callback );
							}
							break;
						}
					}
				} else
					logger.debug( 'peer does NOT confirm host is online' );
			},

	confirm_pool_in_callback: function ( p_data ) {
				logger.debug( 'server msg - confirm_pool_in_callback - pool_id: ' + p_data.pool_id + ', response: ' + p_data.result );

				if ( 'success' == p_data.result ) {
					for ( var count in global.arrServers ) {
						if ( p_data.pool_id == global.arrServers[ count ].id ) {
							global.arrServers[ count ].in = 1;
							global.arrServers[ count ].peer_confirms_offline = 0;
							global.arrServers[ count ].peer_confirms_online = 0;
							logger.debug( 'host ' + p_data.pool_id + ' set as "in pool"' );
							break;
						}
					}
				}
			},

	confirm_host_status_for_peer: function( p_pool_id ) {
				var now = new Date();
				for ( var count in global.arrServers ) {
					if ( p_pool_id == global.arrServers[ count ].id ) {
						if ( null != global.arrServers[ count ].last_contact ) {
							if ( ( now - global.arrServers[ count ].last_contact ) > MAX_NO_RESPONSE_MS ) {
								logger.debug( 'host with pool_id ' + p_pool_id + ' is offline' );
								return HOST_OFFLINE;
							} else {
								logger.debug( 'host with pool_id ' + p_pool_id + ' is online' );
								return HOST_ONLINE;
							}
						} else {
							logger.debug( 'host with pool_id ' + p_pool_id + ' has never been contacted since boot-up' );
							return HOST_NO_CONTACT;
						}
					}
				}
				logger.debug( 'host with pool_id ' + p_pool_id + ' is unknown to me' );
				return HOST_UNKNOWN;
			}
}

module.exports = scribe;

