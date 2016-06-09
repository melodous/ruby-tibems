TibEMS
=======

 Is a Ruby interface to the TIBCO EMS (tm) C library

urls
-----

- https://github.com/jalonsoa/ruby-tibems

Description
------------

- FIX (describe your package)

Features/Problems
------------------

- Only TibEMSAdmin module implemented: get_info: get information and stats from server, topics and queue

Synopsis
-------------

-  FIX (code sample of usage)

Requirements
---------------------

- You need the TIBCO EMS C development library (include a lib files)

Install
---------

* FIX (sudo gem install, anything else)

Developers
-----------------

After checking out the source, run:

>  $ rake compile -- --with-tibems-dir=<tibco_esm_dir>

Then, you can check with irb:

>  $ irb -I./lib -I./vendor/ -r tibems
>  irb(main):006:0* admin = TibEMS::Admin.new(:url => "tcp://localhost:7222", :user => "admin", :pass => "password")
>  TibEMS::Error: 2016-05-25 01:42:49: Server not connected
>  	from /home/justo/dia/esb/ems/tibco-ems-dev/ruby/tibems/lib/tibems/admin.rb:20:in `create'
>  	from /home/justo/dia/esb/ems/tibco-ems-dev/ruby/tibems/lib/tibems/admin.rb:20:in `initialize'
>  	from (irb):6:in `new'
>  	from (irb):6
>  	from /usr/bin/irb:12:in `<main>'


This task will install any missing dependencies, run the tests/specs,
and generate the RDoc.

License
------------

(GNU GENERAL PUBLIC LICENSE) Show LICENSE file

