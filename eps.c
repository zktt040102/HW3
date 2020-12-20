#include <stdio.h>                      //standard C stuffs
#include <stdlib.h>                     //malloc
#include <errno.h>                      //error code
#include <stdbool.h>                    //boolean type and values
#include <string.h>                     //strlen
#include <sys/socket.h>                 //main sockets header
#include <arpa/inet.h>                  //internet operations definitions
#include </usr/include/netinet/ip.h>    //ipv4 protocols
#include </usr/include/netinet/ip6.h>   //ipv6 protocols
#include </usr/include/pcap/pcap.h>     //pcap library
#include <net/ethernet.h>               //ethernet fundamental onstants
#include <netinet/in.h>                 //internet protocol family
#include <netinet/if_ether.h>           //ethernet header declarations
#include <netinet/ether.h>              //ethernet header declarations
#include <netinet/tcp.h>                //tcp header declarations
#include <netinet/udp.h>                //udp header declarations
#include <netinet/ip_icmp.h>            //icmp header declarations
#include <netinet/icmp6.h>              //icmpv6 header declarations
#include <time.h>     

/* Prototypes */
void handle_packet(u_char *, const struct pcap_pkthdr *, const u_char *);
void handle_ipv6(int, int, const u_char*, char*);
void print_tcp (const u_char*, int*);
void print_udp (const u_char*, int*);
void print_payload (const u_char *, int);
void print_ipv4(char*, char*);
void print_icmp6(const u_char*, int*);
void print_ipv6();

/* Global Variables */
//boolean for traffics and protocols
bool ipv4_bool = false;
bool ipv6_bool = false;
bool tcp_bool = false;
bool udp_bool = false;
bool icmp_bool = false;
bool other_traffic_bool = false;
bool unknown_protocol_bool = false;

int packet_counter = 0; //packet number
int headerLength = 0;       //packet header length

/* Declear IPv6 traffic source and destination address */
char sourIP6[INET_ADDRSTRLEN];  //source address
char destIP6[INET_ADDRSTRLEN];  //destination address

struct time_val
{
long tv_sec;         /* seconds 含义同 time_t 对象的值 */
long tv_usec;        /* and microseconds */
};

struct pcap_pkthdr *ptk_header;



/* Main */
int main(int argc, char *argv[]) 
{
    const char *fname = argv[1];   //pacp filename
    char errbuf[PCAP_ERRBUF_SIZE]; //error buffer
    pcap_t *handle;                //handle captured packet file
    ptk_header  = (struct pcap_pkthdr *)malloc(sizeof(struct pcap_pkthdr));

    //handle if pcap file is missing
    if(argc == 1){
	printf("Error: pcap file is missing! \n");
	printf("Please use following format command: $./eps [captured_file_name] \n");
	exit(EXIT_FAILURE);
    }
    
    //only one traffic and one protocol determined for every packet, others will be unvisable
    for(int i = 2; i < argc; i++){
	if(strcasecmp("IPV4", argv[i]) == 0){
	    ipv4_bool = true;
	}
	else if(strcasecmp("IPV6", argv[i]) == 0){
	    ipv6_bool = true;
	}
	else if(strcasecmp("TCP", argv[i]) == 0){
	    tcp_bool = true;
	}
	else if(strcasecmp("UDP", argv[i]) == 0){
	    udp_bool = true;
	}
	else if(strcasecmp("ICMP", argv[i]) == 0){
	    icmp_bool = true;
	}
	else if(strcasecmp("UNKNOWN", argv[i]) == 0){
	    unknown_protocol_bool = true;
	}
    }
    
    //accept all traffic if no other options
    if(argc == 2){
	ipv4_bool = true;
	ipv6_bool = true;
	other_traffic_bool = true;
    }
    
    if((ipv4_bool == true || ipv6_bool == true) && tcp_bool == false && udp_bool == false && icmp_bool == false && unknown_protocol_bool == false){
	tcp_bool = true;
	udp_bool = true;
	icmp_bool = true;
	unknown_protocol_bool = true;
    }
    
    //handle error if command is wrong
    if(argc > 2){
	printf("Error: unrecognized command! \n");
	printf("Please use following format command: $./eps [captured_file_name] \n");
	exit(EXIT_FAILURE);
    }
    
    //open pacp file
    handle = pcap_open_offline(fname, errbuf);

    FILE *fp;

    //if pacp file has errors
    if(handle == NULL){
	printf("pcap file [%s] with error %s \n", fname, errbuf);
	exit(EXIT_FAILURE);
    }

    fp = fopen(fname,"r");
    if(fread(ptk_header, 16, 1, fp) != 1) //读pcap数据包头结构
    {
	return 0;
    }

    //pacp loop to set our callback function
    pcap_loop(handle, 0, handle_packet, NULL);
    fclose(fp);
    return 1;
}

/* Handle packet */
void handle_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    //declear pointers to packet headers
    const struct ether_header *ethernet_header; //ethernet header
    const struct ip *ipv4_header;               //ipv4 header
    const struct ip6_hdr *ipv6_header;          //ipv6 header
    const struct tcphdr *tcp_header;            //tcp header
    const struct udphdr *udp_header;            //udp header
    const struct icmphdr *icmp_header;          //icmp header
    
    //declear IPv4 source and destination address
    char sourIP4[INET_ADDRSTRLEN];  //source address
    char destIP4[INET_ADDRSTRLEN];  //destination address


    //get header length
    headerLength = header->len;

    //increase packet counter -> packet number
    ++packet_counter;

    //define ethernet header
    ethernet_header = (struct ether_header*)(packet);
    
    //get etherent header size
    int size = 0;
    size += sizeof(struct ether_header);

    //now, it's time to determine the traffic type and protocol type
    switch(ntohs(ethernet_header->ether_type)){
	//IPv4 traffic
	case ETHERTYPE_IP:
		if(ipv4_bool == false){
		    return;
		}
		
		//get ipv4 header
		ipv4_header = (struct ip*)(packet + size);
		
		//get ipv4 source and destination address
		inet_ntop(AF_INET, &(ipv4_header->ip_src), sourIP4, INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &(ipv4_header->ip_dst), destIP4, INET_ADDRSTRLEN);
		
		//get ipv4 header size
		size += sizeof(struct ip);
		
		//payload
		u_char *payload;
		int dataLength = 0;
		
		//now, we need to determine the protocol type based on ipv4 header
		switch(ipv4_header->ip_p){
		    //TCP
		    case IPPROTO_TCP:
			if(tcp_bool == false){
			    return;
			}
			print_ipv4(sourIP4, destIP4);
			print_tcp(packet, &size);
			break;
		    
		    //UDP
		    case IPPROTO_UDP:
			if(udp_bool == false){
			    return;
			}
			print_ipv4(sourIP4, destIP4);
			print_udp(packet, &size);
			break;
		
		    //ICMPv4
		    case IPPROTO_ICMP:  
			if(icmp_bool == false){
			    return;
			}
			print_ipv4(sourIP4, destIP4);
			printf("Protocol: ICMP \n"); 
			
			//get icmp header
			icmp_header = (struct icmphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip));
			u_int type = icmp_header->type;
			
			//determine what error message
			if(type == 11){
			    printf("TTL Expired! \n");
			}
			else if(type == ICMP_ECHOREPLY){
			    printf("ICMP Echo Reply! \n");
			}
		  
			//print out payload data
			payload = (u_char*)(packet + sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct icmphdr));
			dataLength = header->len - (sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct icmphdr)); 
			printf("Payload: (%d bytes) \n", dataLength);
			printf("\n");
			print_payload(payload, dataLength);

			break;
		
		     //Unknow protocol
		     default:
			if(unknown_protocol_bool == false){
			    return;
			}
			printf("Protocol: Unknown \n");
			break;
		}
		break;
		
	//IPv6
	case ETHERTYPE_IPV6:
		if(ipv6_bool == false){
		    return;
		}
		
		//get ipv6 header
		ipv6_header = (struct ip6_hdr*)(packet + size); 

		//get ipv6 source and destination address
		inet_ntop(AF_INET6, &(ipv6_header->ip6_src), sourIP6, INET6_ADDRSTRLEN);
		inet_ntop(AF_INET6, &(ipv6_header->ip6_dst), destIP6, INET6_ADDRSTRLEN);
		
		//next header in ipv6
		int nextheader = ipv6_header->ip6_nxt;

		size += sizeof(struct ip6_hdr);

		char string[100] = " ";

		handle_ipv6(nextheader, size, packet, string);

		break;
	
	//Other traffic
	default:
		if(other_traffic_bool == false){
		    return;
		}
		printf("Ether Type: Other \n");
		break;
    }
}

/* Handle IPv6 header */
void handle_ipv6(int header, int size, const u_char *packet, char *string)
{
    //now, we need to determine the protocol tyoe in ipv6
    switch(header){
	//rounting
	case IPPROTO_ROUTING:
		strcat(string, "ROUTING, ");
		struct ip6_rthdr* header = (struct ip6_rthdr*)(packet + size); 
		size+=sizeof(struct ip6_rthdr);
		print_ipv6(header->ip6r_nxt, size, packet, string);
		break;
	
	//hop by hop
	case IPPROTO_HOPOPTS:
		strcat(string, "HOP-BY_HOP, ");
		struct ip6_hbh* header_hop = (struct ip6_hbh*)(packet + size); 
		size+=sizeof(struct ip6_hbh);
		print_ipv6(header_hop->ip6h_nxt, size, packet, string);
		break;
	
	//fragmentation
	case IPPROTO_FRAGMENT:
		strcat(string, "FRAGMENTATION, ");
		struct ip6_frag* header_frag = (struct ip6_frag*)(packet + size); 
		size+=sizeof(struct ip6_frag);
		print_ipv6(header_frag->ip6f_nxt, size, packet, string);
		break;
	
	//destination options
	case IPPROTO_DSTOPTS:
		strcat(string, "Destination options, ");
		struct ip6_dest* header_dest = (struct ip6_dest*)(packet + size); 
		size+=sizeof(struct ip6_dest);
		print_ipv6(header_dest->ip6d_nxt, size, packet, string);
		break;
	
	//TCP
	case IPPROTO_TCP:
		if(tcp_bool == false){
		    return;
		}
		print_ipv6();
		printf("%s \n", string);
		print_tcp(packet, &size);
		break;
	
	//UDP
	case IPPROTO_UDP:
		if(udp_bool == false){
		    return;
		}
		print_ipv6();
		printf("%s \n", string);
		print_udp(packet, &size);
		break;
	
	//ICMPv6
	case IPPROTO_ICMPV6:
		if(icmp_bool == false){
		    return;
		}
		print_ipv6();
		printf("%s \n", string);
		print_icmp6(packet, &size);
		break;
		
	//Unknown
	default:
		if(unknown_protocol_bool == false){
		    return;
		}
		print_ipv6();
		printf("Protocol: Unknown \n");
		break;
    }
}

/* Print out IPv6 header */
void print_ipv6()
{
    char my_time[20]; 
    printf("\n");
    printf("********************************************************* \n");
    printf("Packet #: %d \n", packet_counter);
    printf("Ether Type: IPv6 \n");
    printf("From: %s \n", sourIP6);
    printf("To: %s \n", destIP6);
    //strftime(my_time, sizeof(my_time), "%Y-%m-%d %T", localtime(&(ptk_header->ts.tv_sec)));
    printf("Time: %s \n", ctime((const time_t*)&ptk_header->ts.tv_sec));

    printf("Extension Headers:");
}

/* Print out ICMPv6 header */
void print_icmp6(const u_char *packet, int *size)
{
    printf("Protocol: ICMPv6 \n");
    
    u_char *payload;
    int dataLength = 0;

    //get icmp6 header
    struct icmp6_hdr* header_icmp6 = (struct icmp6_hdr*)(packet+*size);

    //get and print out payload data
    payload = (u_char*)(packet + *size + sizeof(struct icmp6_hdr));
    dataLength = headerLength - *size + sizeof(struct icmp6_hdr); 
    
    printf("Payload: (%d bytes) \n", dataLength);
    print_payload(payload, dataLength);
}

/* Print out TCP header */
void print_tcp(const u_char *packet, int *size)
{    
    const struct tcphdr* tcp_header;
    
    u_int sourPort, destPort;  //source and destination port number
    u_char *payload;           //payload
    int dataLength = 0;
    
    //get tcp header
    tcp_header = (struct tcphdr*)(packet + *size);

    //get source and destination port number
    sourPort = ntohs(tcp_header->source);
    destPort = ntohs(tcp_header->dest);

    //get payload
    *size += tcp_header->doff*4;
    payload = (u_char*)(packet + *size);
    dataLength = headerLength - *size;

    //print out protocol details
    printf("protocol: TCP \n");
    printf("Src port: %d\n", sourPort);
    printf("Dst port: %d\n", destPort);
    printf("Payload: (%d bytes) \n", dataLength);
    printf("\n");

    //print out payload data
    print_payload(payload, dataLength);
}

/* Print out UDP header */
void print_udp(const u_char *packet, int *size)
{     
    const struct udphdr* udp_header;
    
    u_int sourPort, destPort;  //source and destination port number
    u_char *payload;           //payload
    int dataLength = 0;

    //get udp header
    udp_header = (struct udphdr*)(packet + *size);
    
    //get source and destination port number
    sourPort = ntohs(udp_header->source);
    destPort = ntohs(udp_header->dest);

    //get payload data
    *size+=sizeof(struct udphdr);
    payload = (u_char*)(packet + *size);
    dataLength = headerLength - *size;
    
    //print out protocol details
    printf("protocol: UDP \n");
    printf("Src port: %d\n", sourPort);
    printf("Dst port: %d\n", destPort);
    printf("Payload: (%d bytes) \n", dataLength);
    printf("\n");
    
    //print out payload data
    print_payload(payload, dataLength);
}

/* Print out IPv4 header */
void print_ipv4(char *source, char *dest)
{
    char my_time[20]; 
    printf("\n");
    printf("********************************************************* \n");
    printf("Packet #: %d \n", packet_counter);
    printf("Ether Type: IPv4 \n");
    printf("From: %s \n", source);
    //strftime(my_time, sizeof(my_time), "%Y-%m-%d %T", localtime(&(ptk_header->ts.tv_sec)));
    printf("Time: %s \n", ctime((const time_t*)&ptk_header->ts.tv_sec));
    printf("To: %s \n", dest);
    
}

/* Print out payload data with 16 bytes each row (offset hex ascii)*/

void print_payload(const u_char *payload, int Size)
{
    int i , j;
    for(i = 0; i < Size; i++){
        if( i!=0 && i%16==0){   //if one line of hex printing is complete...
            printf("         ");
            
	    for(j = i - 16; j < i; j++){
                if(payload[j] >= 32 && payload[j] <= 128){
                    printf("%c",(unsigned char)payload[j]); //if its a number or alphabet
		}
                else{
		    printf("."); //otherwise print a dot
		}
            }
            printf("\n");
        }
         
        if(i%16 == 0) printf("   ");
            printf(" %02X",(unsigned int)payload[i]);
                 
        if(i == Size - 1){  //print the last spaces
            for(j = 0; j < 15 - i%16; j++){
		printf("   "); //extra spaces
            }
             
            printf("         ");
             
            for(j = i - i%16; j <= i; j++){
                if(payload[j] >= 32 && payload[j] <= 128){
		    printf("%c",(unsigned char)payload[j]);
                }
                else{
		    printf(".");
                }
            }
            printf("\n" );
        }
    }
}

