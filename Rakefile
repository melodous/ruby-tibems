# Rakefile

spec = Gem::Specification.load('tibems.gemspec')

if RUBY_PLATFORM =~ /java/
  require 'rake/javaextensiontask'
  Rake::JavaExtensionTask.new do |ext|
    ext.name      = 'tibems'
    ext.classpath = "vendor/tibjmsadmin.jar"
    ext.gem_spec  = spec 
  end
else
  require 'rake/extensiontask'
  Rake::ExtensionTask.new('tibems', spec)
end

