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

## Implementation Comparison: C vs Go vs Rust

This section analyzes equivalent implementations of the same service in different languages.

### C Implementation (Current - mihttpd)

**Advantages:**
- ✅ **Minimal Dependencies**: Only libmicrohttpd and libjson-c
- ✅ **Tiny Binary**: ~200KB static binary
- ✅ **Low Memory Footprint**: ~2-5MB RAM at runtime
- ✅ **Zero GC Pauses**: Deterministic performance, no garbage collection
- ✅ **Direct System Access**: Direct getpwnam_r/getpwuid_r calls
- ✅ **POSIX Compliance**: Runs on any POSIX system
- ✅ **Educational Value**: Learn low-level security practices

**Disadvantages:**
- ❌ **Manual Memory Management**: Risk of leaks/corruption if not careful
- ❌ **Verbose Code**: More boilerplate for safety checks
- ❌ **Limited Ecosystem**: Need external libraries for common tasks
- ❌ **Slower Development**: More time to implement safely
- ❌ **Type System Limitations**: No sum types, enums are limited
- ❌ **Error Handling**: Manual errno checking, no built-in try/catch
- ❌ **Testing**: Requires external test framework setup

### Go Implementation (Equivalent)

```go
package main

import (
    "encoding/json"
    "fmt"
    "log"
    "net/http"
    "os/user"
    "strconv"
    "strings"
)

func handler(w http.ResponseWriter, r *http.Request) {
    w.Header().Set("Content-Type", "application/json")
    parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/"), "/")

    var u *user.User
    var err error

    if len(parts) == 2 && parts[0] == "name" {
        u, err = user.Lookup(parts[1])
    } else if len(parts) == 2 && parts[0] == "uid" {
        u, err = user.LookupId(parts[1])
    }

    if err != nil {
        json.NewEncoder(w).Encode(map[string]string{"error": "no such user entry"})
        return
    }

    json.NewEncoder(w).Encode(u)
}

func main() {
    http.HandleFunc("/", handler)
    log.Fatal(http.ListenAndServe(":8888", nil))
}
```

**Advantages:**
- ✅ **Automatic Memory Management**: GC handles allocation/deallocation
- ✅ **Concurrency**: Goroutines for easy parallelism
- ✅ **Rich Standard Library**: HTTP, JSON, user lookup built-in
- ✅ **Fast Development**: Simple, readable code
- ✅ **Cross-Platform**: Single binary works on Linux/macOS/Windows
- ✅ **Static Binary**: Compiles to single executable
- ✅ **Built-in Testing**: `testing` package included
- ✅ **Error Handling**: Explicit error returns, clear control flow

**Disadvantages:**
- ❌ **GC Overhead**: Unpredictable pause times (10-100ms)
- ❌ **Binary Size**: ~6-8MB for simple program
- ❌ **Memory Usage**: Higher baseline (~20-40MB RAM)
- ❌ **Startup Time**: Slower (100-200ms vs 1-5ms)
- ❌ **Runtime Dependency**: Needs Go runtime libraries
- ❌ **Not MISRA Safe**: No formal safety compliance
- ❌ **Learning Curve**: Different paradigm (concurrency model)

### Rust Implementation (Equivalent)

```rust
use actix_web::{web, App, HttpServer, HttpResponse};
use serde::{Deserialize, Serialize};
use users::{get_user_by_name, get_user_by_uid};

#[derive(Serialize)]
struct User {
    username: String,
    uid: u32,
    gid: u32,
}

#[derive(Serialize)]
struct Error {
    error: String,
}

async fn get_by_name(name: web::Path<String>) -> HttpResponse {
    match get_user_by_name(&name) {
        Some(user) => HttpResponse::Ok().json(User {
            username: user.name().to_string_lossy().into_owned(),
            uid: user.uid(),
            gid: user.primary_group_id(),
        }),
        None => HttpResponse::Ok().json(Error {
            error: "no such user entry".to_string(),
        }),
    }
}

async fn get_by_uid(uid: web::Path<u32>) -> HttpResponse {
    match get_user_by_uid(*uid) {
        Some(user) => HttpResponse::Ok().json(User {
            username: user.name().to_string_lossy().into_owned(),
            uid: user.uid(),
            gid: user.primary_group_id(),
        }),
        None => HttpResponse::Ok().json(Error {
            error: "no such user entry".to_string(),
        }),
    }
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    HttpServer::new(|| {
        App::new()
            .route("/name/{name}", web::get().to(get_by_name))
            .route("/uid/{uid}", web::get().to(get_by_uid))
    })
    .bind("127.0.0.1:8888")?
    .run()
    .await
}
```

**Advantages:**
- ✅ **Memory Safety**: Compile-time safety, no runtime crashes
- ✅ **No GC Pauses**: Deterministic performance (like C)
- ✅ **Zero-Cost Abstractions**: Safe code without performance penalty
- ✅ **Excellent Error Handling**: `Result<T, E>` type forces handling
- ✅ **Pattern Matching**: Elegant control flow
- ✅ **Async/Await**: Modern concurrency model
- ✅ **Strong Type System**: Prevents whole classes of bugs
- ✅ **Binary Size**: Similar to Go (~5-7MB)

**Disadvantages:**
- ❌ **Steep Learning Curve**: Ownership/borrowing concepts
- ❌ **Compile Time**: Slower compilation (10-30s)
- ❌ **Verbose Type Annotations**: More explicit types needed
- ❌ **Ecosystem Immaturity**: Fewer battle-tested libraries than Go
- ❌ **Smaller Community**: Fewer Stack Overflow answers
- ❌ **Dependencies**: Heavier dependency on cargo ecosystem
- ❌ **Runtime Startup**: Similar to Go (slow vs C)

## Comparison Table

| Aspect | C | Go | Rust |
|--------|---|----|----|
| **Memory Safety** | Manual ⚠️ | Automatic ✅ | Compile-time ✅ |
| **GC Pauses** | None ✅ | 10-100ms ❌ | None ✅ |
| **Binary Size** | 200KB ✅ | 6-8MB ❌ | 5-7MB ❌ |
| **Memory Usage** | 2-5MB ✅ | 20-40MB ❌ | 5-15MB ✅ |
| **Development Speed** | Slow ❌ | Fast ✅ | Medium 🟡 |
| **Learning Curve** | Moderate 🟡 | Easy ✅ | Hard ❌ |
| **Concurrency** | Manual ❌ | Easy ✅ | Best ✅ |
| **Error Handling** | Manual ❌ | Explicit ✅ | Type-safe ✅ |
| **Startup Time** | 1-5ms ✅ | 100-200ms ❌ | 100-150ms ❌ |
| **Type System** | Limited 🟡 | Simple ✅ | Strong ✅ |
| **MISRA Compliance** | Possible ✅ | No ❌ | No ❌ |

## Why C for This Project?

1. **Performance-Critical**: Sub-millisecond response times needed
2. **Embedded Deployment**: Minimal resource footprint required
3. **MISRA Safety**: Formal compliance requirements
4. **Educational**: Demonstrates safe C coding practices
5. **Minimal Dependencies**: Runs anywhere with libc
6. **Deterministic**: No GC pauses, predictable behavior

## When to Choose Each?

- **Choose C**: Embedded systems, extreme performance requirements, formal safety compliance
- **Choose Go**: Web services, microservices, rapid prototyping, strong standard library needed
- **Choose Rust**: System tools, high-performance services, memory safety critical, concurrent workloads
