/*
 * netlink trial using NETLINK_ROUTE to retrive the info of network devices
 * Reference:
 * man 7 netlink
 * man 7 rtnetlink
 * man 7 netdevice
 * man 3 netlink
 * man 3 rtnetlink
 * man 3 ether_ntoa
 * June 13, 2013
 * daveti@cs.uoregon.edu
 * http://daveti.blog.com
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/netdevice.h>
#include <net/if_arp.h>
#include <linux/if.h>
#include <netinet/if_ether.h>
#include <netinet/ether.h>

/*
 * Copy the struct net_device_stats from kernel to here
 * as there is no such struct in /usr/include/linux/netdevice.h
 * on this machine....
 */
struct net_device_stats {
        unsigned long   rx_packets;
        unsigned long   tx_packets;
        unsigned long   rx_bytes;
        unsigned long   tx_bytes;
        unsigned long   rx_errors;
        unsigned long   tx_errors;
        unsigned long   rx_dropped;
        unsigned long   tx_dropped;
        unsigned long   multicast;
        unsigned long   collisions;
        unsigned long   rx_length_errors;
        unsigned long   rx_over_errors;
        unsigned long   rx_crc_errors;
        unsigned long   rx_frame_errors;
        unsigned long   rx_fifo_errors;
        unsigned long   rx_missed_errors;
        unsigned long   tx_aborted_errors;
        unsigned long   tx_carrier_errors;
        unsigned long   tx_fifo_errors;
        unsigned long   tx_heartbeat_errors;
        unsigned long   tx_window_errors;
        unsigned long   rx_compressed;
        unsigned long   tx_compressed;
};


int main()
{
	int nSocket, nLen, nAttrLen;
	char szBuffer[4096];
	struct
	{
		struct nlmsghdr nh;
		struct ifinfomsg ifi;
	}struReq;
	struct sockaddr_nl struAddr;
	struct nlmsghdr *pstruNL;
	struct ifinfomsg *pstruIF;
	struct rtattr *pstruAttr;
	struct net_device_stats *pstruInfo;
	struct ether_addr *pstruEther;

	/* Create the netlink socket with netlink_route */
	nSocket = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (nSocket < 0)
	{
		fprintf(stderr, "Create socket error: %s\n", strerror(errno));
		return -1;
	}

	/* Bind the socket with the address */
	memset(&struAddr, 0, sizeof(struAddr));
	struAddr.nl_family = AF_NETLINK;
	struAddr.nl_pid = getpid();
	struAddr.nl_groups = 0;
	if (bind(nSocket, (struct sockaddr *)&struAddr, sizeof(struAddr)) < 0)
	{
		fprintf(stderr, "Bind socket error: %s\n", strerror(errno));
		return -1;
	}

	/* Construct the request sending to the kernel */
	memset(&struReq, 0, sizeof(struReq));
	struReq.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struReq));
	struReq.nh.nlmsg_type = RTM_GETLINK;
	struReq.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	struReq.ifi.ifi_family = AF_UNSPEC;

	/* Send the msg to the kernel */
	memset(&struAddr, 0, sizeof(struAddr));
	struAddr.nl_family = AF_NETLINK;
	struAddr.nl_pid = 0;
	struAddr.nl_groups = 0;
	if (sendto(nSocket, &struReq, struReq.nh.nlmsg_len, 0,
			(struct sockaddr *)&struAddr, sizeof(struAddr)) < 0)
	{
		fprintf(stderr, "Send pkg error: %s\n", strerror(errno));
		return -1;
	}

	/* Recv the response with maximum 30 secs waiting */
	alarm(30);
	memset(szBuffer, 0, sizeof(szBuffer));
	while (nLen = recv(nSocket, szBuffer, sizeof(szBuffer), 0))
	{
		/* Shutdown the timer */
		alarm(0);
		pstruNL = (struct nlmsghdr *)szBuffer;
		
		/* Check if there is still any pkg waiting */
		while (NLMSG_OK(pstruNL, nLen))
		{
			/* Got the complete response now */
			if (pstruNL->nlmsg_type == NLMSG_DONE)
			{
				/* Ending pkg - no need to handle */
				break;
			}

			/* Check for error */
			if (pstruNL->nlmsg_type == NLMSG_ERROR)
			{
				/* This is a netlink error msg */
				struct nlmsgerr *pstruError;
				pstruError = (struct nlmsgerr *)NLMSG_DATA(pstruNL);
				fprintf(stderr, "netlink msg error: %s\n", strerror(errno));
				break;
			}

			/* Handle the response from the kernel */
			pstruIF = NLMSG_DATA(pstruNL);
			fprintf(stderr, "Got device[%d] info\n", pstruIF->ifi_index);
			/* Get the device type */
			fprintf(stderr, "\tdevice type: ");
			switch (pstruIF->ifi_type)
			{
				case ARPHRD_ETHER:
					fprintf(stderr, "Ethernet\n");
					break;
				case ARPHRD_PPP:
					fprintf(stderr, "PPP\n");
					break;
				case ARPHRD_LOOPBACK:
					fprintf(stderr, "Loopback\n");
					break;
				default:
					fprintf(stderr, "Unknown\n");
					break;
			}
			/* Get the device status */
			fprintf(stderr, "\tdevice status:");
			if ((pstruIF->ifi_flags & IFF_UP) == IFF_UP)
			{
				fprintf(stderr, " UP");
			}
			if ((pstruIF->ifi_flags & IFF_BROADCAST) == IFF_BROADCAST)
			{
				fprintf(stderr, " BROADCAST");
			}
			if ((pstruIF->ifi_flags & IFF_DEBUG) == IFF_DEBUG)
			{
				fprintf(stderr, " DEBUG");
			}
			if ((pstruIF->ifi_flags & IFF_LOOPBACK) == IFF_LOOPBACK)
			{
				fprintf(stderr, " LOOPBACK");
			}
			if ((pstruIF->ifi_flags & IFF_POINTOPOINT) == IFF_POINTOPOINT)
			{
				fprintf(stderr, " POINTOPOINT");
			}
			if ((pstruIF->ifi_flags & IFF_RUNNING) == IFF_RUNNING)
			{
				fprintf(stderr, " RUNNING");
			}
			if ((pstruIF->ifi_flags & IFF_NOARP) == IFF_NOARP)
			{
				fprintf(stderr, " NOARP");
			}
			if ((pstruIF->ifi_flags & IFF_PROMISC) == IFF_PROMISC)
			{
				fprintf(stderr, " PROMISC");
			}
			if ((pstruIF->ifi_flags & IFF_NOTRAILERS) == IFF_NOTRAILERS)
			{
				fprintf(stderr, " NOTAILERS");
			}
			if ((pstruIF->ifi_flags & IFF_ALLMULTI) == IFF_ALLMULTI)
			{
				fprintf(stderr, " ALLMULTI");
			}
			if ((pstruIF->ifi_flags & IFF_MASTER) == IFF_MASTER)
			{
				fprintf(stderr, " MASTER");
			}
			if ((pstruIF->ifi_flags & IFF_SLAVE) == IFF_SLAVE)
			{
				fprintf(stderr, " SLAVE");
			}
			if ((pstruIF->ifi_flags & IFF_MULTICAST) == IFF_MULTICAST)
			{
				fprintf(stderr, " MULTICAST");
			}
			if ((pstruIF->ifi_flags & IFF_PORTSEL) == IFF_PORTSEL)
			{
				fprintf(stderr, " PORTSEL");
			}
			if ((pstruIF->ifi_flags & IFF_AUTOMEDIA) == IFF_AUTOMEDIA)
			{
				fprintf(stderr, " AUTOMEDIA");
			}
			if ((pstruIF->ifi_flags & IFF_DYNAMIC) == IFF_DYNAMIC)
			{
				fprintf(stderr, " DYNAMIC");
			}
			fprintf(stderr, "\n");

			/* Retrive the attributes */
			pstruAttr = IFLA_RTA(pstruIF);
			nAttrLen = NLMSG_PAYLOAD(pstruNL, sizeof(struct ifinfomsg));
			while (RTA_OK(pstruAttr, nAttrLen))
			{
				/* Check the type of this valid attribute */
				switch (pstruAttr->rta_type)
				{
					case IFLA_IFNAME:
						fprintf(stderr, "\tdevice name: %s\n",
							(char *)RTA_DATA(pstruAttr));
						break;
					case IFLA_MTU:
						fprintf(stderr, "\tdevice MTU: %d\n",
							*(unsigned int *)RTA_DATA(pstruAttr));
						break;
					case IFLA_QDISC:
						fprintf(stderr, "\tdevice Queueing discipline: %s\n",
							(char *)RTA_DATA(pstruAttr));
						break;
					case IFLA_ADDRESS:
						if (pstruIF->ifi_type == ARPHRD_ETHER)
						{
							pstruEther = (struct ether_addr *)RTA_DATA(pstruAttr);
							fprintf(stderr, "\tMAC address: %s\n",
								ether_ntoa(pstruEther));
						}
						break;
					case IFLA_BROADCAST:
						if (pstruIF->ifi_type == ARPHRD_ETHER)
						{
							pstruEther = (struct ether_addr *)RTA_DATA(pstruAttr);
							fprintf(stderr, "\tBROADCAST address: %s\n",
								ether_ntoa(pstruEther));
						}
						break;
					case IFLA_STATS:
						pstruInfo = (struct net_device_stats *)RTA_DATA(pstruAttr);
						fprintf(stderr, "\treceive info:\n");
						fprintf(stderr, "\t\treceive packets: %lu, bytes: %lu\n",
							pstruInfo->rx_packets,
							pstruInfo->rx_bytes);
						fprintf(stderr, "\t\terrors: %lu, dropped: %lu, multicast: %lu, collisions: %lu\n",
							pstruInfo->rx_errors,
							pstruInfo->rx_dropped,
							pstruInfo->multicast,
							pstruInfo->collisions);
						fprintf(stderr, "\t\tlength: %lu, over: %lu, crc: %lu, frame: %lu, fifo: %lu, missed: %lu\n",
							pstruInfo->rx_length_errors,
							pstruInfo->rx_over_errors,
							pstruInfo->rx_crc_errors,
							pstruInfo->rx_frame_errors,
							pstruInfo->rx_fifo_errors,
							pstruInfo->rx_missed_errors);
						fprintf(stderr, "\tsend info:\n");
						fprintf(stderr, "\t\tsend packets: %lu, bytes: %lu\n",
							pstruInfo->tx_packets,
							pstruInfo->tx_bytes);
						fprintf(stderr, "\t\terrors: %lu, dropped: %lu\n",
							pstruInfo->tx_errors,
							pstruInfo->tx_dropped);
						fprintf(stderr, "\t\taborted: %lu, carrier: %lu, fifo: %lu, heartbeat: %lu, window: %lu\n",
							pstruInfo->tx_aborted_errors,
							pstruInfo->tx_carrier_errors,
							pstruInfo->tx_fifo_errors,
							pstruInfo->tx_heartbeat_errors,
							pstruInfo->tx_window_errors);
						break;
					default:
						break;
				}

				/* Get the next attribute */
				pstruAttr = RTA_NEXT(pstruAttr, nAttrLen);
			}

			/* Get the next netlink msg */
			pstruNL = NLMSG_NEXT(pstruNL, nLen);
		}

		/* Clear the buffer before next recv */
		memset(szBuffer, 0, sizeof(szBuffer));
		/* Recover the timer */
		alarm(30);
	}

	return 0;
}
							

		
