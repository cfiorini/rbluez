require 'rbluez'
include Rbluez


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

#bt = Rfcomm.new
#bt.rfcomm_bind
#bt.rfcomm_listen(29)
#puts "Socket listening..."
#client, client_bdaddr = bt.rfcomm_accept
#puts "Connessione da #{client_bdaddr}"
#puts "Dati ricevuti #{client.rfcomm_recv(20)}"

#client.rfcomm_close 	# close client connection
#bt.rfcomm_close		# close main socket connection
