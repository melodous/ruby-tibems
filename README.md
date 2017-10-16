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

* If you install the java version, you need the tibco jars (see lib directory on your tibco ems installation)
* If you install the C version, you need the TIBCO EMS C development library

Install
---------

* If you install the java version, first you must add the tibco jars files to your local maven repository:

  https://maven.apache.org/guides/mini/guide-3rd-party-jars-local.html

> $ mvn install:install-file -Dfile=[path-to-tibjms.jar-file] -DgroupId=com.tibco -DartifactId=tibjms -Dversion=[tibco_version] -Dpackaging=jar
> $ mvn install:install-file -Dfile=[path-to-tibjmsadmin.jar-file] -DgroupId=com.tibco -DartifactId=tibjms-admin -Dversion=[tibco-version] -Dpackaging=jar

* If you install the C version, then you must add the parameter to define the tibco ems directory

> $ gem install &lt;gem-file&gt; -- --with-tibems-dir=&lt;tibco-ems-dev-dir&gt;

Developers
-----------------

After checking out the source, run:

* If you install the C version, then you must add the parameter to define the tibco ems directory

>  $ rake compile -- --with-tibems-dir=&lt;tibco-ems-dev-dir&gt;

* If you install de java version, don't forget add the tibco jars.

> $ jruby -S rake package --

> $ jruby -S gem install -l pkg/tibems-0.0.7-java.gem

Then, you can check with irb or jirb:

>  $ irb -I./lib -I./vendor/ -r tibems

> 2.3.0 :001 > admin = TibEMS::Admin.new(:url => "tcp://localhost:7222", :user => "admin", :pass => "password")

> TibEMS::Error: 2016-06-09 20:22:42: Server not connected

> &nbsp;&nbsp;&nbsp;&nbsp;from ./lib/tibems/admin.rb:36:in `create`

> &nbsp;&nbsp;&nbsp;&nbsp;from ./lib/tibems/admin.rb:36:in `initialize`

> &nbsp;&nbsp;&nbsp;&nbsp;from (irb):1:in `new`

> &nbsp;&nbsp;&nbsp;&nbsp;from (irb):1

> &nbsp;&nbsp;&nbsp;&nbsp;from ~/.rvm/rubies/ruby-2.3.0/bin/irb:11:in '&lt;main%gt;'

> 2.3.0 :002 > quit

License
------------

(GNU GENERAL PUBLIC LICENSE) Show LICENSE file
