require 'rbluez'
include Rbluez

bt = Rfcomm.new
puts bt
bt.rfcomm_bind
bt.rfcomm_listen(29)
client, client_bdaddr = bt.rfcomm_accept
puts "Connessione da #{client_bdaddr}"
puts "Dati ricevuti #{client.rfcomm_recv(20)}"

client.rfcomm_close 	# close client connection
bt.rfcomm_close		# close main socket connection
