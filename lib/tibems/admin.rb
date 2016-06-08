if defined?(RUBY_ENGINE) && RUBY_ENGINE == "jruby"
  # The line below caused a problem on non-GAE rack environment.
  # unless defined?(JRuby::Rack::VERSION) || defined?(AppEngine::ApiProxy)
  #
  # However, simply cutting defined?(JRuby::Rack::VERSION) off resulted in
  # an unable-to-load-nokogiri problem. Thus, now, Nokogiri checks the presense
  # of appengine-rack.jar in $LOAD_PATH. If Nokogiri is on GAE, Nokogiri
  # should skip loading xml jars. This is because those are in WEB-INF/lib and
  # already set in the classpath.
  unless $LOAD_PATH.to_s.include?("appengine-rack")
    require 'tibjmsadmin.jar'
  end
end

module TibEMS
  class Admin
    def initialize(opts = {})
      opts = TibEMS::Util.key_hash_as_symbols(opts)

      if RUBY_PLATFORM !~ /java/
        initialize_ext
      end

      ##ssl_options = opts.values_at(:sslkey, :sslcert, :sslca, :sslcapath, :sslcipher)
      ##ssl_set(*ssl_options) if ssl_options.any?

      url      = opts[:url] || opts[:host]
      user     = opts[:username] || opts[:user]
      pass     = opts[:password] || opts[:pass]

      # Correct the data types before passing these values down to the C level
      url  = url.to_s unless url.nil?
      user = user.to_s unless user.nil?
      pass = pass.to_s unless pass.nil?

      create url, user, pass
    end
  end
end
