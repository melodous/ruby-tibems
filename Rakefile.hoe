# -*- ruby -*-

if RUBY_PLATFORM =~ /java/
  require 'rake/javaextensiontask'
else
  require 'rake/extensiontask'
end

require "rubygems"
require "hoe"

##Hoe.plugin :clean
##Hoe.plugin :compiler
##Hoe.plugin :package

Hoe.plugin :git
Hoe.plugin :ignore

# Hoe.plugin :gem_prelude_sucks
# Hoe.plugin :inline
# Hoe.plugin :racc
# Hoe.plugin :rcov
# Hoe.plugin :rubyforge

Hoe.spec "tibems" do
  developer('Justo Alonso', 'justo.alonso@gmail.com')
  self.readme_file   = 'README.md'
  self.extra_rdoc_files  = FileList['*.rdoc']
  self.extra_dev_deps << ['rake-compiler']
  self.urls = [ "https://github.com/jalonsoa/ruby-tibems" ]
  self.licenses = [ "GPL-3.0" ]

  extra_dev_deps << ["rake-compiler", "~> 0.8"]

  files = Dir.glob("ext/**/*.{c,java,rb}") +
          Dir.glob("lib/**/*.rb")

  if RUBY_PLATFORM =~ /java/
    ##self.extra_platform = "java"
    files << "lib/tibems.jar"

    Rake::JavaExtensionTask.new('tibems', spec) do |ext|
      ext.lib_dir = File.join('lib', 'tibems')
    end
  else
    self.spec_extras = { :extensions => ["ext/tibems/extconf.rb"] }

    Rake::ExtensionTask.new('tibems', spec) do |ext|
      ext.lib_dir = File.join('lib', 'tibems')
    end
  end

end

## Rake::Task[:test].prerequisites << :compile

include Hoe::Git

desc "Print the current changelog."
task "changelog" do
  tag   = ENV["FROM"] || git_tags.last
  range = [tag, "HEAD"].compact.join ".."
  cmd   = "git log #{range} '--format=tformat:%B|||%aN|||%aE|||'"
  now   = Time.new.strftime "%Y-%m-%d"

  changes = `#{cmd}`.split(/\|\|\|/).each_slice(3).map { |msg, author, email|
              msg.split(/\n/).reject { |s| s.empty? }.first
            }.flatten.compact

  $changes = Hash.new { |h,k| h[k] = [] }

  codes = {
    "!" => :major,
    "+" => :minor,
    "*" => :minor,
    "-" => :bug,
    "?" => :unknown,
  }

  codes_re = Regexp.escape codes.keys.join

  changes.each do |change|
    if change =~ /^\s*([#{codes_re}])\s*(.*)/ then
      code, line = codes[$1], $2
    else
      code, line = codes["?"], change.chomp
    end

    $changes[code] << line
  end

  puts "=== #{ENV['VERSION'] || 'NEXT'} / #{now}"
  puts
  changelog_section :major
  changelog_section :minor
  changelog_section :bug
  changelog_section :unknown
  puts
end

# vim: syntax=ruby
