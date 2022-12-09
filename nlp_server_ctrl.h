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
#ifndef __NLP_SERVER_CTRL__
#define __NLP_SERVER_CTRL__

//------------------------------------------------------------------------------
// Label printer and Iperf3 cmd option
//------------------------------------------------------------------------------
#define	NLP_SERVER_CHANNEL_LEFT     1
#define	NLP_SERVER_CHANNEL_RIGHT    2
#define	NLP_SERVER_MSG_TYPE_MAC     1
#define NLP_SERVER_MSG_TYPE_ERR     2
#define NLP_SERVER_MSG_TYPE_UDP     3
#define NLP_SERVER_MSG_TYPE_TCP     4

//------------------------------------------------------------------------------
//	function prototype
//------------------------------------------------------------------------------
extern int get_my_mac	(char *mac_str);
extern int get_my_ip	(char *ip_str);
extern int net_status	(char *ip_addr);

extern int iperf3_speed_check 		(char *ip_addr, char mode);
extern void nlp_server_disconnect	(int nlp_server_fp);
extern int nlp_server_connect		(char *ip_addr);
extern int nlp_server_version 		(char *ip_addr, char *rdver);
extern int nlp_server_find			(char *ip_addr);
extern int nlp_server_write         (char *ip_addr, char mtype, char *msg, char ch);

//------------------------------------------------------------------------------
#endif  // __NLP_SERVER_CTRL__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
