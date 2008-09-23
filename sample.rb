require 'rbluez'
include Rbluez


hc = Hci.new
puts hc.hci_remote_version("00:1c:c1:87:35:34")
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
