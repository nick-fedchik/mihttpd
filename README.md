# mihttpd
Pet project. Yet another micro http daemon written in C.

# Objective
Lightweigh web service oriented for JavaScript applications.
It looks for two variants of requests and returns /etc/passwd entry records in JSON format.

Two external libraries are used. 

First one is the libmicrohttpd library to quiclky implement trivial HTTP server.

Second is the libjson-c library to create various (maybe very conmplex) structures in JSON.


## Requiest API:
* Method GET, url path **/name/`<username>`**
  
* Method GET, url path **/uid/`<user_id>`**


**`<username>`** is a string up to 255 chars length.

**`<user_id>`** is a string with numbers up to 10 chars length.

By default, the server will receive HTTP requests on **8888** port.

## Sample request:
`$ wget -q -O - http://127.0.0.1:8888/name/user`

or 

`$ wget -q -O - http://127.0.0.1:8888/uid/1000`

If all correct in requiest, the server will return JSON like below:
```javascript
{ "username": "user", "uid": 1000, "gid": 1000 }
```

In other case (any error - no params, invalid uid or username), the server will return JSON like below:
```javascript
{ "error": "no such user entry" }
```
