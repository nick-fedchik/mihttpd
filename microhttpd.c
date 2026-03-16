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
#include <stdint.h>
#include <errno.h>
#include <time.h>

#define PORT 8888 /* The TCP port to listen incoming requests */
#define MAX_IP_LEN 16
#define MAX_USERNAME_LEN 256
#define MAX_UID_LEN 10
#define UINT_MAX_VAL 4294967295U
#define DEBUG_LOGGING 0 /* Set to 1 to enable debug logging, 0 to disable */
#define MAX_REQUESTS_PER_SECOND 100 /* Basic rate limiting threshold */

/* Rate limiting state: track requests per IP */
static struct {
    const char *ip;
    uint32_t request_count;
    time_t last_reset;
} rate_limit_state = { NULL, 0, 0 };

/* Helper function to check rate limiting */
static int32_t check_rate_limit(const char *client_ip)
{
    time_t now = time(NULL);

    /* Reset counter every second */
    if (now != rate_limit_state.last_reset)
    {
        rate_limit_state.request_count = 0;
        rate_limit_state.last_reset = now;
    }

    rate_limit_state.request_count++;

    /* Reject if exceeds threshold */
    if (rate_limit_state.request_count > MAX_REQUESTS_PER_SECOND)
    {
        return 0; /* Rate limit exceeded */
    }

    return 1; /* Request allowed */
}

/* This is a helper function to dump key:value params of HTTP Request header */
int print_key_value(void *cls, enum MHD_ValueKind kind,
                  const char *key, const char *value)
{
    printf("%s: %s\n", key, value);
    return MHD_YES;
}

int32_t answer_to_connection(void *cls, struct MHD_Connection *connection,
                         const char *url,
                         const char *method, const char *version,
                         const char *upload_data,
                         size_t *upload_data_size, void **con_cls)
{
    struct MHD_Response *response = NULL;
    const union MHD_ConnectionInfo *conninfo = NULL;
    int32_t ret = MHD_NO;
    char ipAddress[MAX_IP_LEN];
    struct sockaddr_in *saddr = NULL;
    struct passwd *pwe = NULL; /* User structure from getpw* call */
    const void *json_reply = NULL;
    const char *n = NULL; /* Helper pointer for URL parsing */

    /* Get the client IP for the log record */
    conninfo = MHD_get_connection_info(connection,
                                       MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    if (NULL == conninfo || NULL == conninfo->client_addr)
    {
        strncpy(ipAddress, "unknown", MAX_IP_LEN - 1);
        ipAddress[MAX_IP_LEN - 1] = '\0';
    }
    else
    {
        saddr = (struct sockaddr_in *)conninfo->client_addr;
        if (NULL == inet_ntop(AF_INET, &saddr->sin_addr, ipAddress, MAX_IP_LEN))
        {
            strncpy(ipAddress, "unknown", MAX_IP_LEN - 1);
            ipAddress[MAX_IP_LEN - 1] = '\0';
        }
    }
    
    /* Check rate limiting */
    if (0 == check_rate_limit(ipAddress))
    {
        return MHD_NO; /* Rate limit exceeded, reject request */
    }

    /* Conditional debug logging - disable in production */
#if DEBUG_LOGGING
    printf("%s New %s request with url %s\n", ipAddress, method, url);
    /* List all key:value pairs from HTTP request */
    /*  MHD_get_connection_values (connection, MHD_HEADER_KIND, &print_key_value, NULL); */
#endif

    if (0 != strcmp(method, "GET"))
        return MHD_NO; /* unexpected method */

    /* Validate URL length - prevent DoS via huge URLs */
    if (strlen(url) > 512)
        return MHD_NO; /* URL too long */

    /* Check for path traversal attempts */
    if (NULL != strstr(url, ".."))
        return MHD_NO; /* path traversal attempt detected */

    /* Parse URI path: /uid/num or /name/name */
    n = url; /* set n to begin of URL string */

    if (0 == strncmp(n, "/name/", 6))
    {
        /* Process username lookup via getpwnam_r() - thread-safe version */
        const char *username_start = n + 6; /* offset to username in URL string */
        char decoded_username[MAX_USERNAME_LEN];
        size_t plen = strlen(username_start);

        if ((plen > 0) && (plen <= MAX_USERNAME_LEN - 1))
        {
            /* URL decode the username (handles %2f, %20, etc.) */
            size_t decoded_len = MHD_http_unescape(username_start);
            strncpy(decoded_username, username_start, decoded_len);
            decoded_username[decoded_len] = '\0';

            struct passwd pwd_buffer;
            struct passwd *result = NULL;
            char pwbuf[1024];

            if (0 == getpwnam_r(decoded_username, &pwd_buffer, pwbuf, sizeof(pwbuf), &result))
            {
                pwe = result;
            }
        }
    }
    else if (0 == strncmp(n, "/uid/", 5))
    {
        /* Process UID lookup via getpwuid_r() - thread-safe version */
        const char *uid_start = n + 5; /* offset to UID in URL string */
        char decoded_uid[MAX_UID_LEN];
        size_t plen = strlen(uid_start);

        if ((plen > 0) && (plen <= MAX_UID_LEN - 1))
        {
            /* URL decode the UID string (handles %20, etc.) */
            size_t decoded_len = MHD_http_unescape(uid_start);
            strncpy(decoded_uid, uid_start, decoded_len);
            decoded_uid[decoded_len] = '\0';

            uint32_t the_uid;
            char *num_end = NULL;
            int32_t conversion_len;

            the_uid = (uint32_t)strtoul(decoded_uid, &num_end, 10);

            if (NULL != num_end)
            {
                conversion_len = (int32_t)(num_end - decoded_uid);
                if (conversion_len > 0)
                {
                    if (the_uid <= UINT_MAX_VAL)
                    {
                        struct passwd pwd_buffer;
                        struct passwd *result = NULL;
                        char pwbuf[1024];

                        if (0 == getpwuid_r((__uid_t)the_uid, &pwd_buffer, pwbuf,
                                           sizeof(pwbuf), &result))
                        {
                            pwe = result;
                        }
                    }
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

    if (NULL == jobj)
    {
        return MHD_NO; /* JSON object creation failed */
    }

    if (NULL == pwe) /* in error reply or no getpwXXX happened */
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
    /* Debug output: printf("JSON:\n%s\n\n", (const char *)json_reply); */

    /* Reply page complete */
    char *json_str = strdup(json_reply);
    json_object_put(jobj); // Free JSON object after converting to string

    if (NULL == json_str)
    {
        return MHD_NO; /* Memory allocation failed */
    }

    response = MHD_create_response_from_buffer(strlen(json_str),
                                               (void *)json_str,
                                               MHD_RESPMEM_MUST_FREE);
    if (NULL == response)
    {
        free(json_str);
        return MHD_NO;
    }

    MHD_add_response_header(response, "Content-Type", "application/json");

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
