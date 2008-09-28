require 'rbluez'
include Rbluez


puts "first example"

bt = Hci.new
bt.hci_set_local_name("My Phone")
bt.hci_set_local_cod(COD_COMPUTER_LAPTOP)
puts bt.hc.hci_local_name
puts bt.hci_local_bdaddr
puts bt.hci_local_cod
for r in bt.hci_scan
	puts "Remote device: #{r[:bdaddr]}"
	puts "Remote device of class: #{r[:dev_class]}"
	puts "Remote name #{bt.hci_remote_name(r[:bdaddr]}"
end
bt.hci_close

puts "second example"

bt = Hci.new
bt.hci_connect("00:11:22:33:44:55")
sleep 5
puts bt.hci_lq
puts bt.hci_auth
puts bt.hci_read_tpl(1)
puts bt.hci_remote_version
bt.hci_disconnect
bt.hci_close


=begin
	
 this is a third example but you need two bluetooth host because
 one is the client and one is the server

#Server bluetooth
bt = Rfcomm.new
bt.rfcomm_bind
bt.rfcomm_listen(29)
puts "Socket listening..."
client, client_bdaddr = bt.rfcomm_accept
puts "Connessione da #{client_bdaddr}"
puts "Dati ricevuti #{client.rfcomm_recv(20)}"
client.rfcomm_close 	# close client connection
bt.rfcomm_close


#Client bluetooth
bt.Rfcomm.new
bt.rfcomm_connect("00:11:22:33:44:55")
bt.rfcomm_send("Hello world!", 0)
bt.rfcomm_close

=end
