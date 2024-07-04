//------------------------------------------------------------------------------
/**
 * @file nlp_server_ctrl.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief ODROID Network label printer and iperf server control application.
 * @version 0.1
 * @date 2022-12-08
 *
 * @copyright Copyright (c) 2022
 *
 */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linux/fb.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include "nlp_server_ctrl.h"
//------------------------------------------------------------------------------
#define	NET_DEFAULT_NAME	"eth0"
#define	NET_DEFAULT_PORT	8888
#define NET_IP_BASE			"192.168."

// maximum number of characters in command line.
#define	CMD_LINE_CHARS		128

//------------------------------------------------------------------------------
//	function prototype
//------------------------------------------------------------------------------
int get_my_mac	(char *mac_str);
int get_my_ip	(char *ip_str);
int net_status	(char *ip_addr);

int iperf3_speed_check 			(char *ip_addr, char mode);

static int mac_file_check		(char *mac_str);
static int ip_check_cmd			(char *ip_str);
static int read_with_timeout	(int fd, char *buf, int buf_size, int timeout_ms);
static int check_ip_range		(char *ip_addr);

void nlp_server_disconnect	(int nlp_server_fp);
int nlp_server_connect		(char *ip_addr);
int nlp_server_version 		(char *ip_addr, char *rdver);
int nlp_server_find			(char *ip_addr);
int nlp_server_write		(char *ip_addr, char mtype, char *msg, char ch);

//------------------------------------------------------------------------------
const char *mac_address_file = "/sys/class/net/eth0/address";
const char *mac_linkspd_file = "/sys/class/net/eth0/speed";

//------------------------------------------------------------------------------
static int mac_file_check (char *mac_str)
{
	if (access (mac_address_file, F_OK) == 0) {
		FILE *fp = fopen (mac_address_file, "r");
		if (fp != NULL) {
			char mac_read[20];
			if (fgets (mac_read, sizeof(mac_read), fp) != NULL) {
				if (!strncmp (mac_read, "00:1e:06", strlen("00:1e:06"))) {
					sprintf (mac_str, "001e06%c%c%c%c%c%c",
							mac_read[ 9],mac_read[10],
							mac_read[12],mac_read[13],
							mac_read[15],mac_read[17]);
					fclose(fp);
					return 1;
				}
			}
			fclose(fp);
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
int get_my_mac (char *mac_str)
{
	int sock, if_count, i;
	struct ifconf ifc;
	struct ifreq ifr[10];
	unsigned char mac[6];

	if (mac_file_check(mac_str))
		return 1;

	memset(&ifc, 0, sizeof(struct ifconf));
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	// 검색 최대 개수는 10개
	ifc.ifc_len = 10 * sizeof(struct ifreq);
	ifc.ifc_buf = (char *)ifr;

	// 네트웨크 카드 장치의 정보 얻어옴.
	ioctl(sock, SIOCGIFCONF, (char *)&ifc);

	// 읽어온 장치의 개수 파악
	if_count = ifc.ifc_len / (sizeof(struct ifreq));
	for (i = 0; i < if_count; i++) {
		if (ioctl(sock, SIOCGIFHWADDR, &ifr[i]) == 0) {
			memcpy(mac, ifr[i].ifr_hwaddr.sa_data, 6);
			fprintf(stdout, "find device (%s), mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
				ifr[i].ifr_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			memset (mac_str, 0, 20);
			sprintf(mac_str, "%02x%02x%02x%02x%02x%02x",
						mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
			fprintf (stdout, "My mac addr(NET_NAME = %s) = %s\n", NET_DEFAULT_NAME, mac);
			if (!strncmp ("001e06", mac_str, strlen("001e06")))
				return 1;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int ip_check_cmd (char *ip_str)
{
	FILE *fp;
	char ip_addr[20];

	if ((fp = popen("hostname -I", "r")) != NULL) {
		memset (ip_addr, 0x00, sizeof(ip_addr));
		while (fgets(ip_addr, 2048, fp)) {
			if (NULL != strstr(ip_addr, NET_IP_BASE)) {
				fprintf(stdout, "[DGB] %s = %s\n", __func__, ip_addr);
				strncpy (ip_str, ip_addr, strlen(ip_addr) -1);
				pclose(fp);
				return 1;
			}
		}
		pclose(fp);
	}
	return 0;
}

//------------------------------------------------------------------------------
//	현재 device의 eth0에 할당되어진 ip를 얻어온다.
//	성공 return 1, 실패 return 0
//------------------------------------------------------------------------------
int get_my_ip (char *ip_str)
{
	int fd;
	struct ifreq ifr;
	char ip[16];

	if (ip_check_cmd (ip_str))
		return 1;

	/* this entire function is almost copied from ethtool source code */
	/* Open control socket. */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		fprintf (stdout, "Cannot get control socket\n");
		return 0;
	}
	strncpy(ifr.ifr_name, NET_DEFAULT_NAME, IFNAMSIZ); 
	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) { 
		fprintf (stdout, "SIOCGIFADDR ioctl Error!!\n"); 
		close(fd);
		return 0;
	}
	memset(ip, 0x00, sizeof(ip));
	inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2, ip, sizeof(struct sockaddr)); 
	fprintf (stdout, "My ip addr(NET_NAME = %s) = %s\n", NET_DEFAULT_NAME, ip);

	strncpy (ip_str, ip, strlen(ip));
	return 1;
}

//------------------------------------------------------------------------------
int net_status (char *ip_addr)
{
	char cmd_line[CMD_LINE_CHARS];
	FILE *fp;

	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf (cmd_line, "ping -c 1 -w 1 %s", ip_addr);
	fprintf (stdout, "[DGB] cmd_line =  %s\n", cmd_line);
	if ((fp = popen(cmd_line, "r")) != NULL) {
		memset (cmd_line, 0x00, sizeof(cmd_line));
		while (fgets(cmd_line, 2048, fp)) {
			if (NULL != strstr(cmd_line, "1 received")) {
				pclose(fp);
				fprintf(stdout, "[DGB] %s = alive\n", __func__);
				return 1;
			}
		}
		pclose(fp);
	}
	fprintf(stdout, "%s = dead\n", __func__);
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int iperf3_speed_check (char *ip_addr, char mode)
{
	char cmd_line[CMD_LINE_CHARS], *ptr;
	FILE *fp;

	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf (cmd_line, "iperf3 -t 1 %s -c %s",
					mode == NLP_SERVER_MSG_TYPE_TCP ? "-b G" : "-b G -u", ip_addr);
	fprintf (stdout, "[DGB] cmd_line =  %s\n", cmd_line);
	if ((fp = popen(cmd_line, "r")) != NULL) {
		memset (cmd_line, 0x00, sizeof(cmd_line));
		while (fgets(cmd_line, 2048, fp)) {
			if (NULL != (ptr = strstr(cmd_line, "MBytes"))) {
				ptr += strlen("MBytes");
				pclose (fp);
				return atoi (ptr);
			}
		}
		pclose(fp);
	}
	return 0;
}

//------------------------------------------------------------------------------
// TCP/UDP 데이터 read (timeout가능)
//------------------------------------------------------------------------------
static int read_with_timeout (int fd, char *buf, int buf_size, int timeout_ms)
{
	int rx_len = 0;
	struct	timeval  timeout;
	fd_set	readFds;

	// recive time out config
	// Set 1ms timeout counter
	timeout.tv_sec  = 0;
	timeout.tv_usec = timeout_ms*1000;

	FD_ZERO(&readFds);
	FD_SET(fd, &readFds);
	select(fd+1, &readFds, NULL, NULL, &timeout);

	if(FD_ISSET(fd, &readFds))
	{
		rx_len = read(fd, buf, buf_size);
	}

	return rx_len;
}

//------------------------------------------------------------------------------
//	주소 범위가 현재 장치의 주소 영역에 있는지 확인한다.
//	성공 return 1, 실패 return 0
//------------------------------------------------------------------------------
static int check_ip_range (char *ip_addr)
{
	char *m_tok, *n_tok, my_ip[20], nlp_server_ip[20];
	int m1, m2, m3, n1, n2, n3;

	memset(my_ip, 0, sizeof(my_ip));
	if (!get_my_ip (my_ip)) {
		fprintf (stdout, "Network device not found!\n");
		return 0;
	}

	memset (nlp_server_ip, 0, sizeof(nlp_server_ip));
	strncpy(nlp_server_ip, ip_addr, strlen(ip_addr));

	m_tok = strtok(my_ip, ".");		m1 = atoi(m_tok);
	m_tok = strtok( NULL, ".");		m2 = atoi(m_tok);
	m_tok = strtok( NULL, ".");		m3 = atoi(m_tok);

	n_tok = strtok(nlp_server_ip, ".");	n1 = atoi(n_tok);
	n_tok = strtok(  NULL, ".");		n2 = atoi(n_tok);
	n_tok = strtok(  NULL, ".");		n3 = atoi(n_tok);

	/* compare m1.m2.m3.* == n1.n2.n3.* (ip form : aaa.bbb.ccc.ddd) */
	if ((m1 != n1) || (m2 != n2) || (m3 != n3))
		return 0;

	return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//	연결을 해제한다.
//------------------------------------------------------------------------------
void nlp_server_disconnect (int nlp_server_fp)
{
	if (nlp_server_fp)
		close (nlp_server_fp);
}

//------------------------------------------------------------------------------
//	입력된 주소로 연결한다.
//
//	성공 return nlp_fp, 실패 return 0
//------------------------------------------------------------------------------
int nlp_server_connect (char *ip_addr)
{
	int nlp_server_fp, len;
	struct sockaddr_in s_addr;

	if (!check_ip_range (ip_addr)) {
		fprintf (stdout, "Out of range IP Address! (%s)\n", ip_addr);
		return 0;
	}

	// ip port ping test
	if (!net_status (ip_addr))
		return 0;

	if((nlp_server_fp = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) <0){
		fprintf (stdout, "socket create error : \n");
		return 0;
	}
	len = sizeof(struct sockaddr_in);

	memset (&s_addr, 0, len);

	//소켓에 접속할 주소 지정
	s_addr.sin_family 		= AF_INET;
	s_addr.sin_addr.s_addr 	= inet_addr(ip_addr);
	s_addr.sin_port 		= htons(NET_DEFAULT_PORT);

	// 지정한 주소로 접속
	if(connect (nlp_server_fp, (struct sockaddr *)&s_addr, len) < 0) {
		fprintf (stdout, "connect error : %s\n", ip_addr);
		nlp_server_disconnect (nlp_server_fp);
		return 0;
	}
	return nlp_server_fp;
}

//------------------------------------------------------------------------------
//	프로그램 버전을 확인한다.
//
//	성공 return 1, 실패 return 0
//------------------------------------------------------------------------------
int nlp_server_version (char *ip_addr, char *rdver)
{
	char sbuf[16], rbuf[16];
	int nlp_server_fp = nlp_server_connect (ip_addr), timeout = 1000;

	memset (sbuf, 0, sizeof(sbuf));
	memset (rbuf, 0, sizeof(rbuf));

	strcpy(sbuf, "version");
	write (nlp_server_fp, sbuf, strlen(sbuf));

	// wait read ack
	if (read_with_timeout(nlp_server_fp, rbuf, sizeof(rbuf), timeout)) {
		fprintf (stdout,"read version is %s\n", rbuf);
		strncpy(rdver, rbuf, strlen(rbuf));
	}	else
		fprintf (stdout,"read time out %d ms or rbuf is NULL!\n", timeout);

	nlp_server_disconnect (nlp_server_fp);
	return nlp_server_fp ? 1 : 0;
}

//------------------------------------------------------------------------------
//	nmap을 실행하여 나의 eth0에 할당되어진 현재 주소를 얻은 후 
//	같은 네트워크상의 연결된 장치를 스켄하여 연결을 시도한다.
//	성공시 입력 변수에 접속되어진 주소를 복사한다.
//	성공 return 1, 실패 return 0
//------------------------------------------------------------------------------
int nlp_server_find (char *ip_addr)
{
	FILE *fp;
	char cmd_line[CMD_LINE_CHARS], *ip_tok;
	int ip;

	if (!get_my_ip (ip_addr))	{
		fprintf (stdout,"Network device not found!\n");
		return 0;
	}

	ip_tok = strtok(ip_addr, ".");	ip_tok = strtok(NULL,     ".");
	ip_tok = strtok(NULL,     ".");	ip = atoi(ip_tok);

	memset(cmd_line, 0, sizeof(cmd_line));
	sprintf(cmd_line, "nmap %s%d.* -p T:%4d --open 2<&1", NET_IP_BASE, ip, NET_DEFAULT_PORT);

	if ((fp = popen(cmd_line, "r")) != NULL) {
		memset(cmd_line, 0, sizeof(cmd_line));
		while (fgets(cmd_line, sizeof(cmd_line), fp)) {
			ip_tok = strstr(cmd_line, NET_IP_BASE);
			if (ip_tok != NULL) {
				if (net_status (ip_tok)) {
					// ip_addr arrary size = 20 bytes
					memset (ip_addr, 0, 20);
					strncpy(ip_addr, ip_tok, strlen(ip_tok)-1);
					pclose(fp);
					return 1;
				}
			}
		}
	}
	pclose(fp);
	return 0;
}

//------------------------------------------------------------------------------
//	Network label printer에 MAC 또는 에러메세지를 출력한다.
//  입력된 주소를 가지고 connect 상태를 확인한다.
//	실패시 현재 자신의 주소를 확인 후 자신의 주소를 기준으로 nmap을 실행한다.
//
//  TCP/IP port 8888로 응답이 있고 mac addr이 00:1e:06:xx:xx:xx인 경우 해당함.
//
//	nmap 192.168.xxx.xxx -p T:8888 --open
//
//	scan되어진 ip중 연결되는 ip가 있다면 프린터라고 간주한다.
//	프린터에 메세지를 출력하고 출력이 정상인지 확인한다.
//	정상적으로 연결되어진 ip를 입력되어진 주소영역에 저장한다.
//
//	mtype : 1 (mac addr), 2 (error)
//	msg
//		mtype : 1 -> 00:1e:06:xx:xx:xx
//		mtype : 2 -> err1, err2, ...
//	ch : 1 (left), 2 (right)
//
//	성공 return 1, 실패 return 0
//------------------------------------------------------------------------------
int nlp_server_write (char *ip_addr, char mtype, char *msg, char ch)
{
	int nlp_server_fp, len;
	char sbuf[256], nlp_server_addr[20], nlp_server_ver[20];

	memset(nlp_server_ver , 0, sizeof(nlp_server_ver));
	memset(nlp_server_addr, 0, sizeof(nlp_server_addr));
	memcpy(nlp_server_addr, ip_addr, strlen(ip_addr));

	if (!net_status(nlp_server_addr)) {
		fprintf(stdout, "Netwrok unreachable!! (%s)\n", ip_addr);
		return 0;
	}

	if (!(nlp_server_version (ip_addr, nlp_server_ver))) {
		fprintf(stdout, "Network Label Printer get version error. ip = %s\n", ip_addr);
		return 0;
	}

	// 서버로 보내질 문자열 합치기
	memset (sbuf, 0, sizeof(sbuf));

	if ((mtype == NLP_SERVER_MSG_TYPE_TCP) || (mtype == NLP_SERVER_MSG_TYPE_UDP)) {
		sprintf (sbuf, "iperf %s", msg);
		fprintf(stdout, "send msg : %s\n", sbuf);
	} else {
		if (!strncmp(nlp_server_ver, "202204", strlen("202204")-1)) {
			// charles modified version
			fprintf(stdout, "new version nlp-printer. ver = %s\n", nlp_server_ver);
			sprintf(sbuf, "%s-%c,%s",
							ch == NLP_SERVER_CHANNEL_RIGHT ? "right" : "left",
							mtype == NLP_SERVER_MSG_TYPE_ERR ? 'e' : 'm',
							msg);
		} else {
			fprintf(stdout, "old version nlp-printer.\n");
			// original version
			if (mtype == NLP_SERVER_MSG_TYPE_ERR)
				sprintf(sbuf, "error,%c,%s", ch ? '>' : '<', msg);
			else
				sprintf(sbuf, "mac,%s", msg);
		}
	}

	// 소켓통신 활성화
	if (!(nlp_server_fp = nlp_server_connect (ip_addr))) {
		fprintf(stdout, "Network Label Printer connect error. ip = %s\n", ip_addr);
		return 0;
	}
	// 서버로 문자열 전송
	if ((len = write (nlp_server_fp, sbuf, strlen(sbuf))) != (int)strlen(sbuf))
		fprintf(stdout, "send bytes error : buf = %ld, write = %d\n", strlen(sbuf), len);

	// 소켓 닫음
	nlp_server_disconnect (nlp_server_fp);

	return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if defined(__DEBUG_APP__)
//------------------------------------------------------------------------------
static char *OPT_NLP_SERVER_ADDR	= "192.168.0.0";
static char *OPT_MSG_STR 			= NULL;
static char OPT_MSG_TYPE			= 0;
static char OPT_CHANNEL				= 0;
static char OPT_NLP_SERVER_FIND		= 0;
static char OPT_MAC_PRINT			= 0;
static char OPT_IPERF3_MODE			= 0;

static char NlpServerIP [20] = {0,};
static char NlpServerMsg[64] = {0,};

//------------------------------------------------------------------------------
static void print_usage(const char *prog)
{
	printf("Usage: %s [-fpactm]\n", prog);

	puts(
		/* find server command : nmap {192.168.*.*} -p T:8888 --open */
		"  -f --find_nlp  find network printer server ip.(T:8888)\n"
	    "  -p --mac_print print the macaddress of current device.(find 00:1e:06:xx:xx:xx)\n"
	    "  -a --nlp_addr  Network printer server ip address.(default = 192.168.0.0)\n"
	    "  -c --channel   Message channel (left or right, default = left)\n"
		/* iperf3 udp test command : ipref3 -c {server ip} -t 1 -b G -u */
		"  -u --iperf3    iperf3 udp client speed check\n"
		/* iperf3 udp test command : ipref3 -c {server ip} -t 1 */
		"  -i --iperf3    iperf3 tcp client speed check\n "
		"  -e --msg       error msg print"
	    "  -m --macaddr   print mac address.\n"
		"                 mac address data form is 001e06??????\n"
		"   e.g) nlp_server_test -a 192.168.20.10 -c left -t error -m usb,sata,hdd\n"
		"        nlp_server_test -f -c right -e usb,sata,nvme\n"
		"        nlp_server_test -f -c right -m 001e06234567\n"
	);
	exit(1);
}

//------------------------------------------------------------------------------
static void parse_opts (int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "find_nlp",  	0, 0, 'f' },
			{ "mac_print",	0, 0, 'p' },
			{ "nlp_addr",	1, 0, 'a' },
			{ "channle",	1, 0, 'c' },
			{ "ipref3_udp",	0, 0, 'u' },
			{ "iperf3_tcp",	0, 0, 'i' },
			{ "err_msg",	1, 0, 'e' },
			{ "mac_addr",	1, 0, 'm' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "fpa:c:uie:m:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'f':
			OPT_NLP_SERVER_FIND = 1;
			break;
		case 'p':
			OPT_MAC_PRINT = 1;
			break;
		case 'a':
			OPT_NLP_SERVER_ADDR = optarg;
			break;
		case 'c':
            if (!strncmp("right", optarg, strlen("right")))
				OPT_CHANNEL = NLP_SERVER_CHANNEL_RIGHT;
			else
				OPT_CHANNEL = NLP_SERVER_CHANNEL_LEFT;
			break;
		case 'u':
			OPT_IPERF3_MODE = 1;
			OPT_MSG_TYPE = NLP_SERVER_MSG_TYPE_UDP;
			break;
		case 'i':
			OPT_IPERF3_MODE = 1;
			OPT_MSG_TYPE = NLP_SERVER_MSG_TYPE_TCP;
			break;
		case 'e':
			OPT_MSG_TYPE = NLP_SERVER_MSG_TYPE_ERR;
			OPT_MSG_STR = optarg;
			break;
		case 'm':
			OPT_MSG_TYPE = NLP_SERVER_MSG_TYPE_MAC;
			OPT_MSG_STR = optarg;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    parse_opts(argc, argv);

	memset (NlpServerIP, 0, sizeof(NlpServerIP));
	strncpy (NlpServerIP, OPT_NLP_SERVER_ADDR, strlen(OPT_NLP_SERVER_ADDR));

	if (OPT_NLP_SERVER_FIND) {
		if (nlp_server_find (NlpServerIP))
			fprintf (stdout, "Network Label Printer Fond! IP = %s\n", NlpServerIP);
		else {
			fprintf (stdout, "Network Label Printer not found!\n");
			return 0;
		}
	}

	if (OPT_IPERF3_MODE) {
		int speed = 0;
		/* iperf3 server start send */
		nlp_server_write   (NlpServerIP, OPT_MSG_TYPE, "start", 0);
		sleep(1);
		speed = iperf3_speed_check (NlpServerIP, OPT_MSG_TYPE);
		sleep(1);
		nlp_server_write   (NlpServerIP, OPT_MSG_TYPE, "stop", 0);
		fprintf (stdout, "%s ipref3 %s mode bandwidth %d Mbits/sec\n",
				NlpServerIP,
				OPT_MSG_TYPE == NLP_SERVER_MSG_TYPE_TCP ? "TCP" : "UDP",
				speed);
		return 0;
	}

	if (OPT_MAC_PRINT) {
		char mac_str[20];
		if (get_my_mac (mac_str)) {
			fprintf (stdout, "Send to Net Printer(%s) %s!! (string : %s)\n",
					NlpServerIP,
					nlp_server_write (NlpServerIP, 0, mac_str, OPT_CHANNEL) ? "ok" : "false",
					mac_str);
		}
		return 0;
	}

	if (OPT_MSG_STR != NULL) {
		char success = 0;

		memset (NlpServerMsg, 0, sizeof(NlpServerMsg));
		strncpy (NlpServerMsg, OPT_MSG_STR, strlen(OPT_MSG_STR));
		success = nlp_server_write (NlpServerIP, OPT_MSG_TYPE, NlpServerMsg, OPT_CHANNEL);
		fprintf (stdout, "Send to Net Printer(%s) %s!! (string : %s)\n",
				NlpServerIP,
				success ? "ok" : "false",
				OPT_MSG_STR);
	} else {
		if (!OPT_MAC_PRINT && !OPT_NLP_SERVER_FIND)
			fprintf (stdout, "error: option is missing.\n-h option is to view help.\n");
	}
	return 0;
}

//------------------------------------------------------------------------------
#endif	// #if defined(D__DEBUG_APP__)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
