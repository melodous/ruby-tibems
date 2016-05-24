# encoding: UTF-8

module TibEMS
  class Error < StandardError
    ENCODE_OPTS = {
      :undef => :replace,
      :invalid => :replace,
      :replace => '?'.freeze,
    }.freeze

    attr_reader :error_number, :error_message

    def initialize(msg)
      @server_version ||= nil

      super(clean_message(msg))
    end

    def self.new_with_args(msg, server_version, error_number, error_message)
      err = allocate
      err.instance_variable_set('@server_version', server_version)
      err.instance_variable_set('@error_number', error_number)
      err.instance_variable_set('@error_state', error_message.respond_to?(:encode) ? error_message.encode(ENCODE_OPTS) : error_message)
      err.send(:initialize, msg)
      err
    end

    private

    def clean_message(message)
      return message unless message.respond_to?(:encode)

      message.encode(ENCODE_OPTS)
      ##message.encode(Encoding::UTF_8, ENCODE_OPTS)
    end
  end
end
