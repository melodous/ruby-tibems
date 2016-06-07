module TibEMS
  class Admin
    def initialize(opts = {})
      opts = TibEMS::Util.key_hash_as_symbols(opts)

      initialize_ext

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
