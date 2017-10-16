# Rakefile

spec = Gem::Specification.load('tibems.gemspec')

if RUBY_PLATFORM =~ /java/
  require 'jars/classpath'
  require 'rake/javaextensiontask'
  require 'jars/installer'
  require 'rubygems/package_task'

  task :install_jars do
    Jars::JarInstaller.new.vendor_jars
    #Jars::JarInstaller.new.install_jars
  end

  Rake::JavaExtensionTask.new('tibems', spec) do |ext|
    ext.classpath      = Jars::Classpath.new.classpath_string
    ext.lib_dir        = File.join('lib', 'tibems')
    ext.ext_dir        = "ext"
    ext.source_version = "1.7" 
    ext.target_version = "1.7" 
  end

  Gem::PackageTask.new( spec ) do
    desc 'Pack gem'
    task :package => [:install_jars, :compile]
  end
else
  require 'rake/extensiontask'
  Rake::ExtensionTask.new('tibems', spec) do |ext|
    ext.lib_dir    = File.join('lib', 'tibems')
  end
end

