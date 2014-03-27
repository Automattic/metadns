metadns
=======

Overview
--------

A backend module for PowerDNS that implements consistent hashing on the DNS results for a domain, so as to ensure optimised caching in a large datacenter environment.

The service also has an add-on module to perform remote availability monitoring and consequently add/remove records from the DNS results based on the health status of the individual records. This service can be found in the watcher directory.

Installation
------------

1) Download the PowerDNS source code.

2) Set the correct path the the source code in './src/Makefile'.

3) Run 'make; make install'.

4) Create the database tables on a MariaDB or MySQL server from the schemas provided in the database directory. Populate the tables :)

5) Configure the database auth settings in the PowerDNS config file provided in the pdns-config directory.

6) You are ready to run the metadns service now, by starting PowerDNS with the config from step 5. If you need the monitoring service, then continue to step 8.

7) Setup the 'public_html' folder as your SSL server root folder for the watcher service database communication.

8) Setup each of the watcher services config files. Pay careful attention to the auth tokens, which you would have set in the database as 'monitor.secret_hash' in step 4.

9) Start the watchers!
