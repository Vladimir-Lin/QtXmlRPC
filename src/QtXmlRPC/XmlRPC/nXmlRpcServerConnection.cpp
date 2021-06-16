#include <qtxmlrpc.h>

char        METHODNAME_TAG  [ ] = "<methodName>"  ;
char        METHODNAME_ETAG [ ] = "</methodName>" ;
extern char PARAMS_TAG      [ ]                   ;
extern char PARAMS_ETAG     [ ]                   ;
extern char PARAM_TAG       [ ]                   ;
extern char PARAM_ETAG      [ ]                   ;
extern char XMLRPC          [ ]                   ;

const QString SYSTEM_MULTICALL = QString ( "system.multicall"             ) ;
const QString METHODNAME       = QString ( "methodName"                   ) ;
const QString PARAMS           = QString ( "params"                       ) ;
const QString FAULTCODE        = QString ( "faultCode"                    ) ;
const QString FAULTSTRING      = QString ( "faultString"                  ) ;
const QString FaultRESPONSE_1  = QString ( "<?xml version=\"1.0\"?>"
                                           "\r\n"
                                           "<methodResponse><fault>"
                                           "\r\n"                         ) ;
const QString FaultRESPONSE_2  = QString ( "\r\n"
                                           "</fault></methodResponse>"
                                           "\r\n"                        ) ;
const QString HeaderRESPONSE_1 = QString ( "<?xml version=\"1.0\"?>"
                                           "\r\n"
                                           "<methodResponse><params><param>"
                                           "\r\n"                        ) ;
const QString HeaderRESPONSE_2 = QString ( "\r\n"
                                           "</param></params></methodResponse>"
                                           "\r\n"                        ) ;

N::XmlRpcServerConnection:: XmlRpcServerConnection ( int FD , XmlRpcServer * server )
                          : XmlRpcSource           (     FD                         )
                          , Server                 (                         server )
                          , ConnectionState        ( READ_HEADER                    )
                          , keepAlive              ( true                           )
{
  #ifdef XMLRPC_DEBUG
  log ( 20 , "XmlRpcServerConnection: new socket %d." , FD ) ;
  #endif
  Variables [ "Offline" ] = 5000                             ;
  Variables [ "Expire"  ] = 30000                            ;
  Variables [ "Lastest" ] = nTimeNow                         ;
  setKeepOpen ( true  )                                      ;
}

N::XmlRpcServerConnection::~XmlRpcServerConnection(void)
{
  #ifdef XMLRPC_DEBUG
  log ( 40 , "XmlRpcServerConnection destructor." ) ;
  #endif
  if ( NotNull ( Server ) )                         {
    Server -> removeConnection ( this )             ;
  }                                                 ;
  XmlRpcSource::close ( )                           ;
}

unsigned N::XmlRpcServerConnection::handleEvent(unsigned eventType)
{
  if ( eventType == Exception )                                         {
    #ifdef XMLRPC_DEBUG
    if ( ( ConnectionState == WRITE_RESPONSE ) && ( 0 == bytesWritten ) )
      error                                                 (
        "Error in XmlRpcServerConnection::handleEvent: could not write to client (%s).",
        XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )    ) ;
    else
      error                                                             (
        "Error in XmlRpcServerConnection::handleEvent (state %d): %s."  ,
        ConnectionState                                                 ,
        XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )    ) ;
    #endif
    setKeepOpen ( false )                                               ;
    return 0                                                            ;
  }                                                                     ;
  ///////////////////////////////////////////////////////////////////////
  if ( ConnectionState == READ_HEADER    )                              {
    if ( ReadableEvent == eventType )                                   {
      if ( ! readHeader    ( ) ) return 0                               ;
    }                                                                   ;
  }                                                                     ;
  if ( ConnectionState == READ_REQUEST   )                              {
    if ( ReadableEvent == eventType )                                   {
      if ( ! readRequest   ( ) ) return 0                               ;
    }                                                                   ;
  }                                                                     ;
  if ( ConnectionState == WRITE_RESPONSE )                              {
    if ( WritableEvent == eventType )                                   {
      if ( ! writeResponse ( ) ) return 0                               ;
    }                                                                   ;
  }                                                                     ;
  return ( ConnectionState == WRITE_RESPONSE ) ? WritableEvent          :
                                                 ReadableEvent          ;
}

unsigned N::XmlRpcServerConnection::Processing(bool processed,unsigned newMask)
{
  if ( ( newMask > 0 ) && processed )                         {
    Variables [ "Lastest" ] = nTimeNow                        ;
    return newMask                                            ;
  }                                                           ;
  /////////////////////////////////////////////////////////////
  qint64    expire = Variables [ "Expire"  ] . toLongLong ( ) ;
  QDateTime LT     = Variables [ "Lastest" ] . toDateTime ( ) ;
  qint64    dt     = LT . msecsTo ( nTimeNow )                ;
  if ( dt < expire ) return newMask                           ;
  /////////////////////////////////////////////////////////////
  setKeepOpen ( false )                                       ;
  return 0                                                    ;
}

bool N::XmlRpcServerConnection::readHeader(void)
{
  bool       eof                                            ;
  QByteArray H                                              ;
  if ( ! XmlRpcSource::nbRead ( getfd ( ) , H , &eof ) )    {
    #ifdef XMLRPC_DEBUG
    if ( header . length ( ) > 0 )                          {
      error                                                 (
        "XmlRpcServerConnection::readHeader: error while reading header (%s).",
        XmlRpcSource::getErrorMsg().toUtf8().constData() )  ;
    }                                                       ;
    #endif
    return false                                            ;
  }                                                         ;
  if ( H . size ( ) > 0 ) header += QString::fromUtf8 ( H ) ;
  ///////////////////////////////////////////////////////////
  if ( header . length ( ) <= 0         ) return true       ;
  bool splitted = false                                     ;
  if ( header . contains ( "\r\n\r\n" ) ) splitted = true   ;
  if ( header . contains ( "\n\n"     ) ) splitted = true   ;
  if ( ! splitted                       ) return true       ;
  #ifdef XMLRPC_DEBUG
  log                                                       (
    40                                                      ,
    "XmlRpcServerConnection::readHeader: read %d bytes."    ,
    header.length()                                       ) ;
  #endif
  ///////////////////////////////////////////////////////////
  int  ll = header . length ( )                             ;
  int  bp = -1                                              ;
  int  lp = -1                                              ;
  int  kp = -1                                              ;
  bool dx = true                                            ;
  ///////////////////////////////////////////////////////////
  if   ( header . contains ( "\r\n\r\n" ) )                 {
    bp = header . indexOf  ( "\r\n\r\n" ) + 4               ;
  } else
  if   ( header . contains ( "\n\n"     ) )                 {
    bp = header . indexOf  ( "\n\n"     ) + 2               ;
  }                                                         ;
  lp = header . indexOf ( "Content-length: " )              ;
  kp = header . indexOf ( "Connection: "     )              ;
  ///////////////////////////////////////////////////////////
  if ( bp < 0 )                                             {
    if ( eof )                                              {
      #ifdef XMLRPC_DEBUG
      log(40,"XmlRpcServerConnection::readHeader: EOF")     ;
      if ( header.length() > 0 )
        error("XmlRpcServerConnection::readHeader: EOF while reading header") ;
      #endif
      return false                                          ;
    }                                                       ;
    return true                                             ;
  }                                                         ;
  ///////////////////////////////////////////////////////////
  if ( lp < 0 )                                             {
    #ifdef XMLRPC_DEBUG
    error("XmlRpcServerConnection::readHeader: No Content-length specified") ;
    #endif
    return false                                            ;
  }                                                         ;
  ///////////////////////////////////////////////////////////
  QString CL                                                ;
  int     zp                                                ;
  zp = header . indexOf ( "\n" , lp + 16 )                  ;
  if ( zp              <  0 ) return true                   ;
  CL = header . mid     ( lp + 16 , zp - lp - 16 )          ;
  CL = CL     . replace ( "\r" , ""              )          ;
  CL = CL     . replace ( "\n" , ""              )          ;
  if ( CL . length ( ) <= 0 ) return true                   ;
  contentLength = CL . toInt ( )                            ;
  if ( contentLength <= 0 )                                 {
    #ifdef XMLRPC_DEBUG
    error("XmlRpcServerConnection::readHeader: Invalid Content-length specified (%d).",contentLength) ;
    #endif
    return false                                            ;
  }                                                         ;
  ///////////////////////////////////////////////////////////
  #ifdef XMLRPC_DEBUG
  log(30,"XmlRpcServerConnection::readHeader: specified content length is %d.",contentLength) ;
  #endif
  ///////////////////////////////////////////////////////////
  request   = header . mid ( bp , ll - bp )                 ;
  keepAlive = true                                          ;
  ///////////////////////////////////////////////////////////
  if ( kp >= 0 )                                            {
    if ( header . indexOf ( "HTTP/1.1" ) >=0 )              {
      QString ka = header . mid ( kp + 12 , 10 )            ;
      ka = ka . toLower ( )                                 ;
      if ( ka != "keep-alive" ) keepAlive = false           ;
    } else                                                  {
      QString kc = header . mid ( kp + 12 , 5 )             ;
      kc = kc . toLower ( )                                 ;
      if ( kc == "close"      ) keepAlive = false           ;
    }                                                       ;
    #ifdef XMLRPC_DEBUG
    log(30,"XmlRpcServerConnection::readHeader: KeepAlive: %d",keepAlive) ;
    #endif
  } else                                                    {
    keepAlive = false                                       ;
  }                                                         ;
  ///////////////////////////////////////////////////////////
  header          = ""                                      ;
  response        = ""                                      ;
  ConnectionState = READ_REQUEST                            ;
  return true                                               ;
}

bool N::XmlRpcServerConnection::readRequest(void)
{
  if ( request . length ( ) < contentLength )                   {
    bool       eof = false                                      ;
    QByteArray R                                                ;
    if ( ! XmlRpcSource::nbRead ( getfd ( ) , R , &eof ) )      {
      #ifdef XMLRPC_DEBUG
      error                                                     (
        "XmlRpcServerConnection::readRequest: read error (%s)." ,
        XmlRpcSource::getErrorMsg().toUtf8().constData()      ) ;
      #endif
      return false                                              ;
    }                                                           ;
    request += QString::fromUtf8 ( R )                          ;
    if ( request . length ( )  < contentLength )                {
      if ( eof )                                                {
        #ifdef XMLRPC_DEBUG
        error("XmlRpcServerConnection::readRequest: EOF while reading request") ;
        #endif
        return false                                            ;
      }                                                         ;
      return true                                               ;
    }                                                           ;
  }                                                             ;
  ///////////////////////////////////////////////////////////////
  #ifdef XMLRPC_DEBUG
  log(30,"XmlRpcServerConnection::readRequest read %d bytes.",request.length()            ) ;
  #ifdef VERY_DEEP_DEBUG
  log(50,"XmlRpcServerConnection::readRequest:\n%s\n"        ,request.toUtf8().constData()) ;
  #endif
  #endif
  ///////////////////////////////////////////////////////////////
  response        = ""                                          ;
  ConnectionState = WRITE_RESPONSE                              ;
  return true                                                   ;
}

bool N::XmlRpcServerConnection::writeResponse (void)
{
  if ( 0 == response . length ( ) )                                {
    executeRequest ( )                                             ;
    bytesWritten = 0                                               ;
    if ( 0 == response . length ( ) )                              {
      #ifdef XMLRPC_DEBUG
      error                                                        (
        "XmlRpcServerConnection::writeResponse: empty response.")  ;
      #endif
      return false                                                 ;
    }                                                              ;
  }                                                                ;
  //////////////////////////////////////////////////////////////////
  QByteArray R = response . toUtf8 ( )                             ;
  if ( ! XmlRpcSource::nbWrite ( getfd ( ) , R , &bytesWritten ) ) {
    #ifdef XMLRPC_DEBUG
    error                                                          (
      "XmlRpcServerConnection::writeResponse: write error (%s)."   ,
      XmlRpcSource::getErrorMsg().toUtf8().constData()           ) ;
    #endif
    return false                                                   ;
  }                                                                ;
  //////////////////////////////////////////////////////////////////
  #ifdef XMLRPC_DEBUG
  log                                                              (
    30                                                             ,
    "XmlRpcServerConnection::writeResponse: wrote %d of %d bytes." ,
    bytesWritten                                                   ,
    response.length()                                            ) ;
  #endif
  //////////////////////////////////////////////////////////////////
  if ( bytesWritten >= R . size ( ) )                              {
    header          = ""                                           ;
    request         = ""                                           ;
    response        = ""                                           ;
    ConnectionState = READ_HEADER                                  ;
    bytesWritten    = 0                                            ;
  }                                                                ;
  //////////////////////////////////////////////////////////////////
  return keepAlive                                                 ;
}

void N::XmlRpcServerConnection::executeRequest(void)
{
  XmlRpcValue params, resultValue                                        ;
  QString     methodName = parseRequest ( params )                       ;
  #ifdef XMLRPC_DEBUG
  log                                                                    (
    20                                                                   ,
    "XmlRpcServerConnection::executeRequest: server calling method '%s'" ,
    methodName.toUtf8().constData()                                    ) ;
  #endif
  ////////////////////////////////////////////////////////////////////////
  if ( ( ! executeMethod       ( methodName,params,resultValue      ) ) &&
       ( ! executeMulticall    ( methodName,params,resultValue      ) )  )
       generateFaultResponse ( methodName + ": unknown method name" )    ;
  else generateResponse      ( resultValue . toXml ( )              )    ;
}

QString N::XmlRpcServerConnection::parseRequest(XmlRpcValue & params)
{
  int     offset     = 0                                                     ;
  QString methodName = ""                                                    ;
  ////////////////////////////////////////////////////////////////////////////
  methodName = parseTag ( METHODNAME_TAG , request , &offset )               ;
  if ( methodName . size ( ) <= 0                               ) return ""  ;
  if ( ! findTag ( PARAMS_TAG , request , &offset ) ) return ""              ;
  ////////////////////////////////////////////////////////////////////////////
  int nArgs = 0                                                              ;
  while ( nextTagIs ( PARAM_TAG   , request , &offset ) )                    {
    params [ nArgs ++ ] = XmlRpcValue ( request , &offset )                  ;
    nextTagIs ( PARAM_ETAG  , request , &offset )                            ;
  }                                                                          ;
  nextTagIs ( PARAMS_ETAG , request , &offset )                              ;
  ////////////////////////////////////////////////////////////////////////////
  return methodName                                                          ;
}

bool N::XmlRpcServerConnection::executeMethod  (
       const QString & methodName              ,
       XmlRpcValue   & params                  ,
       XmlRpcValue   & result                  )
{
  XmlRpcServerMethod * method                  ;
  method = Server -> findMethod ( methodName ) ;
  if ( ! method ) return false                 ;
  method -> execute ( params , result )        ;
  if ( ! result . valid ( ) )                  {
    result = false                             ;
  }                                            ;
  return true                                  ;
}

bool N::XmlRpcServerConnection::executeMulticall (
       const QString & methodName                ,
       XmlRpcValue   & params                    ,
       XmlRpcValue   & result                    )
{
  if ( methodName                 != SYSTEM_MULTICALL       ) return false   ;
  if ( params       . getType ( ) != XmlRpcValue::TypeArray ) return false   ;
  if ( params       . size    ( ) != 1                      ) return false   ;
  if ( params [ 0 ] . getType ( ) != XmlRpcValue::TypeArray ) return false   ;
  ////////////////////////////////////////////////////////////////////////////
  int nc = params [ 0 ] . size ( )                                           ;
  result . setSize ( nc )                                                    ;
  ////////////////////////////////////////////////////////////////////////////
  for ( int i = 0 ; i < nc ; ++i )                                           {
    if ( ( ! params [ 0 ] [ i ] . hasMember ( METHODNAME ) )                ||
         ( ! params [ 0 ] [ i ] . hasMember ( PARAMS     ) )               ) {
      result [ i ] [ FAULTCODE   ] = -1                                      ;
      result [ i ] [ FAULTSTRING ] = SYSTEM_MULTICALL                        +
      ": Invalid argument (expected a struct with members methodName and params)" ;
      continue                                                               ;
    }                                                                        ;
    //////////////////////////////////////////////////////////////////////////
    const QString & methodName   = params [ 0 ] [ i ] [ METHODNAME ]         ;
    XmlRpcValue   & methodParams = params [ 0 ] [ i ] [ PARAMS     ]         ;
    XmlRpcValue     resultValue                                              ;
    //////////////////////////////////////////////////////////////////////////
    resultValue . setSize ( 1 )                                              ;
    //////////////////////////////////////////////////////////////////////////
    if ( ! executeMethod    ( methodName , methodParams , resultValue[0] )  &&
         ! executeMulticall ( methodName , params       , resultValue[0] ) ) {
      result[i][FAULTCODE  ] = -1                                            ;
      result[i][FAULTSTRING] = methodName + ": unknown method name"          ;
    } else result [ i ] = resultValue                                        ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  return true                                                                ;
}

void N::XmlRpcServerConnection::generateResponse(const QString & resultXml)
{
  QString body                                        ;
  QString header                                      ;
  /////////////////////////////////////////////////////
  body      = HeaderRESPONSE_1                        ;
  body     += resultXml                               ;
  body     += HeaderRESPONSE_2                        ;
  header    = generateHeader ( body )                 ;
  response  = header + body                           ;
  /////////////////////////////////////////////////////
  #ifdef VERY_DEEP_DEBUG
  log                                                 (
    50                                                ,
    "XmlRpcServerConnection::generateResponse:\n%s\n" ,
    response . toUtf8 ( ) . constData ( )           ) ;
  #endif
}

QString N::XmlRpcServerConnection::generateHeader(const QString & body)
{
  QString header                                               ;
  qint64  bsize = body . size (                              ) ;
  header  = QString           ( "HTTP/1.1 200 OK\r\n"        ) ;
  header += QString           ( "Server: "                   ) ;
  header += QString::fromUtf8 ( XMLRPC                       ) ;
  header += QString           ( "\r\n"                       ) ;
  header += QString           ( "Content-Type: text/xml\r\n" ) ;
  header += QString           ( "Content-length: "           ) ;
  header += QString           ( "%1\r\n\r\n" ) . arg ( bsize ) ;
  return  header                                               ;
}

void N::XmlRpcServerConnection::generateFaultResponse(const QString & errorMsg,int errorCode)
{
  XmlRpcValue faultStruct                               ;
  QString     body                                      ;
  QString     header                                    ;
  ///////////////////////////////////////////////////////
  faultStruct [ FAULTCODE   ] = errorCode               ;
  faultStruct [ FAULTSTRING ] = errorMsg                ;
  ///////////////////////////////////////////////////////
  body                        = FaultRESPONSE_1         ;
  body                       += faultStruct . toXml ( ) ;
  body                       += FaultRESPONSE_2         ;
  header                      = generateHeader ( body ) ;
  response                    = header + body           ;
}
