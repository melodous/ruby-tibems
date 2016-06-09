# Rakefile

spec = Gem::Specification.load('tibems.gemspec')

if RUBY_PLATFORM =~ /java/
  require 'rake/javaextensiontask'
  Rake::JavaExtensionTask.new('tibems', spec) do |ext|
    ext.classpath  = "vendor/tibjmsadmin.jar"
    ext.lib_dir    = File.join('lib', 'tibems')
    ext.source_version = "1.7" 
    ext.target_version = "1.7" 
  end
else
  require 'rake/extensiontask'
  Rake::ExtensionTask.new('tibems', spec) do |ext|
    ext.lib_dir    = File.join('lib', 'tibems')
  end
end

