%modules = ( # path to module name map
    "QtHttpServer" => "$basedir/src",
);

%moduleheaders = ( # restrict the module headers to those found in relative path
);

%dependencies = (
    "qtbase" => "refs/heads/dev",
);

%classnames = (
    "qhttpserver.h" => "QHttpServer",
    "qthttprequest.h" => "QHttpFileData",
    "qthttprequest.h" => "QHttpRequest",
    "qthttperply.h" => "QHttpReply",
    "qtwebsocket.h" => "QWebSocket"
);
