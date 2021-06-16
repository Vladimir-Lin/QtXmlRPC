#include <qtxmlrpc.h>

char XMLRPC                 [ ] = "CIOS"                      ;
char REQUEST_BEGIN          [ ] = "<?xml version=\"1.0\"?>\r\n"
                                  "<methodCall>"
                                  "<methodName>"              ;
char REQUEST_END_METHODNAME [ ] = "</methodName>\r\n"         ;
char PARAMS_TAG             [ ] = "<params>"                  ;
char PARAMS_ETAG            [ ] = "</params>"                 ;
char PARAM_TAG              [ ] = "<param>"                   ;
char PARAM_ETAG             [ ] =  "</param>"                 ;
char REQUEST_END            [ ] = "</methodCall>\r\n"         ;
char METHODRESPONSE_TAG     [ ] = "<methodResponse>"          ;
char METHODRESPONSE_ETAG    [ ] = "</methodResponse>"         ;
char FAULT_TAG              [ ] = "<fault>"                   ;

N::XmlRpcClient:: XmlRpcClient ( const char * h , int p , const char * u )
                : XmlRpcSource (                                         )
{
  #ifdef XMLRPC_DEBUG
  log                                               (
    10                                              ,
    "XmlRpcClient new client: host %s, port %d."    ,
    h                                               ,
    p                                             ) ;
  #endif
  host = QString::fromUtf8(h)                       ;
  port = p                                          ;
  if ( NotNull ( u ) ) uri = u ; else uri = "/RPC2" ;
  connectionState            = NO_CONNECTION        ;
  executing                  = false                ;
  eof                        = false                ;
  Variables [ "Offline"    ] =  5000                ;
  Variables [ "Requesting" ] = 30000                ;
  Variables [ "Responsing" ] = 90000                ;
  setKeepOpen ( true  )                             ;
}

N::XmlRpcClient:: XmlRpcClient ( QString h , int p , QString u )
                : XmlRpcSource (                               )
{
  #ifdef XMLRPC_DEBUG
  log                                                    (
    10                                                   ,
    "XmlRpcClient new client: host %s, port %d."         ,
    h . toUtf8 ( ) . constData ( )                       ,
    p                                                  ) ;
  #endif
  host = h                                               ;
  port = p                                               ;
  if ( u . length ( ) > 0 ) uri = u ; else uri = "/RPC2" ;
  connectionState            = NO_CONNECTION             ;
  executing                  = false                     ;
  eof                        = false                     ;
  Variables [ "Offline"    ] =  5000                     ;
  Variables [ "Requesting" ] = 15000                     ;
  Variables [ "Responsing" ] = 60000                     ;
  setKeepOpen ( true  )                                  ;
}

N::XmlRpcClient::~XmlRpcClient(void)
{
  XmlRpcSource::close ( ) ;
}

void N::XmlRpcClient::close(void)
{
  if ( getKeepOpen ( ) ) return        ;
  #ifdef XMLRPC_DEBUG
  log                                  (
    40                                 ,
    "XmlRpcClient::close(%s): fd (%d)" ,
    host . toUtf8 ( ) . constData (  ) ,
    getfd ( )                        ) ;
  #endif
  connectionState = NO_CONNECTION      ;
  XmlRpcSource::close ( )              ;
}

struct ClearFlagOnExit
{
   ClearFlagOnExit (bool & flag) : _flag(flag) {                }
  ~ClearFlagOnExit (           )               { _flag = false; }
  bool & _flag                                                  ;
} ;

bool N::XmlRpcClient::execute (
        QString       method  ,
        XmlRpcValue & params  ,
        XmlRpcValue & result  )
{
  if ( method . length ( ) <= 0 ) return false                             ;
  return execute ( method . toUtf8 ( ) . constData ( ) , params , result ) ;
}

bool N::XmlRpcClient::execute (
        const char  * method  ,
        XmlRpcValue & params  ,
        XmlRpcValue & result  )
{
  #ifdef XMLRPC_DEBUG
  log                                                            (
    10                                                           ,
    "XmlRpcClient::execute(%s): method %s (connectionState %d)." ,
    host . toUtf8 ( ) . constData (  )                           ,
    method                                                       ,
    connectionState                                            ) ;
  #endif
  ////////////////////////////////////////////////////////////////
  if ( executing ) return false                                  ;
  ClearFlagOnExit cf ( executing )                               ;
  executing               = true                                 ;
  IsFault                 = false                                ;
  eof                     = false                                ;
  flags                   = 0xFFFFFFFF                           ;
  header                  = ""                                   ;
  request                 = ""                                   ;
  response                = ""                                   ;
  Variables [ "DropOut" ] = Variables [ "Requesting" ]           ;
  result                  . clear ( )                            ;
  ////////////////////////////////////////////////////////////////
  if ( ! setupConnection (                 ) ) return false      ;
  if ( ! generateRequest ( method , params ) ) return false      ;
  ////////////////////////////////////////////////////////////////
  Transmit ( )                                                   ;
  if ( connectionState != IDLE    ) return false                 ;
  if ( ! parseResponse ( result ) ) return false                 ;
  ////////////////////////////////////////////////////////////////
  response = ""                                                  ;
  #ifdef XMLRPC_DEBUG
  log                                                            (
    10                                                           ,
    "XmlRpcClient::execute(%s): method %s completed."            ,
    host . toUtf8 ( ) . constData (  )                           ,
    method                                                     ) ;
  #endif
  return true                                                    ;
}

bool N::XmlRpcClient::isFault(void) const
{
  return IsFault ;
}

void N::XmlRpcClient::Transmit(void)
{
  while ( connectionState != IDLE )                                          {
    if ( ( NULL != Controller ) && ( ! (*Controller) ) ) break               ;
    if ( Variables . contains ( "DropOut" ) )                                {
      QDateTime lt = Variables [ "Lastest" ] . toDateTime ( )                ;
      qint64    dt = Variables [ "DropOut" ] . toLongLong ( )                ;
      qint64    ds = lt . msecsTo ( nTimeNow )                               ;
      if ( ds > dt ) break                                                   ;
    }                                                                        ;
    Time::msleep ( 1 )                                                       ;
    //////////////////////////////////////////////////////////////////////////
    int            xfd = getfd ( )                                           ;
    int            nEvents                                                   ;
    fd_set         inFd                                                      ;
    fd_set         outFd                                                     ;
    fd_set         excFd                                                     ;
    struct timeval tv                                                        ;
    //////////////////////////////////////////////////////////////////////////
    if      ( xfd < 0  ) break                                               ;
    FD_ZERO ( &inFd    )                                                     ;
    FD_ZERO ( &outFd   )                                                     ;
    FD_ZERO ( &excFd   )                                                     ;
    if      ( xfd >= 0 )                                                     {
      FD_SET                              ( xfd , &excFd )                   ;
      if ( flags & ReadableEvent ) FD_SET ( xfd , &inFd  )                   ;
      if ( flags & WritableEvent ) FD_SET ( xfd , &outFd )                   ;
    } else break                                                             ;
    //////////////////////////////////////////////////////////////////////////
    tv . tv_sec  = 0                                                         ;
    tv . tv_usec = 100000                                                    ;
    nEvents = ::select ( xfd + 1 , &inFd , &outFd , &excFd , &tv )           ;
    if ( nEvents <  0 ) break                                                ;
    if ( nEvents == 0 ) continue                                             ;
    //////////////////////////////////////////////////////////////////////////
    unsigned int newMask   = (unsigned int) -1                               ;
    bool         processed = false                                           ;
    if ( FD_ISSET ( xfd , &inFd  ) )                                         {
      newMask  &= handleEvent ( ReadableEvent       )                        ;
      processed = true                                                       ;
    } else
    if ( FD_ISSET ( xfd , &outFd ) )                                         {
      newMask  &= handleEvent ( WritableEvent       )                        ;
      processed = true                                                       ;
    }                                                                        ;
    if ( FD_ISSET ( xfd , &excFd ) || ( 0 == newMask ) )                     {
      newMask  &= handleEvent ( Exception           )                        ;
      processed = false                                                      ;
    }                                                                        ;
    newMask     = Processing  ( processed , newMask )                        ;
    if ( 0 == newMask )                                                      {
      XmlRpcClient::close ( )                                                ;
      break                                                                  ;
    } else if ( newMask != (unsigned) -1 )                                   {
      flags = newMask                                                        ;
    }                                                                        ;
  }                                                                          ;
}

unsigned N::XmlRpcClient::handleEvent(unsigned eventType)
{
  if ( connectionState == IDLE ) return WritableEvent                        ;
  if ( eventType == Exception )                                              {
    #ifdef XMLRPC_DEBUG
    if ( ( connectionState == WRITE_REQUEST ) && ( bytesWritten <= 0 ) )     {
      error                                                                  (
        "Error in XmlRpcClient::handleEvent(%s): could not connect to server (%s).",
        host . toUtf8 ( ) . constData (  )                                   ,
        XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )         ) ;
    } else                                                                   {
      error                                                                  (
        "Error in XmlRpcClient::handleEvent(%s): (state %d): %s."            ,
        host . toUtf8 ( ) . constData (  )                                   ,
        connectionState                                                      ,
        XmlRpcSource::getErrorMsg().toUtf8().constData()                   ) ;
    }                                                                        ;
    #endif
    return 0                                                                 ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  if ( connectionState == WRITE_REQUEST )                                    {
    if ( WritableEvent == eventType )                                        {
      if ( ! writeRequest ( ) ) return 0                                     ;
    }                                                                        ;
  }                                                                          ;
  if ( connectionState == READ_HEADER   )                                    {
    if ( ReadableEvent == eventType )                                        {
      if ( ! readHeader   ( ) ) return 0                                     ;
    }                                                                        ;
  }                                                                          ;
  if ( connectionState == READ_RESPONSE )                                    {
    if ( ReadableEvent == eventType )                                        {
      if ( ! readResponse ( ) ) return 0                                     ;
    }                                                                        ;
  }                                                                          ;
  return ( connectionState == WRITE_REQUEST ) ? WritableEvent:ReadableEvent  ;
}

bool N::XmlRpcClient::setupConnection(void)
{
  if ( ( ( NO_CONNECTION != connectionState )            &&
         ( IDLE          != connectionState ) )          ||
           eof                                ) close ( ) ;
  eof = false                                             ;
  if ( connectionState == NO_CONNECTION )                 {
    if ( ! doConnect ( ) ) return false                   ;
  }                                                       ;
  connectionState         = WRITE_REQUEST                 ;
  bytesWritten            = 0                             ;
  Variables [ "Lastest" ] = nTimeNow                      ;
  return true                                             ;
}

bool N::XmlRpcClient::doConnect(void)
{
  connectionState = CONNECTING                                              ;
  int FD = XmlRpcSource::socket ( )                                         ;
  if ( FD < 0 )                                                             {
    #ifdef XMLRPC_DEBUG
    error                                                                   (
      "Error in XmlRpcClient::doConnect(%s): Could not create socket (%s)." ,
      host . toUtf8 ( ) . constData (  )                                    ,
      XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )          ) ;
    #endif
    return false                                                            ;
  }                                                                         ;
  ///////////////////////////////////////////////////////////////////////////
  #ifdef XMLRPC_DEBUG
  log                                                                       (
    30                                                                      ,
    "XmlRpcClient::doConnect(%s): fd (%d)."                                 ,
    host . toUtf8 ( ) . constData (  )                                      ,
    FD                                                                    ) ;
  #endif
  setfd ( FD )                                                              ;
  ///////////////////////////////////////////////////////////////////////////
  if ( ! XmlRpcSource::setNonBlocking ( FD ) )                              {
    close ( )                                                               ;
    #ifdef XMLRPC_DEBUG
    error                                                                   (
      "Error in XmlRpcClient::doConnect(%s): Could not set socket to non-blocking IO mode (%s).",
      host . toUtf8 ( ) . constData (  )                                    ,
      XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )          ) ;
    #endif
    return false                                                            ;
  }                                                                         ;
  ///////////////////////////////////////////////////////////////////////////
  if (!XmlRpcSource::connect ( FD , host , port ) )                         {
    close ( )                                                               ;
    #ifdef XMLRPC_DEBUG
    error                                                                   (
      "Error in XmlRpcClient::doConnect(%s): Could not connect to server (%s)." ,
      host . toUtf8 ( ) . constData (  )                                    ,
      XmlRpcSource::getErrorMsg().toUtf8().constData()                    ) ;
    #endif
    return false                                                            ;
  }                                                                         ;
  ///////////////////////////////////////////////////////////////////////////
  #ifdef XMLRPC_DEBUG
  log                                                                       (
    30                                                                      ,
    "XmlRpcClient::doConnect(%s): %d"                                       ,
    host . toUtf8 ( ) . constData ( )                                       ,
    port                                                                  ) ;
  #endif
  ///////////////////////////////////////////////////////////////////////////
  return true                                                               ;
}

bool N::XmlRpcClient::generateRequest(const char * methodName,XmlRpcValue & params)
{
  if ( NULL == methodName ) return false                  ;
  /////////////////////////////////////////////////////////
  QString body                                            ;
  QString header                                          ;
  body  = QString::fromUtf8 ( REQUEST_BEGIN          )    ;
  body += QString::fromUtf8 ( methodName             )    ;
  body += QString::fromUtf8 ( REQUEST_END_METHODNAME )    ;
  /////////////////////////////////////////////////////////
  if ( params . valid ( ) )                               {
    body += QString::fromUtf8 ( PARAMS_TAG )              ;
    if ( params . getType ( ) == XmlRpcValue::TypeArray ) {
      for (int i = 0 ; i < params . size ( ) ; ++i )      {
        body += QString::fromUtf8    ( PARAM_TAG   )      ;
        body += params [ i ] . toXml (             )      ;
        body += QString::fromUtf8    ( PARAM_ETAG  )      ;
      }                                                   ;
    } else                                                {
      body   += QString::fromUtf8    ( PARAM_TAG   )      ;
      body   += params . toXml       (             )      ;
      body   += QString::fromUtf8    ( PARAM_ETAG  )      ;
    }                                                     ;
    body     += QString::fromUtf8    ( PARAMS_ETAG )      ;
  }                                                       ;
  /////////////////////////////////////////////////////////
  body       += QString::fromUtf8    ( REQUEST_END )      ;
  /////////////////////////////////////////////////////////
  header  = generateHeader ( body )                       ;
  request = header + body                                 ;
  #ifdef XMLRPC_DEBUG
  log                                                     (
    40                                                    ,
    "XmlRpcClient::generateRequest(%s): header is %d bytes, content-length is %d.",
    host   . toUtf8 ( ) . constData ( )                   ,
    header . length ( )                                   ,
    body   . length ( )                                 ) ;
  #endif
  /////////////////////////////////////////////////////////
  return true                                             ;
}

QString N::XmlRpcClient::generateHeader(const QString & body)
{
  qint64  bsize = body . size ( )                                  ;
  QString header                                                   ;
  header    = "POST "                                              ;
  header   += uri                                                  ;
  header   += " HTTP/1.1"                                          ;
  header   += "\r\n"                                               ;
  header   += "User-Agent: "                                       ;
  header   += QString::fromUtf8 ( XMLRPC )                         ;
  header   += "\r\n"                                               ;
  header   += "Host: "                                             ;
  header   += host                                                 ;
  header   += QString ( ":%1\r\n" ) . arg ( port )                 ;
  header   += "Content-Type: text/xml\r\n"                         ;
  header   += QString ( "Content-length: %1\r\n" ) . arg ( bsize ) ;
  if ( getKeepOpen ( ) )                                           {
    header   += "Connection: Keep-Alive\r\n"                       ;
  } else                                                           {
    header   += "Connection: Close\r\n"                            ;
  }                                                                ;
  header   += "\r\n"                                               ;
  return header                                                    ;
}

bool N::XmlRpcClient::writeRequest(void)
{
  QByteArray R = request . toUtf8 ( )                              ;
  if ( ! XmlRpcSource::nbWrite ( getfd ( ) , R , &bytesWritten ) ) {
    #ifdef XMLRPC_DEBUG
    error                                                          (
      "Error in XmlRpcClient::writeRequest(%s): write error (%s)." ,
      host . toUtf8 ( ) . constData ( )                            ,
      XmlRpcSource::getErrorMsg().toUtf8().constData()           ) ;
    #endif
    return false                                                   ;
  }                                                                ;
  Variables [ "Lastest" ] = nTimeNow                               ;
  //////////////////////////////////////////////////////////////////
  #ifdef XMLRPC_DEBUG
  log                                                              (
    30                                                             ,
    "XmlRpcClient::writeRequest(%s): wrote %d of %d bytes."        ,
    host . toUtf8 ( ) . constData ( )                              ,
    bytesWritten                                                   ,
    request . length ( )                                         ) ;
  #endif
  //////////////////////////////////////////////////////////////////
  if ( bytesWritten >= R . size ( ) )                              {
    header                  = ""                                   ;
    response                = ""                                   ;
    request                 = ""                                   ;
    connectionState         = READ_HEADER                          ;
    bytesWritten            = 0                                    ;
    Variables [ "DropOut" ] = Variables [ "Responsing" ]           ;
  }                                                                ;
  //////////////////////////////////////////////////////////////////
  return true                                                      ;
}

bool N::XmlRpcClient::readHeader(void)
{
  QByteArray H                                                               ;
  eof = false                                                                ;
  if ( ( ! XmlRpcSource::nbRead ( getfd ( ) , H , &eof ) )                  ||
       ( eof && ( H . size ( ) == 0 ) )                                    ) {
    #ifdef XMLRPC_DEBUG
    error                                                                    (
      "Error in XmlRpcClient::readHeader(%s): error while reading header (%s) on fd %d.",
      host . toUtf8 ( ) . constData ( )                                      ,
      XmlRpcSource::getErrorMsg().toUtf8().constData()                     ) ;
    #endif
    return false                                                             ;
  }                                                                          ;
  if ( H . size ( ) > 0 ) header += QString::fromUtf8 ( H )                  ;
  ////////////////////////////////////////////////////////////////////////////
  if ( header . length ( ) <= 0         ) return true                        ;
  Variables [ "Lastest" ] = nTimeNow                                         ;
  bool splitted = false                                                      ;
  if ( header . contains ( "\r\n\r\n" ) ) splitted = true                    ;
  if ( header . contains ( "\n\n"     ) ) splitted = true                    ;
  if ( ! splitted                       ) return true                        ;
  #ifdef XMLRPC_DEBUG
  log                                                                        (
    40                                                                       ,
    "XmlRpcClient::readHeader(%s): client has read %d bytes"                 ,
    host   . toUtf8 ( ) . constData ( )                                      ,
    header . length ( )                                                    ) ;
  #endif
  ////////////////////////////////////////////////////////////////////////////
  int  ll  = header . length ( )                                             ;
  int  bp  = -1                                                              ;
  int  lp  = -1                                                              ;
  ////////////////////////////////////////////////////////////////////////////
  if   ( header . contains ( "\r\n\r\n" ) )                                  {
    bp = header . indexOf  ( "\r\n\r\n" ) + 4                                ;
  } else
  if   ( header . contains ( "\n\n"     ) )                                  {
    bp = header . indexOf  ( "\n\n"     ) + 2                                ;
  }                                                                          ;
  lp = header . indexOf ( "Content-length: " )                               ;
  ////////////////////////////////////////////////////////////////////////////
  if ( bp < 0 )                                                              {
    if ( eof )                                                               {
      #ifdef XMLRPC_DEBUG
      error                                                                  (
        "Error in XmlRpcClient::readHeader(%s): EOF while reading header"    ,
        host . toUtf8 ( ) . constData ( )                                  ) ;
      #endif
      return false                                                           ;
    }                                                                        ;
    return true                                                              ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  if ( lp < 0 )                                                              {
    #ifdef XMLRPC_DEBUG
    error                                                                    (
      "Error XmlRpcClient::readHeader(%s): No Content-length specified"      ,
      host   . toUtf8 ( ) . constData ( )                                  ) ;
    #endif
    return false                                                             ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  QString CL                                                                 ;
  int     zp                                                                 ;
  zp = header . indexOf ( "\n" , lp + 16 )                                   ;
  if ( zp              <  0 ) return true                                    ;
  CL = header . mid     ( lp + 16 , zp - lp - 16 )                           ;
  CL = CL     . replace ( "\r" , ""              )                           ;
  CL = CL     . replace ( "\n" , ""              )                           ;
  if ( CL . length ( ) <= 0 ) return true                                    ;
  contentLength = CL . toInt ( )                                             ;
  if ( contentLength <= 0 )                                                  {
    #ifdef XMLRPC_DEBUG
    error                                                                    (
      "Error in XmlRpcClient::readHeader(%s): Invalid Content-length specified (%d)." ,
      host     . toUtf8 ( ) . constData ( )                                  ,
      contentLength                                                        ) ;
    #endif
    return false                                                             ;
  }                                                                          ;
  #ifdef XMLRPC_DEBUG
  log                                                                        (
    40                                                                       ,
    "XmlRpcClient::readHeader(%s): client read content length: %d"           ,
    host     . toUtf8 ( ) . constData ( )                                    ,
    contentLength                                                          ) ;
  #endif
  ////////////////////////////////////////////////////////////////////////////
  response                = header . mid ( bp , header . length ( ) - bp )   ;
  connectionState         = READ_RESPONSE                                    ;
  header                  = ""                                               ;
  Variables [ "Lastest" ] = nTimeNow                                         ;
  ////////////////////////////////////////////////////////////////////////////
  return true                                                                ;
}

bool N::XmlRpcClient::readResponse(void)
{
  eof = false                                                        ;
  if ( response . length ( )  < contentLength )                      {
    QByteArray R                                                     ;
    if ( ! XmlRpcSource::nbRead ( getfd ( ) , R , &eof ) )           {
      #ifdef XMLRPC_DEBUG
      error                                                          (
        "Error in XmlRpcClient::readResponse(%s): read error (%s)."  ,
        host     . toUtf8 ( ) . constData ( )                        ,
        XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( ) ) ;
      #endif
      return false                                                   ;
    }                                                                ;
    if ( R . size ( ) > 0 ) response += QString::fromUtf8 ( R )      ;
    if ( response . length ( )  < contentLength )                    {
      if ( eof )                                                     {
        #ifdef XMLRPC_DEBUG
        error                                                        (
          "Error in XmlRpcClient::readResponse(%s): EOF while reading response" ,
          host     . toUtf8 ( ) . constData ( )                    ) ;
        #endif
        return false                                                 ;
      }                                                              ;
      return true                                                    ;
    }                                                                ;
  }                                                                  ;
  ////////////////////////////////////////////////////////////////////
  #ifdef XMLRPC_DEBUG
  log                                                                (
    30                                                               ,
    "XmlRpcClient::readResponse(%s): (read %d bytes)"                ,
    host     . toUtf8 ( ) . constData ( )                            ,
    response . length ( )                                          ) ;
  #ifdef VERY_DEEP_DEBUG
  log                                                                (
    50                                                               ,
    "response(%s):\n%s"                                              ,
    host     . toUtf8 ( ) . constData ( )                            ,
    response . toUtf8 ( ) . constData ( )                          ) ;
  #endif
  #endif
  ////////////////////////////////////////////////////////////////////
  connectionState         = IDLE                                     ;
  Variables [ "Lastest" ] = nTimeNow                                 ;
  ////////////////////////////////////////////////////////////////////
  return false                                                       ;
}

bool N::XmlRpcClient::parseResponse(XmlRpcValue & result)
{
  int offset = 0                                                             ;
  if ( ! findTag ( METHODRESPONSE_TAG , response , &offset ) )               {
    #ifdef XMLRPC_DEBUG
    error                                                                    (
      "Error in XmlRpcClient::parseResponse(%s): Invalid response - no methodResponse. Response:\n%s",
      host     . toUtf8 ( ) . constData ( )                                  ,
      response . toUtf8 ( ) . constData ( )                                ) ;
    #endif
    return false                                                             ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  bool correct = false                                                       ;
  if   ( nextTagIs ( PARAMS_TAG , response , &offset ) )                     {
    if ( nextTagIs ( PARAM_TAG  , response , &offset ) )                     {
      correct = true                                                         ;
    }                                                                        ;
  } else                                                                     {
    if ( isFault ( ) )                                                       {
      if ( nextTagIs ( FAULT_TAG  , response , &offset ) )                   {
        correct = true                                                       ;
      }                                                                      ;
    }                                                                        ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  if ( correct )                                                             {
    if ( ! result . fromXml ( response , &offset ) )                         {
      #ifdef XMLRPC_DEBUG
      error                                                                  (
        "Error in XmlRpcClient::parseResponse(%s): Invalid response value. Response:\n%s",
        host     . toUtf8 ( ) . constData ( )                                ,
        response . toUtf8 ( ) . constData ( )                              ) ;
      #endif
      response = ""                                                          ;
      return false                                                           ;
    }                                                                        ;
  } else                                                                     {
    #ifdef XMLRPC_DEBUG
    error                                                                    (
      "Error in XmlRpcClient::parseResponse(%s): Invalid response - no param or fault tag. Response:\n%s",
      host     . toUtf8 ( ) . constData ( )                                  ,
      response . toUtf8 ( ) . constData ( )                                ) ;
    #endif
    response = ""                                                            ;
    return false                                                             ;
  }                                                                          ;
  response = ""                                                              ;
  return result . valid ( )                                                  ;
}
