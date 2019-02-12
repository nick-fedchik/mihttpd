#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <pwd.h>

#define PORT 8888 // The port to listen incoming requests

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
                         size_t *upload_data_size, void **con_cls)
{
    //   const char *page = "<html><body>Hello, browser!</body></html>";
    struct MHD_Response *response;
    const union MHD_ConnectionInfo *conninfo;
    int ret;
    char ipAddress[INET_ADDRSTRLEN];
    struct sockaddr_in *saddr;
    struct passwd *pwe = NULL;
    const char *stubname = "fnm";
    const void *json_reply = NULL;

    /* Get the client IP for the log record */
    conninfo = MHD_get_connection_info(connection,
                                       MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    saddr = (struct sockaddr_in *)conninfo->client_addr;
    inet_ntop(AF_INET, &saddr->sin_addr, ipAddress, INET_ADDRSTRLEN);
    // debug
    printf("From: %s New %s request for %s using version %s\n", ipAddress,
           method, url, version);

    /* 	Here it list all Key:Value pairs from HTTP request */
    //    MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_out_key, NULL);

    if (0 != strcmp(method, "GET"))
        return MHD_NO; /* unexpected method */

    /* We expect to get
    /uid/'num'
    or
    /name/'name'
*/

    /* Here I will use libjson-c to prepare JSON response 
    both for successful response or error - it depends on URL parsing
 */
    /*Creating a json object */
    json_object *jobj = json_object_new_object();

    /* get URI path - name or UID ? */
    /* The stub with username will be used prior to real parsing, 
   so any URL could be given now, I use my login name.
*/
    const char * urlparam = NULL;
    /* if username - struct passwd *getpwnam(const char *name); */
    if (0 == strncmp(url, "/name", 5))
    {
        /* max username length is 256 */
        char *n = NULL;
        urlparam = url + 4; // ptr to end of /name
        sscanf(urlparam, "%s/", n);
        printf("> %s <\n",n);
        /* assume we get only username now */
        pwe = getpwnam(stubname);
    }
    else if (0 == strncmp(url, "/uid", 4))
    {
        /*  __uid_t pw_uid	User ID is an unsigned int  */
        urlparam = url + 3; // ptr to end of /uid
        /* scanf string to num */
        /* assume we get only uid 1000  */
        pwe = getpwuid(1000);
    }
    if (NULL == pwe)
    {
        /* user not found, or uid not found, or unexpected URI - we have to return json with error description  */
        json_object_object_add(jobj, "error", json_object_new_string("no such user"));
    }
    else
    {
        /* otherwise user found, we have to return json with correct user description  */
        /* if userid - struct passwd *getpwuid(uid_t uid); */

        json_object_object_add(jobj, "username",
                               json_object_new_string(pwe->pw_name));
        json_object_object_add(jobj, "uid", json_object_new_int(pwe->pw_uid));
        json_object_object_add(jobj, "gid", json_object_new_int(pwe->pw_gid));
    }
    
    json_reply = json_object_to_json_string(jobj);
    // debug output
    printf("JSON:\n%s\n\n", (const char *)json_reply);

    /* Reply page complete */
    response = MHD_create_response_from_buffer(strlen(json_reply),
                                               /*(void *) page, */
                                               (void *)json_reply,
                                               MHD_RESPMEM_PERSISTENT);
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
