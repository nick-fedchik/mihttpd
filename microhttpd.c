#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <pwd.h>

#define PORT 8888 // The TCP port to listen incoming requests

/* This is a helper function to dump key:value params of HTTP Request header */
int print_key_value(void *cls, enum MHD_ValueKind kind,
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
    struct MHD_Response *response;
    const union MHD_ConnectionInfo *conninfo;
    int ret;
    char ipAddress[INET_ADDRSTRLEN];
    struct sockaddr_in *saddr;
    struct passwd *pwe = NULL; // Our main structure to reply on client's request
    const void *json_reply = NULL;
    const char *n = NULL; // helper - floating pointer in the const url

    /* Get the client IP for the log record */
    conninfo = MHD_get_connection_info(connection,
                                       MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    saddr = (struct sockaddr_in *)conninfo->client_addr;
    inet_ntop(AF_INET, &saddr->sin_addr, ipAddress, INET_ADDRSTRLEN);
    
    // debug (log record)
    printf("%s New %s request with url %s\n", ipAddress, method, url);

    // debug (key:value)
    /* 	Here it list all Key:Value pairs from HTTP request */
    //    MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_key_value, NULL);

    if (0 != strcmp(method, "GET"))
        return MHD_NO; /* unexpected method */

    /* We expect to get in url
    /uid/'num'
    or
    /name/'name'
    */
    n = url; // set n to begin of const url
    /* get URI path - name or UID ? */
    if (0 == strncmp(n, "/name/", 6))
    {
        /* if we got username - struct passwd *getpwnam(const char *name); */
        // printf("URL name request found\n");
        n = n + 6;       // offset to username in url string
        size_t plen = 0; //parameter length
        plen = strlen(n);
        if ((plen > 0) && (plen <= 256))
        {
            // debug
            // printf("name param `%s`, length %li\n", n, (long unsigned int)plen);
            /* assume we get only username now */
            pwe = getpwnam(n);
        }
    }
    else if (0 == strncmp(url, "/uid/", 5))
    {
        /*  __uid_t pw_uid	User ID is an unsigned int  */
        /* if we got uid in url, then call  struct passwd *getpwuid(const char *name); */
        n = n + 5;       // offset to uid in url string
        size_t plen = 0; //parameter length
        plen = strlen(n);
        /* (__uid_t is an unsigned int); 
        Maximum value for a variable of type unsigned int.	4294967295 (0xffffffff)
        and we should get a sting from 1 to 10 digits long prior to conversation.
         */
        if ((plen > 0) && (plen <= 10))
        {
            unsigned long the_uid;
            char *num_end;

            the_uid = strtoul(n, &num_end, 10);
            printf("uid value=%li, n=%p e=%p (%lu)\n", the_uid, n, num_end, num_end - n);
            if (num_end - n > 0) // did we really convert or there is no correct digits?
            {
                if ((the_uid >= 0) && (the_uid < 4294967295)) // check the u_int range
                {
                    // printf("call getpwuid(%li)\n", the_uid); //debug
                    pwe = getpwuid((__uid_t)the_uid);
                }
            }
        }
    }

    /* From this code line we must provide a JSON answer in reply */
    /* Here I will use libjson-c to prepare JSON response 
    both for successful response and for error - it depends on URL parsing
    */
    /* Creating a json object */
    json_object *jobj = json_object_new_object();

    if (NULL == pwe) // in error reply or no getpwXXX happend
    {
        /* user is not found, or uid is not found, or wegot unexpected/incorrect URI - 
        we have to return json with error description.
        Not a lot of details provided (todo) but  please make a correct request for correct reply  */
        json_object_object_add(jobj, "error", json_object_new_string("no such user entry"));
    }
    else
    {
        /* otherwise user is found, we have to return json with correct user description  */
        /* all records are in the struct passwd, lets make in in JSON!  */
        json_object_object_add(jobj, "username",
                               json_object_new_string(pwe->pw_name));
        json_object_object_add(jobj, "uid", json_object_new_int(pwe->pw_uid));
        json_object_object_add(jobj, "gid", json_object_new_int(pwe->pw_gid));
        json_object_object_add(jobj, "gecos", json_object_new_string(pwe->pw_gecos));
        json_object_object_add(jobj, "dir", json_object_new_string(pwe->pw_dir));
        json_object_object_add(jobj, "shell", json_object_new_string(pwe->pw_shell));
    }

    json_reply = json_object_to_json_string(jobj);
    // debug output
    // printf("JSON:\n%s\n\n", (const char *)json_reply);

    /* Reply page complete */
    response = MHD_create_response_from_buffer(strlen(json_reply),
                                               (void *)json_reply,
                                               MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    /* yes, we need to handle ret value of the response here, but ToDo again... */
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
