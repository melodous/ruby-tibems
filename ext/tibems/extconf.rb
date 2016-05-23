# encoding: UTF-8
require 'mkmf'
require 'English'

# 2.0-only
have_header('ruby/thread.h') && have_func('rb_thread_call_without_gvl', 'ruby/thread.h')

# 1.9-only
have_func('rb_thread_blocking_region')
have_func('rb_hash_dup')
have_func('rb_intern3')

# If the user has provided a --with-tibems-dir argument, we must respect it or fail.
inc, lib = dir_config('tibems')
if inc && lib
  # TODO: Remove when 2.0.0 is the minimum supported version
  # Ruby versions not incorporating the mkmf fix at
  # https://bugs.ruby-lang.org/projects/ruby-trunk/repository/revisions/39717
  # do not properly search for lib directories, and must be corrected
  unless lib && lib[-3, 3] == 'lib'
    @libdir_basename = 'lib'
    inc, lib = dir_config('tibems')
  end

  if RUBY_PLATFORM =~ /x86_64/
    # Add dir with /64 in libraries
    libs64 = lib.split(File::PATH_SEPARATOR).map! { |l| "#{l}/64" }
    lib = libs64.join(File::PATH_SEPARATOR) + File::PATH_SEPARATOR + lib
  end

  abort "-----\nCannot find include dir(s) #{inc}\n-----" unless inc && inc.split(File::PATH_SEPARATOR).any? { |dir| File.directory?(dir) }
  abort "-----\nCannot find library dir(s) #{lib}\n-----" unless lib && lib.split(File::PATH_SEPARATOR).any? { |dir| File.directory?(dir) }
  warn "-----\nUsing --with-tibems-dir=#{File.dirname inc}\n-----"
  rpath_dir = lib
end

if have_header('tibems.h')
  prefix = nil
elsif have_header('tibems/tibems.h')
  prefix = 'tibems'
else
  asplode 'tibems.h'
end

%w(emsadmin.h emserr.h admtypes.h qinfo.h status.h).each do |h|
  header = [prefix, h].compact.join '/'
  asplode h unless have_header header
end

if RUBY_PLATFORM =~ /x86_64/
  have_library("tibems64");
  have_library("tibemslookup64");
  have_library("tibemsufo64");
  have_library("tibemsadmin64");
else
  have_library("tibems");
  have_library("tibemslookup");
  have_library("tibemsufo");
  have_library("tibemsadmin");
end

have_library("ssl");

create_makefile('tibems')
