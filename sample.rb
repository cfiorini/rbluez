require 'rbluez'
include Rbluez

=begin
hc = Hci.new
hc.hci_connect("00:1c:c1:87:35:34")
sleep 5
puts hc.hci_lq
puts hc.hci_auth
puts hc.hci_read_tpl(1)
puts hc.hci_remote_version
sleep 5
hc.hci_disconnect
hc.hci_close
=end
bt = Rfcomm.new
#bt.rfcomm_bind
#bt.rfcomm_listen(29)
#puts "Socket listening..."
#client, client_bdaddr = bt.rfcomm_accept
#puts "Connessione da #{client_bdaddr}"
#puts "Dati ricevuti #{client.rfcomm_recv(20)}"
bt.rfcomm_connect("00:11:67:8e:49:a1")
bt.rfcomm_send("ciao da claudio aaaaaaaaaaaa", 0)

#client.rfcomm_close 	# close client connection
bt.rfcomm_close		# close main socket connection
