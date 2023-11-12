/**
 * udpfwd.c - UDP packet forwarder
 *
 * Copyright(c) 2020 Hancom MDS Inc.
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ifaddrs.h>

#define BUF_LENGTH          2048
#define WELL_KNOWN_PORT_MAX 1023        /* 0 ~ 1023 are reserved for common TCP/IP applications */

/* Local function */
static int   find_ifname(const char *ifname);
static void  usage(void);

/* Local variables */
static uint8_t  udpBuffer[BUF_LENGTH];
static int      srcFd;                 /* source socket */
static int      bcastFd;               /* broadcast socket */

int main(int argc, char *argv[])
{
    struct sockaddr_in srcAddr;        /* Source address */
    struct sockaddr_in srcClientAddr;  /* Source client address */
    char              *srcIfname;      /* source interface */
    struct sockaddr_in bcastAddr;      /* Broadcast address */
    int                bcastFlag;      /* Socket option to set broadcast */
    char              *bcastIfname;    /* Broadcast interface */
    ssize_t            bufLen;         /* UDP buffer length */
    ssize_t            bcastLen;       /* Broadcasted length */
    socklen_t          srcClientSize;  /* Source client address length */
    int                srcPort;        /* Source port */
    int                bcastPort;      /* Broadcast port */
    int                status;
    int                optch;          /* option char */

    /* initialize local variables */
    srcIfname   = NULL;
    bcastIfname = NULL;
    srcPort     = 0;
    bcastPort   = 0;

    /* 
     * gets input arguments 
     */
    opterr = 0;

    while ((optch = getopt(argc, argv, "s:n:b:x:h")) != -1)
    {
        switch (optch)
	{
        case 's':
            srcIfname = optarg;
            break;
	case 'n':
	    srcPort = atoi(optarg);
	    break;
        case 'b':
	    bcastIfname = optarg;
	    break;
	case 'x':
	    bcastPort = atoi(optarg);
	    break;
	case 'h':
	case '?':
	default:
            usage();
	    break;
	}
    }
  
    /* 
     * verify input arguments 
     */
    if (argc != 9)
    {
        usage();
    }
    
    if (srcPort <= WELL_KNOWN_PORT_MAX)
    {
        fprintf(stderr, "source port %d is out of range valid at (1024 ~ 65353)\n", srcPort);
	exit(1);
    }

    if (bcastPort <= WELL_KNOWN_PORT_MAX)
    {
        fprintf(stderr, "broadcast port %d is out of range valid at (1024 ~ 65353)\n", srcPort);
        exit(1);
    }

    if (find_ifname(srcIfname) != 0)
    {
        fprintf(stderr, "Unknown interface %s\n", srcIfname);
	exit(1);
    }

    if (find_ifname(bcastIfname) != 0)
    {
        fprintf(stderr, "Unknown interface %s\n", bcastIfname);
	exit(1);
    }

    printf("sif:%s, bif:%s\n", srcIfname, bcastIfname);

    /* 
     * open source UDP socket 
     */
    srcFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (srcFd == -1)
    {
        perror("Cannot open source UDP socket");
	exit(1);
    }
   
    /* bind source to a single interface */ 
    status = setsockopt(srcFd, SOL_SOCKET, SO_BINDTODEVICE, srcIfname, strlen(srcIfname));
    
    if (status < 0)
    {
        fprintf(stderr, "Cannot bind to interface %s\n", srcIfname);
	close(srcFd);
	exit(1);

    }

    /* bind source address */
    memset((void *)&srcAddr, 0x0, sizeof(srcAddr));

    srcAddr.sin_family      = AF_INET;
    srcAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srcAddr.sin_port        = htons(srcPort);

    status = bind(srcFd, (struct sockaddr *)&srcAddr, sizeof(srcAddr));

    if (status == -1)
    {
        close(srcFd);

        perror("source bind failed");
	exit(1);
    }

    /* 
     * open broadcast UDP socket 
     */
    bcastFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (bcastFd == -1)
    {
        perror("Cannot open destination UDP socket\n");
	
	close(srcFd);
	exit(1);
    }

   /* bind broadcst to a single interface */ 
    status = setsockopt(bcastFd, SOL_SOCKET, SO_BINDTODEVICE, bcastIfname, strlen(bcastIfname));
    
    if (status < 0)
    {
        fprintf(stderr, "Cannot bind to interface %s\n", bcastIfname);
        close(srcFd);
	close(bcastFd);
        exit(1);

    }

    
    /* set socket to allow broadcast */
    bcastFlag = 1;
    
    status = setsockopt(bcastFd, SOL_SOCKET, SO_BROADCAST, (void *)&bcastFlag, sizeof(bcastFlag));
    
    if (status < 0)
    {
        perror("set broadcast permission failed");
	close(srcFd);
	close(bcastFd);
	exit(1);
    }

    
    /* construct broadcast address structure */
    memset((void *)&bcastAddr, 0x0, sizeof(bcastAddr));    /* zero out the structure */
    bcastAddr.sin_family      = AF_INET;                   /* Internet address family */
    bcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);   /* Broadcast IP address */
    bcastAddr.sin_port        = htons(bcastPort);          /* Broadcast port */

    srcClientSize = sizeof(srcClientAddr);
    char * strptr, *strptr2;	
    /* 
     * main loop: wait for a datagram, then broadcast it 
     */

    struct ifaddrs *id, *tmp;
    getifaddrs(&id);
    tmp = id;

    struct sockaddr_in * myAddr;
    memset(&myAddr, 0x0, sizeof(myAddr)); 

    while (tmp)
    {
	if( strcmp(tmp->ifa_name, "wave-data0") == 0)
	{
		if(tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
		{	myAddr = (struct sockaddr_in *) tmp->ifa_addr;				
			strptr2 =inet_ntoa(myAddr->sin_addr);
			printf("my address: %s \n",strptr2);
		}
	}
	tmp = tmp->ifa_next;

    }
    freeifaddrs(id);

    while (1)
    {
        /* receive from source port */
 	//memset((void *)&srcClientAddr, 0x0, sizeof(srcClientAddr));

        bufLen = recvfrom(srcFd, (void *)udpBuffer, BUF_LENGTH, 0, (struct sockaddr *)&srcClientAddr, &srcClientSize);
        
	if (bufLen < 0)
        {
            perror("receive datagram from source");
	    break;
	}
	 strptr = inet_ntoa(srcClientAddr.sin_addr);
      	 printf("recv from client: %s \n", strptr);

	if (srcClientAddr.sin_addr.s_addr == myAddr->sin_addr.s_addr )
		printf ("recv %d but i send\n", bufLen);
	
	else 
	{
		printf("recv = %d\n", bufLen);

		/* broadcast data from source port to clients */
		bcastLen = sendto(bcastFd, (const void *)udpBuffer, bufLen, 0, (const struct sockaddr *)&bcastAddr, sizeof(bcastAddr));
		    
		if (bcastLen < 0)
		{
			perror("broadcasting failed");
			break;
		}

		printf("bcast = %d\n", bcastLen);
	}
	
    }

    /* clean up resources */
    close(srcFd);
    close(bcastFd);

    return (status);
}


/*
 * find_ifname - find network interface named ifname.
 *
 * RETURN: zero if found, otherwise return  error code.
 */
static int find_ifname(const char *ifname)
{
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    int             family;
    int             status;
    
    if ((ifname == NULL) || (strlen(ifname) <= 1))
    {
        status = EINVAL;
    }
    else if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        status = errno;
    }
    else
    {
        /* set initial error code */
	status = ENOENT;

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
            if (ifa->ifa_addr == NULL)
	    {
                continue;
	    }
	    else
	    {
                family = ifa->ifa_addr->sa_family;
                
		/* only IPv4/IPv6 interface */
		if ((family == AF_INET) || (family == AF_INET6))
		{
                    if (strcmp(ifname, ifa->ifa_name) == 0)
		    {
	                status = 0;
		    }
		}
		else
		{
                    continue;
		}
	    }
	}
    }

    return (status);
}

static void usage(void)
{
    printf("Usage:udpfwd [option]\n");
    printf("Options: \n");
    printf("-s <ifname>     : set source interface name (e.g: eth0)\n");
    printf("-n <src port>   : source UDP port\n");
    printf("-b <ifname>     : broadcast interface\n");
    printf("-x <bcast port> : broadcast UDP port\n");

    exit(1);
}
