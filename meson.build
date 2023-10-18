project('sbus-relay', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m')

if not cc.has_function('asprintf', prefix: '#define _GNU_SOURCE\n#include <stdio.h>')
    error('need function "asprintf"')
endif

server = executable('sbus_relay', 'sbus_relay.c', dependencies: [m_dep], install : true)
