/*
 * network_defunes.h
 *
 *  Created on: Nov 21, 2017
 *      Author: RChak
 */

#ifndef NETWORK_DEFUNES_H_
#define NETWORK_DEFUNES_H_

//#define POST_REQUEST_URI 		"/post.php"
#define POST_REQUEST_URI        "/webapp_pub/cgh_post.php"
//#define POST_REQUEST_URI        "/senddata.php"
#define POST_DELETE_SQL			"/delete_SQL.php"

								//{ "key": "value",\n\ "key": "value" \n\  }
//#define POST_DATA           	"{\n\"name\":\"xyz\",\n\"address\":\n{\n\"plot#\":12,\n\"street\":\"abc\",\n\"city\":\"ijk\"\n},\n\"age\":30\n}"
#define POST_DATA 				"say=Hi&to=Mom"


#define DELETE_REQUEST_URI 		"/delete"
#define message_brd             "/webapp_pub/CC3200command.php"

#define PUT_REQUEST_URI 		"/put"
#define PUT_DATA            	"PUT request."

#define GET_REQUEST_URI_JSON 	"/get"
#define data_txt				"/data.txt"
#define GET_REQUEST_URI_PAGE	"/data.html"							//*****************************************************
#define GET_REQUEST_URI 		"/get.php?id=goodbye&mode=run"		//the text that follows the ? is the query string
//#define GET_action_page			"/action_page.php?id=action&mode=page"	//action_page.php
#define GET_action_page         "/webapp_pub/CGH.php"

//#define GET_REQUEST_URI 		"/get.php"

//#define HOST_NAME       		"httpbin.org" //"<host name>"
#define HOST_NAME       		"cnktechlabs.com" //"<host name>" 	********************************************************
#define HOST_PORT           	80
#define SL_SSL_CA_CERT_FILE_NAME        "/cert/testcacert.der"

#endif /* NETWORK_DEFUNES_H_ */
