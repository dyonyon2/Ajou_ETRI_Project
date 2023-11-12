#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/if.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <net/route.h>
#include <net/sock.h>

#define SKB_NETWORK_HDR_RAW(skb) skb_network_header(skb)
#define SKB_NETWORK_HDR_IPH(skb) ((struct iphdr *)skb_network_header(skb))

#define NETLINK_USER 28

struct sock *nl_sk = NULL;

static int pid = 0;
struct nlmsghdr *nlh;

static void hello_nl_recv_msg(struct sk_buff *skb) {

	int msg_size;
	char *msg="Hi! I'm kernel";

	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

	msg_size=strlen(msg);

	nlh=(struct nlmsghdr*)skb->data;
	printk(KERN_INFO "Netlink received msg payload from user:%s\n",(char*)nlmsg_data(nlh));
	pid = nlh->nlmsg_pid;

	printk(KERN_INFO "pid : %d\n\n\n",pid);
}

static unsigned int hooking(void *priv,
			       struct sk_buff *skb,
			       const struct nf_hook_state *state)
{
	unsigned int hooknum=state->hook;	
	struct in_addr ifaddr, bcaddr;
	struct iphdr *iph = SKB_NETWORK_HDR_IPH(skb);

	struct sk_buff *skb_out;
	unsigned int size;
	char *msg;
	unsigned char *tmp,*tmp1;
	int res,i;

	memset(&ifaddr, 0, sizeof(struct in_addr));
	memset(&bcaddr, 0, sizeof(struct in_addr));

	struct udphdr *udph;
	udph = (struct udphdr *)((char *)iph + (iph->ihl << 2));

	if(udph->dest==5632)
		return NF_ACCEPT;
//	printk(KERN_INFO "test1");
	if(pid==0)
		return NF_ACCEPT;
//	printk(KERN_INFO "test2");

	switch (hooknum) {
	case NF_INET_PRE_ROUTING:
		break;
	default:
		return NF_ACCEPT;
	}
//	printk(KERN_INFO "test4");

	printk(KERN_INFO "Entering: %s / port : %d / pid : %d\n", __FUNCTION__,udph->dest,pid);

	tmp=(unsigned char*)&(iph->saddr);
	tmp1=(unsigned char*)&(iph->daddr);

	skb_out = nlmsg_new(skb->len+4,0);

	printk(KERN_INFO "[0] Receive : %d.%d.%d.%d     size : %d(Header : 28)\n",tmp[0],tmp[1],tmp[2],tmp[3],skb->len);
	printk(KERN_INFO "[1] send : [Src] %d.%d.%d.%d\n",tmp[0],tmp[1],tmp[2],tmp[3]);
	printk(KERN_INFO "         : [Dst] %d.%d.%d.%d\n",tmp1[0],tmp1[1],tmp1[2],tmp1[3]);
	printk(KERN_INFO "         : [Size] %d\n",skb->len);


	if(!skb_out)
	{
   		printk(KERN_ERR "******************Failed to allocate new skb******************\n");
    		return 0;
	} 


	nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,skb->len+4,0);  

	NETLINK_CB(skb_out).dst_group = 0;
	memcpy(nlmsg_data(nlh),&(skb->len),4);
	memcpy(nlmsg_data(nlh)+4,skb->data,skb->len);

	res=nlmsg_unicast(nl_sk,skb_out,pid);

	
	if(res<0)
    	{
    		printk(KERN_INFO "Error while sending bak to user\n");
		return 0;
	}

	printk(KERN_INFO "[3] Send Success\n\n");

	return NF_ACCEPT;
};

static struct nf_hook_ops ops = {
	 .hook = hooking,
	 .pf = PF_INET,
	 .hooknum = NF_INET_PRE_ROUTING,
	 .priority = NF_IP_PRI_FIRST,
};

static int __init kajou_init(void)
{
	int ret = -ENOMEM;
	
	printk(KERN_INFO "\n");
	printk(KERN_INFO "kajou init");

	struct netlink_kernel_cfg cfg = {
    		.input = hello_nl_recv_msg,
	};

	nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);

	if(!nl_sk)
	{
    		printk(KERN_ALERT "Error creating socket.\n");
    		return -10;
	}
	else
		printk(KERN_INFO "netlink is successed\n");

	ret = nf_register_hook(&ops);
	if (ret < 0)
	{
		printk(KERN_ALERT " register_hook is failed : %d \n",ret);	
	}
	else
		printk(KERN_INFO "register_hook is successed\n\n");
	
	return ret;
}


static void __exit kajou_exit(void)
{
	nf_unregister_hook(&ops);
	netlink_kernel_release(nl_sk);
}

module_init(kajou_init);
module_exit(kajou_exit);
