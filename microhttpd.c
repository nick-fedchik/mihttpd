#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>

#define PORT 8888	// The port to listen incoming requests

int print_out_key(void *cls, enum MHD_ValueKind kind,
		  const char *key, const char *value)
{
    printf("%s: %s\n", key, value);
    return MHD_YES;
}

int answer_to_connection(void *cls, struct MHD_Connection *connection,
		     const char *url,
		     const char *method, const char *version,
		     const char *upload_data,
		     size_t * upload_data_size, void **con_cls)
{
    const char *page = "<html><body>Hello, browser!</body></html>";
    struct MHD_Response *response;
    const union MHD_ConnectionInfo *conninfo;
    int ret;
    char ipAddress[INET_ADDRSTRLEN];
    struct sockaddr_in *saddr;
    struct passwd *pwe;

/* Get the client IP for the log record */
    conninfo = 	MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    saddr = (struct sockaddr_in *) conninfo->client_addr;
    inet_ntop(AF_INET, &saddr->sin_addr, ipAddress, INET_ADDRSTRLEN);
    printf("From: %s New %s request for %s using version %s\n", ipAddress,
	   method, url, version);

/* 	Here it list all Key:Value pairs from HTTP request */
//    MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_out_key, NULL);

    if (0 != strcmp(method, "GET"))
	return MHD_NO;		/* unexpected method */

/* Here I will use libjson-c to prepare JSON response */
/* get URI path - name or UID ? */

/* if username - struct passwd *getpwnam(const char *name); */
/* assume we get only username now */
struct passwd *pwe = getpwnam(const char *url);
if ( NULL == pwe) {
  /* user not found, we have to return json with error description  */
}
/* otherwise user found, we have to return json with correct user description  */
/* if userid - struct passwd *getpwuid(uid_t uid); */

/* fulfill JSON structure for reply */
/* json_object * json_object_new_object () */

/*
  /*Creating a json object*/
  json_object * jobj = json_object_new_object();

  /*Creating a json string*/
  json_object *jstring = json_object_new_string("Joys of Programming");

  /*Creating a json integer*/
  json_object *jint = json_object_new_int(10);

*/

/* Reply page complete */
    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

int main(void)
{
    struct MHD_Daemon *daemon;
    /* It still not a real Linux daemon, but console program, so I use STDOUT for debugging */
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
			      &answer_to_connection, NULL, MHD_OPTION_END);
    if (NULL == daemon)
	return 1;
    getchar();

    MHD_stop_daemon(daemon);
    return 0;
}
