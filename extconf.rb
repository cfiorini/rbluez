require 'mkmf'

extension_name = 'rbluez'

dir_config(extension_name)
have_library("bluetooth")
create_makefile(extension_name)
