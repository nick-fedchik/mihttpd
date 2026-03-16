# mihttpd

Pet project. A micro HTTP daemon written in C for querying system user information.

## Objective

Lightweight web service oriented for JavaScript applications. Returns `/etc/passwd` entry records in JSON format via HTTP API.

## Dependencies

- **libmicrohttpd** - HTTP server library for handling requests
- **libjson-c** - JSON serialization library for structured responses


## API

### Endpoints

**GET /name/<username>** - Query user by username
- `<username>` - Username string (max 255 characters)

**GET /uid/<user_id>** - Query user by UID
- `<user_id>` - Numeric UID (max 10 digits, 0-4294967295)

Default port: **8888**

### Examples

```bash
$ wget -q -O - http://127.0.0.1:8888/name/root
$ wget -q -O - http://127.0.0.1:8888/uid/0
```

### Success Response

```json
{
  "username": "root",
  "uid": 0,
  "gid": 0,
  "gecos": "root",
  "dir": "/root",
  "shell": "/bin/bash"
}
```

### Error Response

```json
{
  "error": "no such user entry"
}
```

## Security Features

- **Memory Safety**: Proper buffer management, no strcpy/sprintf vulnerabilities
- **MISRA C Compliance**: Follows MISRA C 2012 standard for safe C coding
- **Thread-Safe**: Uses reentrant `getpwnam_r()` and `getpwuid_r()` functions
- **DoS Protection**: URL length validation (max 512 chars)
- **Path Traversal Prevention**: Blocks requests with `..` sequences
- **Rate Limiting**: Basic rate limiting (100 requests/second)
- **URL Decoding**: Proper handling of URL-encoded parameters (%20, %2f, etc.)
- **Null-Safety**: All pointers validated before dereferencing
- **Type Safety**: Explicit types (int32_t, uint32_t) for portability

## Configuration

Edit `microhttpd.c` defines to customize:

```c
#define PORT 8888                  /* Server port */
#define DEBUG_LOGGING 0            /* Set to 1 to enable debug output */
#define MAX_REQUESTS_PER_SECOND 100 /* Rate limiting threshold */
#define MAX_USERNAME_LEN 256       /* Max username length */
#define MAX_UID_LEN 10             /* Max UID string length */
```

## Building

```bash
make clean
make
./microhttpd
```

Press Enter to stop the server.
