require 'rbluez'
include Rbluez

bt = Rfcomm.new
puts bt
bt.rfcomm_bind
bt.rfcomm_listen(29)
puts bt.rfcomm_accept
bt.rfcomm_close
