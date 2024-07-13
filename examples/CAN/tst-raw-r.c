/*
 *  $Id$
 */

/*
 * tst-raw.c
 *
 * Copyright (c) 2002-2007 Volkswagen Group Electronic Research
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Volkswagen nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * Send feedback to <linux-can@vger.kernel.org>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h> 
#include <errno.h>  // errno is a global variable of c function.

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#define _kbhit kbhit
#define _getch getchar

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if(ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int result;
    int hsocket;
    struct sockaddr_can addr;
    struct can_filter rfilter[4];
    struct can_frame frame;
    int nbytes, i;
    struct ifreq ifr;
    char *ifname = "vcan2";
    int ifindex;
    int opt;
    int flag;
    struct timeval tv;
    
    hsocket = -1;
    /* sockopt test */
    int defaultvalue = 0;
    int loopback = 0;
    //int set_loopback = 0;
    int recv_own_msgs = 0;
    //int set_recv_own_msgs = 0;
    int send_one_frame = 0;
    int ignore_errors = 0;
    int show_frame = 0;
    int enable_block = 0;
    int enable_delay = 0;

    unsigned int can_sid = 0;
    unsigned int can_eid = 0;
    int send_stop_frame = 0;
    int so_rcvbuf = 0;
    int so_sndbuf = 0;
    socklen_t socklen;
    int bstop = 0;
    
    fd_set wfds;
	int ch;

    while ((opt = getopt(argc, argv, "i:lrsembd")) != -1) {
        switch (opt) {

        case 'i':
            ifname = optarg;
            break;

        case 'l':
            //loopback = atoi(optarg);
            //set_loopback = 1;
            loopback = 1;
            break;

        case 'r':
            //recv_own_msgs = atoi(optarg);
            //set_recv_own_msgs = 1;
            recv_own_msgs = 1;
            break;

        case 's':
            send_one_frame = 1;
            break;

        case 'e':
            ignore_errors = 1;
            break;

        case 'm':
            show_frame = 1;
            break;
        
        case 'b':
            enable_block = 1;
            break;
        
        case 'd':
            enable_delay = 1;
            break;
            
        default:
            fprintf(stderr, "Unknown option %c\n", opt);
            break;
        }
    }


    if ((hsocket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("socket");
        goto funcexit;
    }
    
    memset(&ifr, 0, sizeof(struct ifreq));
    memset(&addr, 0, sizeof(struct sockaddr_can));
    
    strcpy(ifr.ifr_name, ifname);
    result = ioctl(hsocket, SIOCGIFINDEX, &ifr);
    if (result < 0)
    {
        printf("ioctl errno: %i \n", errno);
        goto funcexit;
    }
    ifindex = ifr.ifr_ifindex;

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifindex;

    if (bind(hsocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        goto funcexit;
    }

    // enable non block operation
    if (enable_block == 0)
    {
        printf("enable non block operation \n");
        flag = fcntl(hsocket, F_GETFL, 0);
        if (flag < 0)
        {
            printf("fcntl F_GETFL errno: %i \n", errno);
            goto funcexit;
        }
        else
        {
            if (flag & O_NONBLOCK)
            {
                printf("Get flag O_NONBLOCK \n");
            }
            flag = flag | O_NONBLOCK;
            result = fcntl(hsocket, F_SETFL, flag);
            if (result < 0)
            {
                printf("fcntl non block errno: %i \n", errno);
                goto funcexit;
            }
        }
    }
    else
    {
        printf("enable block operation \n");
        flag = fcntl(hsocket, F_GETFL, 0);
        if (flag < 0)
        {
            printf("fcntl F_GETFL errno: %i \n", errno);
            goto funcexit;
        }
        else
        {
            if (flag & O_NONBLOCK)
            {
                printf("Get flag O_NONBLOCK \n");
            }
            flag = flag & ~(O_NONBLOCK);
            result = fcntl(hsocket, F_SETFL, flag);
            if (result < 0)
            {
                printf("fcntl block errno: %i \n", errno);
                goto funcexit;
            }
            flag = fcntl(hsocket, F_GETFL, 0);
            if (flag & O_NONBLOCK)
            {
                printf("dont clear O_NONBLOCK \n");
                goto funcexit;
            }
        }
        // if block operation, you can setting timeout value for read...etc operations.
        // setting timeout value
        memset(&tv, 0, sizeof(tv));
        tv.tv_sec = 5;
        tv.tv_usec = 0; //100ms
        result = setsockopt(hsocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (result < 0)
        {
            printf("setsockopt SO_RCVTIMEO errno: %i \n", errno);
            goto funcexit;
        }
        result = setsockopt(hsocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        if (result < 0)
        {
            printf("setsockopt SO_SNDTIMEO errno: %i \n", errno);
            goto funcexit;
        }
    }

    rfilter[0].can_id   = 0x123;
    rfilter[0].can_mask = CAN_SFF_MASK;
    rfilter[1].can_id   = 0x200;
    rfilter[1].can_mask = 0x700;
    rfilter[2].can_id   = 0x80123456;
    rfilter[2].can_mask = 0x1FFFF000;
    rfilter[3].can_id   = 0x80333333;
    rfilter[3].can_mask = CAN_EFF_MASK;

    //setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    so_rcvbuf = 0;
    socklen = sizeof(so_rcvbuf);
    nbytes = getsockopt(hsocket, SOL_SOCKET, SO_RCVBUF, &so_rcvbuf, &socklen);
    printf("get so_rcvbuf:%i errno:%i \n", so_rcvbuf, errno);

    so_rcvbuf = 256;
    nbytes = setsockopt(hsocket, SOL_SOCKET, SO_RCVBUF, &so_rcvbuf, sizeof(so_rcvbuf));
    printf("set so_rcvbuf:%i errno:%i \n", so_rcvbuf, errno);

    nbytes = getsockopt(hsocket, SOL_SOCKET, SO_RCVBUF, &so_rcvbuf, &socklen);
    printf("get so_rcvbuf:%i errno:%i \n", so_rcvbuf, errno);
    
    so_sndbuf = 0;
    socklen = sizeof(so_sndbuf);
    nbytes = getsockopt(hsocket, SOL_SOCKET, SO_SNDBUF, &so_sndbuf, &socklen);
    printf("get so_sndbuf:%i errno:%i \n", so_sndbuf, errno);

    so_sndbuf = 256;
    nbytes = setsockopt(hsocket, SOL_SOCKET, SO_SNDBUF, &so_sndbuf, sizeof(so_sndbuf));
    printf("set so_sndbuf:%i errno:%i \n", so_sndbuf, errno);

    nbytes = getsockopt(hsocket, SOL_SOCKET, SO_SNDBUF, &so_sndbuf, &socklen);
    printf("get so_sndbuf:%i errno:%i \n", so_sndbuf, errno);
    
    /* get default loopback behaviour */
    
    socklen = sizeof(loopback);
    result = getsockopt(hsocket, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &defaultvalue, &socklen);
    printf("getsockopt CAN_RAW_LOOPBACK:%u \n", defaultvalue);
    
    socklen = sizeof(recv_own_msgs);
    result = getsockopt(hsocket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &defaultvalue, &socklen);
    printf("getsockopt CAN_RAW_RECV_OWN_MSGS:%u \n", defaultvalue);

    //if(set_loopback)
    {
        result = setsockopt(hsocket, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
        if (result < 0)
        {
            printf("setsockopt CAN_RAW_LOOPBACK failed \n");
        }
        else
        {
            printf("setsockopt CAN_RAW_LOOPBACK:%u success \n", loopback);
        }
    }

    //if(set_recv_own_msgs)
    {
        result = setsockopt(hsocket, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs));
        if (result < 0)
        {
            printf("setsockopt CAN_RAW_RECV_OWN_MSGS failed \n");
        }
        else
        {
            printf("setsockopt CAN_RAW_RECV_OWN_MSGS:%d success \n", recv_own_msgs);
        }
    }


    
    
	srand((unsigned)time(NULL));
	can_sid = 0;
	can_eid = 0;
	while (1) {
		result = kbhit();
		if (result == 1) {
			ch = getchar();
			if (ch == 27) {
				break;
			}
		}
		nbytes = recv(hsocket, &frame, sizeof(struct can_frame), MSG_DONTROUTE | MSG_CONFIRM);
		if (nbytes < 0) {
			if (errno == EAGAIN)
				continue;
			printf("read errno: %i \n", errno);
			if (!ignore_errors)
				return 1;
		} else if (nbytes < sizeof(struct can_frame)) {
			printf("read: incomplete CAN frame\n");
			return 1;
		} else if (show_frame) {
			printf("receive ");
			if (frame.can_id & CAN_EFF_FLAG)
				printf("EFF %8X ", frame.can_id & CAN_EFF_MASK);
			else
				printf("SFF %3X ", frame.can_id & CAN_SFF_MASK);
			printf("[%u] ", frame.can_dlc);
			if (frame.can_id & CAN_RTR_FLAG)
				printf("remote request ");
			else {
				for (i = 0; i < frame.can_dlc; i++)
					printf("%02X ", frame.data[i]);
			}
			printf("\n");
		}
	}

    goto funcexit;
funcexit:
    if (hsocket != -1)
    {
        close(hsocket);
        hsocket = -1;
    }
    printf("can_sid=%u can_eid=%u \n", can_sid, can_eid);
    
    printf("#define	ENODEV		19	/* No such device */ \n");
    printf("#define	ENETDOWN	100	/* Network is down */ \n");

    return 0;
}
