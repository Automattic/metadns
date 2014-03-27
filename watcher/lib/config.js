
var fs = require( 'fs' );

var config_loader = {
	_cache: null,
	load: function( s_file_name ) {
		this._cache = JSON.parse( fs.readFileSync( s_file_name ).toString() );
		return this._cache;
	},
	get: function( key ) {
		return this._cache[ key ];
	}
};

module.exports = config_loader;
