# mihttpd
Pet project. Yet another micro http daemon written in C.

Objective
Lightweigh web service oriented for JavaScript applications.
It return /etc/passwd entry records in JSON format.

Two libraries are used. First one is libmicrohttpd to quiclky implement HTTP server.
Second one is libjson-c library to create various (maybe very conmplex) structures in JSON.

Requiest API:
URL method GET, path /name/<username>
URL method GET, path /uid/<user_id>

<username> is a string up to 256 chars length.
<user_id> is a string with numbers up to 5 chars length.

by default, the server will receive HTTP requests on 8888 port.
Sample request:
$ wget -q -O - http://127.0.0.1:8888/name/fnm

If all correct in requiest, the server will return JSON like below:
{ "username": "fnm", "uid": 1000, "gid": 1000 }

In other case (any error - no params, invalid uid or username), the server will return JSON like below:
{ "error": "no such user" }
