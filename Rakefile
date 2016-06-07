# Rakefile

spec = Gem::Specification.load('tibems.gemspec')

if RUBY_PLATFORM =~ /java/
  require 'rake/javaextensiontask'
  Rake::JavaExtensionTask.new('tibems', spec)
else
  require 'rake/extensiontask'
  Rake::ExtensionTask.new('tibems', spec)
end

