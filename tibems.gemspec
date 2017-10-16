# tibems.gemspec

Gem::Specification.new do |s|
  s.name        = "tibems"
  s.version     = "0.0.7"
  s.summary     = "TibEMS bindings for Ruby"
  s.description = "TibEMS bindings for Ruby. Ruby interface for the tibjmsadmin.jar library"
  s.authors     = ["Justo Alonso"]
  s.email       = ["justo.alonso@gmail.com"]
  s.homepage    = "https://github.com/jalonsoa/ruby-tibems"
  s.licenses =    [ "Apache-2.0" ]

  files = Dir.glob("lib/**/*.rb") +
          Dir.glob("lib/tibems.rb")

  if RUBY_PLATFORM =~ /java/
    s.platform = "java"
    files += Dir.glob("ext/**/*.{java,rb}")
    files << "lib/tibems/tibems.jar"

    s.add_runtime_dependency 'jar-dependencies', "~>0.2"

    s.requirements << "jar 'javax.jms:javax.jms-api', '2.0'"
    s.requirements << "jar 'com.tibco:tibjms', '~>7.0'"
    s.requirements << "jar 'com.tibco:tibjms-admin', '~>7.0'"

    s.add_development_dependency 'ruby-maven', '~> 3.3', '>= 3.3.8'
  else
    files += Dir.glob("ext/**/*.{c,h,rb}")
    s.extensions << "ext/tibems/extconf.rb"
  end

  s.files = files

  s.add_development_dependency "rake-compiler", "~>0.9"
end

