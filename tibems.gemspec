# tibems.gemspec

Gem::Specification.new do |s|
  s.name    = "tibems-websocket"
  s.version = "0.0.1"
  s.summary = "TibEMS bindings for Ruby"
  s.author  = ["Justo Alonso","justo.alonso@gmail.com"]

  files = Dir.glob("ext/**/*.{c,java,rb}") +
          Dir.glob("lib/**/*.rb")

  if RUBY_PLATFORM =~ /java/
    s.platform = "java"
    files << "lib/tibems.jar"
  else
    s.extensions << "ext/tibems/extconf.rb"
  end

  s.files = files

  s.add_development_dependency "rake-compiler"
end

