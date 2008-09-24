require 'rbluez'
include Rbluez


hc = Hci.new
hc.hci_connect("00:1c:c1:87:35:34")
sleep 10
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
