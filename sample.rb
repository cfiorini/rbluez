require 'rbluez'
bt = Rbluez.new
#for r in bt.rz_scan
#	p r 
#end
puts bt.rfcomm_socket
#s.rfcomm_bind(1)
#s.rfcomm_listen(1)
#s.rfcomm_accept
#puts s.rfcomm_read
bt.rz_close
