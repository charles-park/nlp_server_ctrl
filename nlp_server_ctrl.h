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
//	function prototype
//------------------------------------------------------------------------------
extern int get_my_mac	(char *mac_str);
extern int get_my_ip	(char *ip_str);
extern int net_status	(char *ip_addr);

extern int iperf3_speed_check 		(char *ip_addr, char mode);
extern void nlp_server_disconnect	(int nlp_server_fp);
extern int nlp_server_connect		(char *ip_addr);
extern int nlp_server_version 		(char *ip_addr, char *rdver);
extern int nlp_server_version		(int nlp_server_fp, char *rdver);
extern int nlp_server_find			(char *ip_addr);
extern int nlp_server_write         (char *ip_addr, char mtype, char *msg, char ch);

//------------------------------------------------------------------------------
#endif  // __NLP_SERVER_CTRL__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
