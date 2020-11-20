/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

//#define _BSD_SOURCE /* for endian.h */
//#define _DEFAULT_SOURCE
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/eventfd.h>

#include "libaio.h"
#define IOCB_FLAG_RESFD         (1 << 0)

#include <linux/usb/functionfs.h>

#define BUF_LEN		8192

#define htole32(x) (x)
#define htole16(x) (x)
#define WCNAME "DeviceInterfaceGUID"
//#define WCDATA "{1D4B2365-4749-48EA-B38A-7C6FDDDD7E26}"

#define WCDATA "{F70242C7-FB25-443B-9E7E-A4260F373982}"
/******************** Descriptors and Strings *******************************/

static const struct {
	struct usb_functionfs_descs_head_v2 header;
	__le32 fs_count;
	__le32 hs_count;
	__le32 ss_count;
	__le32 os_count;
	struct {
		struct usb_interface_descriptor intf;
		struct usb_endpoint_descriptor_no_audio bulk_sink;
		struct usb_endpoint_descriptor_no_audio bulk_source;
	} __attribute__ ((__packed__)) fs_descs, hs_descs;
	struct {
		struct usb_interface_descriptor intf;
		struct usb_endpoint_descriptor_no_audio sink;
		struct usb_ss_ep_comp_descriptor sink_comp;
		struct usb_endpoint_descriptor_no_audio source;
		struct usb_ss_ep_comp_descriptor source_comp;
	} __attribute__ ((__packed__)) ss_descs;
	struct usb_os_desc_header os_header;
	struct usb_ext_compat_desc os_desc;
	struct usb_os_desc_header prop_header;
	struct usb_ext_prop_desc prop_desc;
	const char wcname[sizeof(WCNAME)];
	__le32 wcdata_length;
	const char wcdata[sizeof(WCDATA)];
	
} __attribute__ ((__packed__)) descriptors = {
	.header = {
		.magic = htole32(FUNCTIONFS_DESCRIPTORS_MAGIC_V2),
		.flags = htole32(FUNCTIONFS_HAS_FS_DESC |
				 FUNCTIONFS_HAS_HS_DESC |
				 FUNCTIONFS_HAS_SS_DESC |
				 FUNCTIONFS_HAS_MS_OS_DESC),
		.length = htole32(sizeof(descriptors)),
	},
	.fs_count = htole32(3),
	.fs_descs = {
		.intf = {
			.bLength = sizeof(descriptors.fs_descs.intf),
			.bDescriptorType = USB_DT_INTERFACE,
			.bNumEndpoints = 2,
			.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
			.bInterfaceSubClass = 0xff,
			.iInterface = 0,
		},
		.bulk_sink = {
			.bLength = sizeof(descriptors.fs_descs.bulk_sink),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 1 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
		},
		.bulk_source = {
			.bLength = sizeof(descriptors.fs_descs.bulk_source),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 2 | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
		},
	},
	.hs_count = htole32(3),
	.hs_descs = {
		.intf = {
			.bLength = sizeof(descriptors.hs_descs.intf),
			.bDescriptorType = USB_DT_INTERFACE,
			.bNumEndpoints = 2,
			.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
			.bInterfaceSubClass = 0xff,
			.iInterface = 0,
		},
		.bulk_sink = {
			.bLength = sizeof(descriptors.hs_descs.bulk_sink),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 1 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = htole16(512),
		},
		.bulk_source = {
			.bLength = sizeof(descriptors.hs_descs.bulk_source),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 2 | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = htole16(512),
		},
	},
	.ss_count = htole32(5),
	.ss_descs = {
		.intf = {
			.bLength = sizeof(descriptors.ss_descs.intf),
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
			.bInterfaceSubClass = 0xff,
			.iInterface = 1,
		},
		.sink = {
			.bLength = sizeof(descriptors.ss_descs.sink),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 1 | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = htole16(1024),
		},
		.sink_comp = {
			.bLength = sizeof(descriptors.ss_descs.sink_comp),
			.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
			.bMaxBurst = 4,
		},
		.source = {
			.bLength = sizeof(descriptors.ss_descs.source),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = 2 | USB_DIR_OUT,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize = htole16(1024),
		},
		.source_comp = {
			.bLength = sizeof(descriptors.ss_descs.source_comp),
			.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
			.bMaxBurst = 4,
		},
	},
	.os_count = htole32(2),
	.os_header = {
		.interface = htole32(0),
		//.interface = htole32(1),
		.dwLength = htole32(sizeof(descriptors.os_header) +
			    sizeof(descriptors.os_desc)),
		.bcdVersion = htole32(1),
		.wIndex = htole32(4),
		.bCount = htole32(1),
		.Reserved = htole32(0),
	},
	.os_desc = {
		.bFirstInterfaceNumber = 0,
		.Reserved1 = htole32(1),
		.CompatibleID = {0x57, 0x49, 0x4E, 0x55, 0x53, 0x42, 0x00, 0x00},
		.SubCompatibleID = {0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00},
		.Reserved2 = {0},
	},
	.prop_header = {
		.interface = htole32(0),
		//.interface = htole32(1),
		.dwLength = htole32(83),
		.bcdVersion = htole32(1),
		.wIndex = htole32(5),
		.wCount = htole32(1),
	},
	.prop_desc = {
		.dwSize = htole32(73),
		.dwPropertyDataType = htole32(1),
		.wPropertyNameLength = htole32(sizeof(WCNAME)),
	},
	.wcname = WCNAME,
	.wcdata_length = sizeof(WCDATA),//39
	.wcdata = WCDATA,
};

#define STR_INTERFACE "AIO Test"

static const struct {
	struct usb_functionfs_strings_head header;
	struct {
		__le16 code;
		const char str1[sizeof(STR_INTERFACE)];
	} __attribute__ ((__packed__)) lang0;
} __attribute__ ((__packed__)) strings = {
	.header = {
		.magic = htole32(FUNCTIONFS_STRINGS_MAGIC),
		.length = htole32(sizeof(strings)),
		.str_count = htole32(1),
		.lang_count = htole32(1),
	},
	.lang0 = {
		htole16(0x0409), /* en-us */
		STR_INTERFACE,
	},
};

/******************** Endpoints handling *******************************/

static void display_event(struct usb_functionfs_event *event)
{
	static const char *const names[] = {
		[FUNCTIONFS_BIND] = "BIND",
		[FUNCTIONFS_UNBIND] = "UNBIND",
		[FUNCTIONFS_ENABLE] = "ENABLE",
		[FUNCTIONFS_DISABLE] = "DISABLE",
		[FUNCTIONFS_SETUP] = "SETUP",
		[FUNCTIONFS_SUSPEND] = "SUSPEND",
		[FUNCTIONFS_RESUME] = "RESUME",
	};
	switch (event->type) {
	case FUNCTIONFS_BIND:
	case FUNCTIONFS_UNBIND:
	case FUNCTIONFS_ENABLE:
	case FUNCTIONFS_DISABLE:
	case FUNCTIONFS_SETUP:
	case FUNCTIONFS_SUSPEND:
	case FUNCTIONFS_RESUME:
		printf("Event %s\n", names[event->type]);
	}
}

static void handle_ep0(int ep0, bool *ready)
{
	struct usb_functionfs_event event;
	int ret;

	struct pollfd pfds[1];
	pfds[0].fd = ep0;
	pfds[0].events = POLLIN;

	ret = poll(pfds, 1, 0);

	if (ret && (pfds[0].revents & POLLIN)) {
		ret = read(ep0, &event, sizeof(event));
		if (!ret) {
			perror("unable to read event from ep0");
			return;
		}
		display_event(&event);
		switch (event.type) {
		case FUNCTIONFS_SETUP:
			if (event.u.setup.bRequestType & USB_DIR_IN){
				printf("bRequestType=0x%x,bRequest=0x%x,wValue=0x%x,wIndex=0x%x,wLength=0x%x\n",
				event.u.setup.bRequestType,event.u.setup.bRequest,
				event.u.setup.wValue,event.u.setup.wIndex,event.u.setup.wLength);
				write(ep0, NULL, 0);
			}else
				read(ep0, NULL, 0);
			break;

		case FUNCTIONFS_ENABLE:
			*ready = true;
			break;

		case FUNCTIONFS_DISABLE:
			*ready = false;
			break;

		default:
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int i, ret;
	char *ep_path;

	int ep0;
	int ep[2];

	io_context_t ctx;

	int evfd;
	fd_set rfds;

	char *buf_in, *buf_out;
	struct iocb *iocb_in, *iocb_out;
	int req_in = 0, req_out = 0;
	bool ready;

	if (argc != 2) {
		printf("ffs directory not specified!\n");
		return 1;
	}

	ep_path = malloc(strlen(argv[1]) + 4 /* "/ep#" */ + 1 /* '\0' */);
	if (!ep_path) {
		perror("malloc");
		return 1;
	}

	/* open endpoint files */
	sprintf(ep_path, "%s/ep0", argv[1]);
	ep0 = open(ep_path, O_RDWR);
	if (ep0 < 0) {
		perror("unable to open ep0");
		return 1;
	}
	if (write(ep0, &descriptors, sizeof(descriptors)) < 0) {
		perror("unable do write descriptors");
		return 1;
	}

	if (write(ep0, &strings, sizeof(strings)) < 0) {
		perror("unable to write strings");
		return 1;
	}

	for (i = 0; i < 2; ++i) {
		sprintf(ep_path, "%s/ep%d", argv[1], i+1);
		ep[i] = open(ep_path, O_RDWR);
		if (ep[i] < 0) {
			printf("unable to open ep%d: %s\n", i+1,
			       strerror(errno));
			return 1;
		}
	}

	free(ep_path);

	memset(&ctx, 0, sizeof(ctx));
	/* setup aio context to handle up to 2 requests */
	if (io_setup(2, &ctx) < 0) {
		perror("unable to setup aio");
		return 1;
	}

	evfd = eventfd(0, 0);
	if (evfd < 0) {
		perror("unable to open eventfd");
		return 1;
	}

	/* alloc buffers and requests */
	buf_in = malloc(BUF_LEN);
	buf_out = malloc(BUF_LEN);
	iocb_in = malloc(sizeof(*iocb_in));
	iocb_out = malloc(sizeof(*iocb_out));

	while (1) {
		FD_ZERO(&rfds);
		FD_SET(ep0, &rfds);
		FD_SET(evfd, &rfds);

		ret = select(((ep0 > evfd) ? ep0 : evfd)+1,
			     &rfds, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("select");
			break;
		}

		if (FD_ISSET(ep0, &rfds))
			handle_ep0(ep0, &ready);

		/* we are waiting for function ENABLE */
		if (!ready)
			continue;

		/* if something was submitted we wait for event */
		if (FD_ISSET(evfd, &rfds)) {
			uint64_t ev_cnt;
			ret = read(evfd, &ev_cnt, sizeof(ev_cnt));
			if (ret < 0) {
				perror("unable to read eventfd");
				break;
			}

			struct io_event e[2];
			/* we wait for one event */
			ret = io_getevents(ctx, 1, 2, e, NULL);
			/* if we got event */
			for (i = 0; i < ret; ++i) {
				if (e[i].obj->aio_fildes == ep[0]) {
					printf("ev=in; ret=%lu\n", e[i].res);
					req_in = 0;
				} else if (e[i].obj->aio_fildes == ep[1]) {
					printf("ev=out; ret=%lu\n", e[i].res);
					req_out = 0;
				}
			}
		}

		if (!req_in) { /* if IN transfer not requested*/
			/* prepare write request */
			io_prep_pwrite(iocb_in, ep[0], buf_in, BUF_LEN, 0);
			/* enable eventfd notification */
			iocb_in->u.c.flags |= IOCB_FLAG_RESFD;
			iocb_in->u.c.resfd = evfd;
			/* submit table of requests */
			ret = io_submit(ctx, 1, &iocb_in);
			if (ret >= 0) { /* if ret > 0 request is queued */
				req_in = 1;
				printf("submit: in\n");
			} else
				perror("unable to submit request");
		}
		if (!req_out) { /* if OUT transfer not requested */
			/* prepare read request */
			io_prep_pread(iocb_out, ep[1], buf_out, BUF_LEN, 0);
			/* enable eventfs notification */
			iocb_out->u.c.flags |= IOCB_FLAG_RESFD;
			iocb_out->u.c.resfd = evfd;
			/* submit table of requests */
			ret = io_submit(ctx, 1, &iocb_out);
			if (ret >= 0) { /* if ret > 0 request is queued */
				req_out = 1;
				printf("submit: out\n");
			} else
				perror("unable to submit request");
		}
	}

	/* free resources */

	io_destroy(ctx);

	free(buf_in);
	free(buf_out);
	free(iocb_in);
	free(iocb_out);

	for (i = 0; i < 2; ++i)
		close(ep[i]);
	close(ep0);

	return 0;
}
