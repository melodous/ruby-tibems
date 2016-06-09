# encoding: UTF-8
require 'date'
require 'bigdecimal'

require 'tibems/version' unless defined? TibEMS::VERSION
require 'tibems/error'
if defined?(RUBY_ENGINE) && RUBY_ENGINE == "jruby"
  require 'jruby'
  require 'tibems/tibems.jar'
  org.jalonsoa.tibems.TibEMSService.new.basicLoad(JRuby.runtime)
else
  require 'tibems/tibems'
end
require 'tibems/admin'


# = TibEMS
#
# A modern, simple binding to TibEMS libraries
module TibEMS
end

# For holding utility methods
module TibEMS
  module Util
    #
    # Rekey a string-keyed hash with equivalent symbols.
    #
    def self.key_hash_as_symbols(hash)
      return nil unless hash
      Hash[hash.map { |k, v| [k.to_sym, v] }]
    end

    #
    # Thread#handle_interrupt is used to prevent Timeout#timeout
    # from interrupting query execution.
    #
    # Timeout::ExitException was removed in Ruby 2.3.0, 2.2.3, and 2.1.8,
    # but is present in earlier 2.1.x and 2.2.x, so we provide a shim.
    #
    if Thread.respond_to?(:handle_interrupt)
      require 'timeout'
      # rubocop:disable Style/ConstantName
      TimeoutError = if defined?(::Timeout::ExitException)
        ::Timeout::ExitException
      else
        ::Timeout::Error
      end
    end
  end
end
