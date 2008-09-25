/**
 *   Rbluez - Ruby binding for Bluez (official Linux Bluetooth stack)
 *   Copyright (C) 2008  Claudio Fiorini <claudio@cfiorini.it>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

/* Any help and suggestions are always welcome */


#include "ruby.h"
#include "rubyio.h"
#include "rubysig.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>


#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

VALUE rb_mRbluez;
VALUE rb_cHci;
VALUE rb_cHciConn;
VALUE rb_cRfcomm;

void Init_rbluez();

int Rconnect();

typedef struct rzadapter_s {
	int dev_id;
	int sock_id;
	uint16_t handle;
} rzadapter_t;

typedef struct rzconn_s {
	uint16_t handle;
} rzconn_t;

static char *get_minor_device_name(int major, int minor)
{
	switch (major) {
	case 0:	/* misc */
		return "";
	case 1:	/* computer */
		switch(minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Desktop workstation";
		case 2:
			return "Server";
		case 3:
			return "Laptop";
		case 4:
			return "Handheld";
		case 5:
			return "Palm";
		case 6:
			return "Wearable";
		}
		break;
	case 2:	/* phone */
		switch(minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Cellular";
		case 2:
			return "Cordless";
		case 3:
			return "Smart phone";
		case 4:
			return "Wired modem or voice gateway";
		case 5:
			return "Common ISDN Access";
		case 6:
			return "Sim Card Reader";
		}
		break;
	case 3:	/* lan access */
		if (minor == 0)
			return "Uncategorized";
		switch(minor / 8) {
		case 0:
			return "Fully available";
		case 1:
			return "1-17% utilized";
		case 2:
			return "17-33% utilized";
		case 3:
			return "33-50% utilized";
		case 4:
			return "50-67% utilized";
		case 5:
			return "67-83% utilized";
		case 6:
			return "83-99% utilized";
		case 7:
			return "No service available";
		}
		break;
	case 4:	/* audio/video */
		switch(minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Device conforms to the Headset profile";
		case 2:
			return "Hands-free";
			/* 3 is reserved */
		case 4:
			return "Microphone";
		case 5:
			return "Loudspeaker";
		case 6:
			return "Headphones";
		case 7:
			return "Portable Audio";
		case 8:
			return "Car Audio";
		case 9:
			return "Set-top box";
		case 10:
			return "HiFi Audio Device";
		case 11:
			return "VCR";
		case 12:
			return "Video Camera";
		case 13:
			return "Camcorder";
		case 14:
			return "Video Monitor";
		case 15:
			return "Video Display and Loudspeaker";
		case 16:
			return "Video Conferencing";
			/* 17 is reserved */
		case 18:
			return "Gaming/Toy";
		}
		break;
	case 5:	/* peripheral */ {
		static char cls_str[48]; cls_str[0] = 0;

		switch(minor & 48) {
		case 16:
			strncpy(cls_str, "Keyboard", sizeof(cls_str));
			break;
		case 32:
			strncpy(cls_str, "Pointing device", sizeof(cls_str));
			break;
		case 48:
			strncpy(cls_str, "Combo keyboard/pointing device", sizeof(cls_str));
			break;
		}
		if((minor & 15) && (strlen(cls_str) > 0))
			strcat(cls_str, "/");

		switch(minor & 15) {
		case 0:
			break;
		case 1:
			strncat(cls_str, "Joystick", sizeof(cls_str) - strlen(cls_str));
			break;
		case 2:
			strncat(cls_str, "Gamepad", sizeof(cls_str) - strlen(cls_str));
			break;
		case 3:
			strncat(cls_str, "Remote control", sizeof(cls_str) - strlen(cls_str));
			break;
		case 4:
			strncat(cls_str, "Sensing device", sizeof(cls_str) - strlen(cls_str));
			break;
		case 5:
			strncat(cls_str, "Digitizer tablet", sizeof(cls_str) - strlen(cls_str));
		break;
		case 6:
			strncat(cls_str, "Card reader", sizeof(cls_str) - strlen(cls_str));
			break;
		default:
			strncat(cls_str, "(reserved)", sizeof(cls_str) - strlen(cls_str));
			break;
		}
		if(strlen(cls_str) > 0)
			return cls_str;
	}
	case 6:	/* imaging */
		if (minor & 4)
			return "Display";
		if (minor & 8)
			return "Camera";
		if (minor & 16)
			return "Scanner";
		if (minor & 32)
			return "Printer";
		break;
	case 7: /* wearable */
		switch(minor) {
		case 1:
			return "Wrist Watch";
		case 2:
			return "Pager";
		case 3:
			return "Jacket";
		case 4:
			return "Helmet";
		case 5:
			return "Glasses";
		}
		break;
	case 8: /* toy */
		switch(minor) {
		case 1:
			return "Robot";
		case 2:
			return "Vehicle";
		case 3:
			return "Doll / Action Figure";
		case 4:
			return "Controller";
		case 5:
			return "Game";
		}
		break;
	case 63:	/* uncategorised */
		return "";
	}
	return "Unknown (reserved) minor device class";
}

static char *major_classes[] = {
	"Miscellaneous", "Computer", "Phone", "LAN Access",
	"Audio/Video", "Peripheral", "Imaging", "Uncategorized"
};


/* Used from Data_Wrap_Stuct to mark values */
void rzadapter_mark(rzadapter_t* self)
{
	rb_gc_mark(self->dev_id);
	rb_gc_mark(self->sock_id);
	rb_gc_mark(self->handle);
}

/* Used from Data_Wrap_Stuct to free the memory */
void rzadapter_free(rzadapter_t* self)
{
	free(self);
}

void rzconn_mark(rzconn_t* self)
{
	rb_gc_mark(self->handle);
}

void rzconn_free(rzconn_t* self)
{
	free(self);
}


VALUE bz_hci_init(VALUE klass)
{
	rzadapter_t *rza = malloc(sizeof(rzadapter_t));
	
	rza->dev_id = hci_get_route(NULL);
	rza->sock_id = hci_open_dev(rza->dev_id);

	if (rza->dev_id < 0 || rza->sock_id < 0)
		return Qnil;

	return Data_Wrap_Struct(klass, rzadapter_mark, rzadapter_free, rza);
}

VALUE bz_hci_inquiry(VALUE klass)
{
	rzadapter_t* rza;
	inquiry_info *ii;
	int  max_rsp, num_rsp;
	int i, len, flags;
	char addr[19] = { 0 };
	char *dev_class = bt_malloc(32);

	if (!dev_class)
		return Qnil;

	Data_Get_Struct(klass, rzadapter_t, rza);

	len = 8;
	max_rsp = 256;
	flags = IREQ_CACHE_FLUSH;
	ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

	num_rsp = hci_inquiry(rza->dev_id, len, max_rsp, NULL, &ii, flags);
	if(num_rsp < 0)
		return Qnil;
	
	VALUE remote_devices = rb_ary_new2(num_rsp);
	for(i = 0; i < num_rsp; i++) {
		ba2str(&(ii+i)->bdaddr, addr);
		VALUE dev_remote = rb_hash_new();

		rb_hash_aset(dev_remote, rb_str_new2("bdaddr"), rb_str_new2(addr));

		sprintf(dev_class, "%s, %s", major_classes[(ii+i)->dev_class[1] & 0x1f],
			get_minor_device_name((ii+i)->dev_class[1] & 0x1f, (ii+i)->dev_class[0] >> 2));

	/*
	*	sprintf(dev_class, "0x%2.2x%2.2x%2.2x", (ii+i)->dev_class[2],
	*				 (ii+i)->dev_class[1], (ii+i)->dev_class[0]);
	*/
		rb_hash_aset(dev_remote, rb_str_new2("dev_class"), rb_str_new2(dev_class));

		rb_ary_push(remote_devices, dev_remote);

	}
	return remote_devices;
}

VALUE bz_hci_local_name(VALUE klass)
{
	rzadapter_t* rza;
	char name[32] = { 0 };

	Data_Get_Struct(klass, rzadapter_t, rza);

	memset(name, 0, sizeof(name));
	if(hci_read_local_name(rza->sock_id, 8, name, 0) < 0)
		return Qnil;

	return rb_str_new2(name);
}

VALUE bz_hci_local_bdaddr(VALUE klass)
{
	rzadapter_t* rza;
	bdaddr_t bdaddr;
	char addr[19] = { 0 };

	Data_Get_Struct(klass, rzadapter_t, rza);

	if(hci_devba(rza->dev_id, &bdaddr) < 0)
		return Qnil;

	ba2str(&bdaddr, addr);
	return rb_str_new2(addr);
}

VALUE bz_hci_local_cod(VALUE klass)
{
	rzadapter_t* rza;
	char *dev_class = bt_malloc(32);
	uint8_t cls[3];	

	if (!dev_class)
		return Qnil;

	Data_Get_Struct(klass, rzadapter_t, rza);
	if(hci_read_class_of_dev(rza->sock_id, cls, 0) < 0)
		return Qnil;

	sprintf(dev_class, "%s, %s", major_classes[cls[1] & 0x1f],
			get_minor_device_name(cls[1] & 0x1f, cls[0] >> 2));

	return rb_str_new2(dev_class); 
}

VALUE bz_hci_write_local_cod(VALUE klass, VALUE cod)
{
	rzadapter_t* rza;
	uint32_t cls;

	if(TYPE(cod) == T_STRING && TYPE(cod) != T_NIL) {
		if(RSTRING(cod)->len == 6) {
			/*
 			* THIS METHOD NEEDS MORE CONTROLS
 			*/
			cls = strtoul(RSTRING(cod)->ptr, NULL, 16);

			Data_Get_Struct(klass, rzadapter_t, rza);

			if(hci_write_class_of_dev(rza->sock_id, cls, 2000) < 0)
				return Qnil;

			return Qtrue;
		} else {
			rb_raise(rb_eArgError, "value length must be 6");
			return Qnil;
		}
	} else {
		rb_raise(rb_eTypeError, "not valid value");
		return Qnil;
	}
}

VALUE bz_hci_remote_name(VALUE klass, VALUE str)
{
	rzadapter_t* rza;
	char name[32] = { 0 };
	bdaddr_t addr;

	if(TYPE(str) == T_STRING && TYPE(str) != T_NIL) {
		Data_Get_Struct(klass, rzadapter_t, rza);

		str2ba(RSTRING(str)->ptr, &addr);
		memset(name, 0, sizeof(name));
		if(hci_read_remote_name(rza->sock_id, &addr, sizeof(name), name, 0) < 0)
			return Qnil;
		
		return rb_str_new2(name);
	} else {
		rb_raise(rb_eTypeError, "not valid value");
		return Qnil;
	}
}

VALUE bz_hci_write_local_name(VALUE klass, VALUE str)
{
	rzadapter_t* rza;
	
	if(TYPE(str) == T_STRING && TYPE(str) != T_NIL) {
		if(RSTRING(str)->len < 18) {
			Data_Get_Struct(klass, rzadapter_t, rza);
			if(hci_write_local_name(rza->sock_id, RSTRING(str)->ptr, 0) < 0)
				return Qnil;
			
			return rb_str_new2(RSTRING(str)->ptr);
		} else {
			rb_raise(rb_eArgError, "string length must be > 0 < 16");
			return Qnil; 
		}
	} else {
		rb_raise(rb_eTypeError, "not valid value");
		return Qnil;
	}
}

VALUE bz_hci_connect(VALUE klass, VALUE rb_bdaddr)
{
	bdaddr_t bd_addr;

	rzadapter_t* rza;
	struct hci_dev_info di;

	str2ba(RSTRING(rb_bdaddr)->ptr, &bd_addr);
	Data_Get_Struct(klass, rzadapter_t, rza);
	
	if(hci_devinfo(rza->dev_id, &di) < 0) {
		return Qnil;
	}

	if (hci_create_connection(rza->sock_id, &bd_addr,
				htobs(di.pkt_type & ACL_PTYPE_MASK),
				0, 0x01, &rza->handle, 25000) < 0) {
		close(rza->sock_id);
		return Qnil;
	}

	return INT2NUM(0);
}

VALUE bz_hci_remote_version(VALUE klass)
{
	char *version_str = bt_malloc(1024);

	rzadapter_t* rza;
	struct hci_version version;
	
	Data_Get_Struct(klass, rzadapter_t, rza);
	
	if (hci_read_remote_version(rza->sock_id, rza->handle, &version, 20000) == 0) {
		char *ver = lmp_vertostr(version.lmp_ver);
		sprintf(version_str, "\tLMP Version: %s (0x%x) LMP Subversion: 0x%x\n"
			"\tManufacturer: %s (%d)\n",
			ver ? ver : "n/a",
			version.lmp_ver,
			version.lmp_subver,
			bt_compidtostr(version.manufacturer),
			version.manufacturer);
		if (ver)
			bt_free(ver);
	}
	

	return rb_str_new2(version_str);
}

VALUE bz_hci_link_quality(VALUE klass)
{
	uint8_t lq;
	rzadapter_t* rza;

	Data_Get_Struct(klass, rzadapter_t, rza);

	if(hci_read_link_quality(rza->sock_id, rza->handle, &lq, 1000) < 0) {
		rb_raise(rb_eException, "hci_read_link_quality error");
		return Qnil;
	}

	return INT2FIX(lq);
}

VALUE bz_hci_auth_link(VALUE klass)
{
	rzadapter_t* rza;

	Data_Get_Struct(klass, rzadapter_t, rza);

	if(hci_authenticate_link(rza->sock_id, rza->handle, 25000) < 0) {
		rb_raise(rb_eException, "hci_authenticate_link error");
		return Qnil;
	}

	return INT2FIX(0);
}

VALUE bz_hci_read_tpl(VALUE klass, VALUE rbtype)
{
	rzadapter_t* rza;
	uint8_t type;
	int8_t level;

	if(TYPE(rbtype) != T_FIXNUM) {
		rb_raise(rb_eTypeError, "not valid value");
	}

	if(TYPE(rbtype) == T_NIL) {
		type = 0;
	} else {
		type = FIX2INT(rbtype);
	}
/*
	if(type != 0 || type != 1) {
		rb_raise(rb_eArgError, "type must be 0 or 1");
	}
*/
	Data_Get_Struct(klass, rzadapter_t, rza);

	if (hci_read_transmit_power_level(rza->sock_id, rza->handle, type, &level, 1000) < 0) {
		rb_raise(rb_eException, "hci_read_transmit_power_level error");
		return Qnil;
	}

	return INT2FIX(level);
}

VALUE bz_hci_disconnect(VALUE klass)
{
	rzadapter_t* rza;
	
	Data_Get_Struct(klass, rzadapter_t, rza);

	if(hci_disconnect(rza->sock_id, rza->handle, HCI_OE_USER_ENDED_CONNECTION, 10000) < 0) {
		rb_raise(rb_eScriptError, "Error on closing");
		return Qnil;
	}
	return INT2NUM(0);
}

VALUE bz_hci_close(VALUE klass)
{
	rzadapter_t* rza;

	Data_Get_Struct(klass, rzadapter_t, rza);

	return INT2FIX(close(rza->sock_id));
}


/* SOCKET FUNCTIONS */

static int ruby_socket(int domain, int type, int proto)
{
    int fd;

    fd = socket(domain, type, proto);
    if (fd < 0) {
	if (errno == EMFILE || errno == ENFILE) {
	    rb_gc();
	    fd = socket(domain, type, proto);
	}
    }
    return fd;
}

static VALUE init_sock(VALUE sock, int fd)
{
    OpenFile *fp;

    MakeOpenFile(sock, fp);
    fp->f = rb_fdopen(fd, "r");
    fp->f2 = rb_fdopen(fd, "w");
    fp->mode = FMODE_READWRITE;
    rb_io_synchronized(fp);

    return sock;
}

static int wait_connectable(int fd)
{
	int sockerr;
	socklen_t sockerrlen;
	fd_set fds_w;
	fd_set fds_e;

	for (;;) {
		FD_ZERO(&fds_w);
		FD_ZERO(&fds_e);

		FD_SET(fd, &fds_w);
		FD_SET(fd, &fds_e);

		rb_thread_select(fd+1, 0, &fds_w, &fds_e, 0);

		if (FD_ISSET(fd, &fds_w)) {
	    		return 0;
		}
		else if (FD_ISSET(fd, &fds_e)) {
	    		sockerrlen = sizeof(sockerr);
	    		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&sockerr,
			   	&sockerrlen) == 0) {

				if (sockerr == 0)
		    			continue;	/* workaround for winsock */
				errno = sockerr;
	    		}
	    		return -1;
		}
    	}
	return 0;
}

/* RFCOMM FUNCTIONS */
static VALUE bz_rfcomm_init(VALUE sock)
{
    	int fd;

    	rb_secure(3);
    	fd = ruby_socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    	if (fd < 0) rb_sys_fail("socket(2)");

	return init_sock(sock, fd);
}


static VALUE bz_rfcomm_bind(VALUE sock)
{
	OpenFile *fptr;
	struct sockaddr_rc loc_addr = {};

	loc_addr.rc_family = AF_BLUETOOTH;
	loc_addr.rc_bdaddr = *BDADDR_ANY;
	loc_addr.rc_channel = (uint8_t) 1;

	GetOpenFile(sock, fptr);
	if (bind(fileno(fptr->f), (struct sockaddr*)&loc_addr, sizeof(loc_addr)) < 0)
		rb_sys_fail("bind(2)");

	return INT2FIX(0);
}


static VALUE bz_rfcomm_listen(VALUE sock, VALUE log)
{
	OpenFile *fptr;

	rb_secure(4);
	GetOpenFile(sock, fptr);
	if (listen(fileno(fptr->f), NUM2INT(log)) < 0)
		rb_sys_fail("listen(2)");

	return INT2FIX(0);
}


static VALUE s_accept(klass, fd, sockaddr, len)
    VALUE klass;
    int fd;
    struct sockaddr *sockaddr;
    socklen_t len;
{
    int fd2;
    int retry = 0;

    rb_secure(3);
  retry:
    rb_thread_wait_fd(fd);
#if defined(_nec_ews)
    fd2 = accept(fd, sockaddr, len);
#else
    TRAP_BEG;
    fd2 = accept(fd, sockaddr, &len);
    TRAP_END;
#endif
    if (fd2 < 0) {
	switch (errno) {
	  case EMFILE:
	  case ENFILE:
	    if (retry) break;
	    rb_gc();
	    retry = 1;
	    goto retry;
	  case EWOULDBLOCK:
	    break;
	  default:
	    if (!rb_io_wait_readable(fd)) break;
	    retry = 0;
	    goto retry;
	}
	rb_sys_fail(0);
    }
    if (!klass) return INT2NUM(fd2);
    return init_sock(rb_obj_alloc(klass), fd2);
}

static VALUE bz_rfcomm_accept(VALUE sock)
{
	OpenFile *fptr;
	VALUE sock2;
	struct sockaddr_rc rem_addr;
	char addr[19] = { 0 };

	GetOpenFile(sock, fptr);
	sock2 = s_accept(rb_cRfcomm,fileno(fptr->f), (struct sockaddr *)&rem_addr, sizeof(rem_addr));
	ba2str(&rem_addr.rc_bdaddr, addr);
	return rb_assoc_new(sock2, rb_str_new2(addr));
}


static VALUE s_recvfrom(VALUE sock, int argc, VALUE *argv)
{
    OpenFile *fptr;
    VALUE str;
    char buf[1024];
    socklen_t alen = sizeof buf;
    VALUE len, flg;
    long buflen;
    long slen;
    int fd, flags;

    rb_scan_args(argc, argv, "11", &len, &flg);

    if (flg == Qnil) flags = 0;
    else             flags = NUM2INT(flg);
    buflen = NUM2INT(len);

    GetOpenFile(sock, fptr);
    if (rb_read_pending(fptr->f)) {
	rb_raise(rb_eIOError, "recv for buffered IO");
    }
    fd = fileno(fptr->f);

    str = rb_tainted_str_new(0, buflen);

  retry:
    rb_str_locktmp(str);
    rb_thread_wait_fd(fd);
    TRAP_BEG;
    slen = recvfrom(fd, RSTRING(str)->ptr, buflen, flags, (struct sockaddr*)buf, &alen);
    TRAP_END;
    rb_str_unlocktmp(str);

    if (slen < 0) {
	if (rb_io_wait_readable(fd)) {
	    goto retry;
	}
	rb_sys_fail("recvfrom(2)");
    }
    if (slen < RSTRING(str)->len) {
	RSTRING(str)->len = slen;
	RSTRING(str)->ptr[slen] = '\0';
    }
    rb_obj_taint(str);
	return (VALUE)str;
}

static VALUE bz_rfcomm_recv(VALUE argc, VALUE *argv, VALUE sock)
{
	return s_recvfrom(sock, argc, argv);
}


static VALUE bz_rfcomm_send(VALUE argc, VALUE *argv, VALUE sock)
{
	VALUE mesg, to;
	VALUE flags;
	OpenFile *fptr;
	FILE *f;
	int fd, n;

	rb_secure(4);
	rb_scan_args(argc, argv, "21", &mesg, &flags, &to);

	StringValue(mesg);
	if (!NIL_P(to)) StringValue(to);
	GetOpenFile(sock, fptr);
	f = GetWriteFile(fptr);
	fd = fileno(f);
	rb_thread_fd_writable(fd);

    retry:

        TRAP_BEG;
	n = send(fd, RSTRING(mesg)->ptr, RSTRING(mesg)->len, NUM2INT(flags));
        TRAP_END;

	if (n < 0) {
		if (rb_io_wait_writable(fd)) {
			goto retry;
		}
		rb_sys_fail("send(2)");
	}
	return INT2FIX(n);
}


static int
ruby_connect(fd, sockaddr, len, socks)
    int fd;
    struct sockaddr *sockaddr;
    int len;
    int socks;
{
    int status;
    int mode;
#if WAIT_IN_PROGRESS > 0
    int wait_in_progress = -1;
    int sockerr;
    socklen_t sockerrlen;
#endif

#if defined(HAVE_FCNTL)
# if defined(F_GETFL)
    mode = fcntl(fd, F_GETFL, 0);
# else
    mode = 0;
# endif
# endif

#ifdef O_NDELAY
# define NONBLOCKING O_NDELAY
#else
#ifdef O_NBIO
# define NONBLOCKING O_NBIO
#else
# define NONBLOCKING O_NONBLOCK
#endif
#endif
    fcntl(fd, F_SETFL, mode|NONBLOCKING);

    for (;;) {
	if (socks) {
	    status = Rconnect(fd, sockaddr, len);
	}
	else
	{
	    status = connect(fd, sockaddr, len);
	}
	if (status < 0) {
	    switch (errno) {
	      case EAGAIN:
#ifdef EINPROGRESS
	      case EINPROGRESS:
#endif
#if WAIT_IN_PROGRESS > 0
		sockerrlen = sizeof(sockerr);
		status = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&sockerr, &sockerrlen);
		if (status) break;
		if (sockerr) {
		    status = -1;
		    errno = sockerr;
		    break;
		}
#endif
#ifdef EALREADY
	      case EALREADY:
#endif
#if WAIT_IN_PROGRESS > 0
		wait_in_progress = WAIT_IN_PROGRESS;
#endif
		status = wait_connectable(fd);
		if (status) {
		    break;
		}
		errno = 0;
		continue;

#if WAIT_IN_PROGRESS > 0
	      case EINVAL:
		if (wait_in_progress-- > 0) {
		    /*
		     * connect() after EINPROGRESS returns EINVAL on
		     * some platforms, need to check true error
		     * status.
		     */
		    sockerrlen = sizeof(sockerr);
		    status = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&sockerr, &sockerrlen);
		    if (!status && !sockerr) {
			struct timeval tv = {0, 100000};
			rb_thread_wait_for(tv);
			continue;
		    }
		    status = -1;
		    errno = sockerr;
		}
		break;
#endif

#ifdef EISCONN
	      case EISCONN:
		status = 0;
		errno = 0;
		break;
#endif
	      default:
		break;
	    }
	}
#ifdef HAVE_FCNTL
	fcntl(fd, F_SETFL, mode);
#endif
	return status;
    }
}

static VALUE bz_rfcomm_connect(VALUE sock, VALUE addr)
{
	OpenFile *fptr;
	int fd;
	struct sockaddr_rc rem_addr = { 0 };	

	if(TYPE(addr) != T_STRING) {
		rb_raise(rb_eTypeError, "not valid value: string request");
		return Qnil;
	}	

	rem_addr.rc_family = AF_BLUETOOTH;
	rem_addr.rc_channel = (uint8_t) 1;
	str2ba(RSTRING(addr)->ptr, &rem_addr.rc_bdaddr); 

	GetOpenFile(sock, fptr);
	fd = fileno(fptr->f);
	if (ruby_connect(fd, (struct sockaddr*)&rem_addr, sizeof(rem_addr), 0) < 0) {
		rb_sys_fail("connect(2)");
	}

	return INT2FIX(0);
}

static VALUE bz_rfcomm_close(VALUE sock)
{
	OpenFile *fptr;

	if (rb_safe_level() >= 4 && !OBJ_TAINTED(sock)) {
		rb_raise(rb_eSecurityError, "Insecure: can't close socket");
	}

	GetOpenFile(sock, fptr);

	/* '2' = sends and receives are disallowed (like close()) */
	shutdown(fileno(fptr->f), 2); 
	shutdown(fileno(fptr->f2), 2);
	return rb_io_close(sock);
}

void Init_rbluez()
{
	rb_mRbluez = rb_define_module("Rbluez");

	rb_cHci = rb_define_class_under(rb_mRbluez, "Hci", rb_cObject);
	rb_define_singleton_method(rb_cHci, "new", bz_hci_init, 0);	
	rb_define_method(rb_cHci, "hci_scan", bz_hci_inquiry, 0);
	rb_define_method(rb_cHci, "hci_local_name", bz_hci_local_name, 0);
	rb_define_method(rb_cHci, "hci_local_bdaddr", bz_hci_local_bdaddr, 0);
	rb_define_method(rb_cHci, "hci_local_cod", bz_hci_local_cod, 0);
	rb_define_method(rb_cHci, "hci_set_local_name", bz_hci_write_local_name, 1);
	rb_define_method(rb_cHci, "hci_set_local_cod", bz_hci_write_local_cod, 1);
	rb_define_method(rb_cHci, "hci_remote_name", bz_hci_remote_name, 1);
	rb_define_method(rb_cHci, "hci_remote_version", bz_hci_remote_version, 0);
	rb_define_method(rb_cHci, "hci_close", bz_hci_close, 0);
	rb_define_method(rb_cHci, "hci_connect", bz_hci_connect, 1);
	rb_define_method(rb_cHci, "hci_disconnect", bz_hci_disconnect, 0);
	rb_define_method(rb_cHci, "hci_lq", bz_hci_link_quality, 0);
	rb_define_method(rb_cHci, "hci_auth", bz_hci_auth_link, 0);
	rb_define_method(rb_cHci, "hci_read_tpl", bz_hci_read_tpl, 1);

	rb_cRfcomm = rb_define_class_under(rb_mRbluez, "Rfcomm", rb_cIO);
	rb_define_method(rb_cRfcomm, "initialize", bz_rfcomm_init, 0); 
	rb_define_method(rb_cRfcomm, "rfcomm_bind", bz_rfcomm_bind, 0);
	rb_define_method(rb_cRfcomm, "rfcomm_connect", bz_rfcomm_connect, 1);
	rb_define_method(rb_cRfcomm, "rfcomm_listen", bz_rfcomm_listen, 1);
	rb_define_method(rb_cRfcomm, "rfcomm_accept", bz_rfcomm_accept, 0);
	rb_define_method(rb_cRfcomm, "rfcomm_recv", bz_rfcomm_recv, -1);
	rb_define_method(rb_cRfcomm, "rfcomm_send", bz_rfcomm_send, -1);
	rb_define_method(rb_cRfcomm, "rfcomm_close", bz_rfcomm_close, 0);

	/* Rbluez module defines */
	rb_define_const(rb_mRbluez, "AF_BLUETOOTH", INT2FIX(AF_BLUETOOTH));
	rb_define_const(rb_mRbluez, "PF_BLUETOOTH", INT2FIX(AF_BLUETOOTH));
	rb_define_const(rb_mRbluez, "BTPROTO_L2CAP", INT2FIX(BTPROTO_L2CAP));
	rb_define_const(rb_mRbluez, "BTPROTO_RFCOMM", INT2FIX(BTPROTO_RFCOMM));
	rb_define_const(rb_mRbluez, "BTPROTO_HCI", INT2FIX(BTPROTO_HCI));

	/* Class of Device */
	rb_define_const(rb_mRbluez, "COD_MISC", rb_str_new2("0x0000"));
	rb_define_const(rb_mRbluez, "COD_COMPUTER_UNCATEGORIZED", rb_str_new2("0x0100"));
	rb_define_const(rb_mRbluez, "COD_COMPUTER_DESKTOP", rb_str_new2("0x0104"));
	rb_define_const(rb_mRbluez, "COD_COMPUTER_SERVER", rb_str_new2("0x0108"));
	rb_define_const(rb_mRbluez, "COD_COMPUTER_LAPTOP", rb_str_new2("0x010c"));
	rb_define_const(rb_mRbluez, "COD_COMPUTER_HANDHELD", rb_str_new2("0x0110"));
	rb_define_const(rb_mRbluez, "COD_COMPUTER_PALM", rb_str_new2("0x0114"));
	rb_define_const(rb_mRbluez, "COD_COMPUTER_WEARABLE", rb_str_new2("0x0118"));
	rb_define_const(rb_mRbluez, "COD_PHONE_UNCATEGORIZED", rb_str_new2("0x0200"));
	rb_define_const(rb_mRbluez, "COD_PHONE_CELLULAR", rb_str_new2("0x0204"));
	rb_define_const(rb_mRbluez, "COD_PHONE_CORDLESS", rb_str_new2("0x0208"));
	rb_define_const(rb_mRbluez, "COD_PHONE_SMARTPHONE", rb_str_new2("0x020c"));
	rb_define_const(rb_mRbluez, "COD_PHONE_WIREDMODEM", rb_str_new2("0x0210"));
	rb_define_const(rb_mRbluez, "COD_PHONE_ISDNACCESS", rb_str_new2("0x0214"));
	rb_define_const(rb_mRbluez, "COD_PHONE_SIMREADER", rb_str_new2("0x0218"));
}
