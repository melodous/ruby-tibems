require "rake/extensiontask"
require 'rubygems'
require 'rubygems/package_task'

spec = Gem::Specification.new do |s|
  s.name = 'tibems'
  s.version = '0.0.1'
  s.authors = ['Justo Alonso']
  s.license = "MIT"
  s.email = ['justo.alonso@gmail.com']
  s.homepage = 'http://github.com/jalonsoa/tibems'
  s.rdoc_options = ["--charset=UTF-8"]
  s.summary = 'Interface to the tibems C libraries'

  s.files         = `git ls-files`.split($/)
  s.executables   = s.files.grep(%r{^bin/}) { |f| File.basename(f) }
  s.test_files    = s.files.grep(%r{^(test|spec|features)/})
  s.require_paths = ["lib", "ext"]

  s.platform = Gem::Platform::RUBY
  s.extensions = FileList["ext/**/extconf.rb"]

  s.required_ruby_version = ">= 1.9.3"

  s.add_development_dependency "bundler", "~> 1.3"
  s.add_development_dependency "rake", "~> 1.8"
  s.add_development_dependency "rake-compiler", "~> 1.8"
end

Gem::PackageTask.new(spec) do |pkg|
  pkg.need_zip = true
  pkg.need_tar = true
end

Rake::ExtensionTask.new("tibems",spec) do |ext|
  ext.lib_dir = "lib/tibems"
  ext.gem_spec = spec
end
