require 'rbluez'

bt = Rbluez.new
for r in bt.rz_scan
	p r
end
bt.rz_close
