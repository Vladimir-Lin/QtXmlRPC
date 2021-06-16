#include <qtxmlrpc.h>

static const QString LIST_METHODS ( "system.listMethods" ) ;
static const QString METHOD_HELP  ( "system.methodHelp"  ) ;
static const QString MULTICALL    ( "system.multicall"   ) ;

class ListMethods : public N::XmlRpcServerMethod
{
  public:

    explicit ListMethods(N::XmlRpcServer * s) : XmlRpcServerMethod(LIST_METHODS,s) { ; }
    virtual ~ListMethods(void) { ; }

    virtual int Type (void) const { return 100001 ; }

    void execute(N::XmlRpcValue & params,N::XmlRpcValue & result)
    { Q_UNUSED ( params )         ;
      Server->listMethods(result) ;
    }

    QString help(void)
    {
      return QString("List all methods available on a server as an array of strings") ;
    }

} ;

class MethodHelp : public N::XmlRpcServerMethod
{
  public:

    explicit MethodHelp(N::XmlRpcServer * s) : XmlRpcServerMethod(METHOD_HELP,s) { ; }
    virtual ~MethodHelp(void) { ; }

    virtual int Type (void) const { return 100002 ; }

    void execute(N::XmlRpcValue & params,N::XmlRpcValue & result)
    {
      if ( params       . getType ( ) != N::XmlRpcValue::TypeArray  ) return ;
      if ( params . size ( ) <= 0                                   ) return ;
      if ( params [ 0 ] . getType ( ) != N::XmlRpcValue::TypeString ) return ;
      XmlRpcServerMethod * m = Server -> findMethod ( params [ 0 ]  )        ;
      if ( NULL == m                                                ) return ;
      result = m -> help ( )                                                 ;
    }

    QString help(void)
    {
      return QString("Retrieve the help string for a named method");
    }

} ;

/////////////////////////////////////////////////////////////////////////////

N::XmlRpcServer:: XmlRpcServer (void)
                : XmlRpcSource (    )
{
  introspectionEnabled    = false         ;
  listmethods             = NULL          ;
  methodHelp              = NULL          ;
  flags                   = ReadableEvent ;
  Variables [ "Broken"  ] = 60000         ;
  Variables [ "Expire"  ] = 90000         ;
  Variables [ "Running" ] = false         ;
  Variables [ "Enter"   ] = false         ;
  Variables [ "Ready"   ] = true          ;
  Variables [ "Backlog" ] = 0             ;
  setKeepOpen ( true )                    ;
}

N::XmlRpcServer::~XmlRpcServer(void)
{
  shutdown        ( )                               ;
  methods . clear ( )                               ;
  if ( NotNull ( listmethods ) ) delete listmethods ;
  if ( NotNull ( methodHelp  ) ) delete methodHelp  ;
  listmethods = NULL                                ;
  methodHelp  = NULL                                ;
}

void N::XmlRpcServer::addMethod(XmlRpcServerMethod * method)
{
  if ( NULL == method ) return            ;
  methods [ method -> name ( ) ] = method ;
}

void N::XmlRpcServer::removeMethod(XmlRpcServerMethod * method)
{
  if ( NULL == method                              ) return ;
  if ( ! methods . contains ( method -> name ( ) ) ) return ;
  methods  .take ( method -> name ( ) )                     ;
}

void N::XmlRpcServer::removeMethod(const QString & methodName)
{
  if ( ! methods . contains ( methodName ) ) return ;
  methods . take ( methodName )                     ;
}

N::XmlRpcServerMethod * N::XmlRpcServer::findMethod(const QString & name) const
{
  if ( ! methods . contains ( name ) ) return NULL ;
  return methods [ name ]                          ;
}

void N::XmlRpcServer::enableIntrospection(bool enabled)
{
  if ( introspectionEnabled == enabled ) return ;
  introspectionEnabled = enabled                ;
  if   ( enabled                )               {
    if ( IsNull ( listmethods ) )               {
      listmethods = new ListMethods ( this )    ;
      methodHelp  = new MethodHelp  ( this )    ;
    } else                                      {
      addMethod  ( listmethods  )               ;
      addMethod  ( methodHelp   )               ;
    }                                           ;
  } else                                        {
    removeMethod ( LIST_METHODS )               ;
    removeMethod ( METHOD_HELP  )               ;
  }                                             ;
}

void N::XmlRpcServer::listMethods(XmlRpcValue & result)
{
  QStringList K = methods . keys ( )          ;
  int         i = 0                           ;
  result . setSize ( methods . size ( ) + 1 ) ;
  for (i = 0 ; i < K . count ( ) ; i++ )      {
    result [ i ] = K [ i ]                    ;
  }                                           ;
  result   [ i ] = MULTICALL                  ;
}

bool N::XmlRpcServer::bindAndListen(int port,int backlog)
{
  int FD = XmlRpcSource::socket ( )                                          ;
  if ( FD < 0 )                                                              {
    #ifdef XMLRPC_DEBUG
    error                                                                    (
      "XmlRpcServer::bindAndListen: Could not create socket (%s)."           ,
      XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )           ) ;
    #endif
    return false                                                             ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  setfd ( FD )                                                               ;
  if ( ! XmlRpcSource::setNonBlocking ( FD ) )                               {
    XmlRpcSource::close ( )                                                  ;
    #ifdef XMLRPC_DEBUG
    error                                                                    (
      "XmlRpcServer::bindAndListen: Could not set socket to non-blocking input mode (%s)." ,
      XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )           ) ;
    #endif
    return false                                                             ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  if ( ! XmlRpcSource::setReuseAddr ( FD ) )                                 {
    XmlRpcSource::close ( )                                                  ;
    #ifdef XMLRPC_DEBUG
    error                                                                    (
      "XmlRpcServer::bindAndListen: Could not set SO_REUSEADDR socket option (%s)." ,
      XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )           ) ;
    #endif
    return false                                                             ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  if (!XmlRpcSource::bind ( FD , port ) )                                    {
    XmlRpcSource::close ( )                                                  ;
    #ifdef XMLRPC_DEBUG
    error                                                                    (
      "XmlRpcServer::bindAndListen: Could not bind to specified port (%s)."  ,
      XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )           ) ;
    #endif
    return false                                                             ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  Variables [ "Backlog" ] = backlog                                          ;
  #ifdef XMLRPC_DEBUG
  log                                                                        (
    20                                                                       ,
    "XmlRpcServer::bindAndListen: server listening on port %d fd %d"         ,
    port                                                                     ,
    FD                                                                     ) ;
  #endif
  return true                                                                ;
}

void N::XmlRpcServer::work(double msTime)
{
  if ( Variables [ "Running" ] . toBool ( ) ) return                         ;
  Variables [ "Running" ] = true                                             ;
  Variables [ "Enter"   ] = true                                             ;
  ////////////////////////////////////////////////////////////////////////////
  struct timeval tv                                                          ;
  int    nEvents                                                             ;
  int    loop                                                                ;
  while ( Variables [ "Running" ] . toBool ( ) )                             {
    Time::msleep ( 1 )                                                       ;
    if ( ( NULL != Controller ) && ( ! (*Controller) ) ) break               ;
    //////////////////////////////////////////////////////////////////////////
    int        sfd   = getfd ( )                                             ;
    int        maxFd = sfd                                                   ;
    fd_set     inFd                                                          ;
    fd_set     outFd                                                         ;
    fd_set     excFd                                                         ;
    //////////////////////////////////////////////////////////////////////////
    if ( sfd < 0 ) break                                                     ;
    //////////////////////////////////////////////////////////////////////////
    XmlRpcSource::listen ( sfd , Variables [ "Backlog" ] . toInt ( ) )       ;
    //////////////////////////////////////////////////////////////////////////
    FD_ZERO (       &inFd  )                                                 ;
    FD_ZERO (       &outFd )                                                 ;
    FD_ZERO (       &excFd )                                                 ;
    FD_SET  ( sfd , &inFd  )                                                 ;
    FD_SET  ( sfd , &excFd )                                                 ;
    //////////////////////////////////////////////////////////////////////////
    for (int i = 0 ; i < sources . count ( ) ; i++ )                         {
      int          vfd  = sources [ i ] -> getfd ( )                         ;
      unsigned int mask = sources [ i ] -> flags                             ;
      if ( vfd >= 0 )                                                        {
        FD_SET                             ( vfd , &excFd )                  ;
        if ( mask & ReadableEvent ) FD_SET ( vfd , &inFd  )                  ;
        if ( mask & WritableEvent ) FD_SET ( vfd , &outFd )                  ;
        if ( vfd > maxFd          ) maxFd  = vfd                             ;
      }                                                                      ;
    }                                                                        ;
    //////////////////////////////////////////////////////////////////////////
    tv . tv_sec  = 0                                                         ;
    tv . tv_usec = 25000                                                     ;
    nEvents = ::select ( maxFd + 1 , &inFd , &outFd , &excFd , &tv )         ;
    //////////////////////////////////////////////////////////////////////////
    if ( nEvents <  0 ) break                                                ;
    if ( nEvents == 0 ) continue                                             ;
    //////////////////////////////////////////////////////////////////////////
    if ( FD_ISSET ( sfd , &excFd ) ) break                                   ;
    if ( FD_ISSET ( sfd , &inFd  ) ) handleEvent ( ReadableEvent )           ;
    //////////////////////////////////////////////////////////////////////////
    loop = 0                                                                 ;
    while ( loop < sources . count ( ) )                                     {
      if ( ( NULL != Controller ) && ( ! (*Controller) ) )                   {
        Variables [ "Running" ] = false                                      ;
        Variables [ "Enter"   ] = false                                      ;
        return                                                               ;
      }                                                                      ;
      ////////////////////////////////////////////////////////////////////////
      XmlRpcSource * src = NULL                                              ;
      if ( loop < sources . count ( ) ) src = sources [ loop ]               ;
      if ( ( NULL == src ) && ( loop < sources . count ( ) ) )               {
        sources . takeAt ( loop )                                            ;
        continue                                                             ;
      }                                                                      ;
      ////////////////////////////////////////////////////////////////////////
      int          vfd     = src -> getfd ( )                                ;
      unsigned int newMask = (unsigned int) -1                               ;
      bool         correct = true                                            ;
      if ( vfd <= maxFd )                                                    {
        bool processed = false                                               ;
        if ( FD_ISSET ( vfd , &inFd  ) )                                     {
          newMask  &= src -> handleEvent ( ReadableEvent )                   ;
          processed = true                                                   ;
        } else
        if ( FD_ISSET ( vfd , &outFd ) )                                     {
          newMask  &= src -> handleEvent ( WritableEvent )                   ;
          processed = true                                                   ;
        }                                                                    ;
        if ( FD_ISSET ( vfd , &excFd ) || ( 0 == newMask ) )                 {
          newMask  &= src -> handleEvent ( Exception     )                   ;
          processed = false                                                  ;
        }                                                                    ;
        newMask = src -> Processing ( processed , newMask )                  ;
        if ( 0 == newMask )                                                  {
          delete src                                                         ;
          correct = false                                                    ;
        } else if ( newMask != (unsigned) -1 )                               {
          sources [ loop ] -> flags = newMask                                ;
        }                                                                    ;
      }                                                                      ;
      if ( correct ) loop ++                                                 ;
      Time::msleep ( 1 )                                                     ;
    }                                                                        ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  Variables [ "Running" ] = false                                            ;
  Variables [ "Enter"   ] = false                                            ;
}

unsigned N::XmlRpcServer::handleEvent(unsigned mask)
{
  if ( ReadableEvent == mask )                {
    if ( Variables [ "Ready" ] . toBool ( ) ) {
      acceptConnection ( )                    ;
    }                                         ;
  }                                           ;
  return ReadableEvent                        ;
}

void N::XmlRpcServer::acceptConnection(void)
{
  int s = XmlRpcSource::accept ( getfd ( ) )                               ;
  #ifdef XMLRPC_DEBUG
  log ( 20 , "XmlRpcServer::acceptConnection: socket %d" , s             ) ;
  #endif
  if ( s < 0 )                                                             {
    #ifdef XMLRPC_DEBUG
    error (
      "XmlRpcServer::acceptConnection: Could not accept connection (%s)."  ,
      XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )         ) ;
    #endif
    return                                                                 ;
  }                                                                        ;
  if ( ! XmlRpcSource::setNonBlocking ( s ) )                              {
    XmlRpcSource::close ( s )                                              ;
    #ifdef XMLRPC_DEBUG
    error                                                                  (
      "XmlRpcServer::acceptConnection: Could not set socket to non-blocking input mode (%s).",
      XmlRpcSource::getErrorMsg ( ) . toUtf8 ( ) . constData ( )         ) ;
    #endif
  } else                                                                   {
    #ifdef XMLRPC_DEBUG
    log ( 20 , "XmlRpcServer::acceptConnection: creating a connection"   ) ;
    #endif
    sources . append ( createConnection ( s ) )                            ;
    #ifdef XMLRPC_DEBUG
    log ( 20 , "XmlRpcServer::acceptConnection: source.append"           ) ;
    #endif
  }                                                                        ;
}

N::XmlRpcServerConnection * N::XmlRpcServer::createConnection(int s)
{
  N::XmlRpcServerConnection * srs                          ;
  srs = new XmlRpcServerConnection ( s , this )            ;
  srs -> flags = ReadableEvent                             ;
  if ( Variables . contains ( "Expire" ) )                 {
    srs -> Variables [ "Expire" ] = Variables [ "Expire" ] ;
  }                                                        ;
  return srs                                               ;
}

void N::XmlRpcServer::removeConnection(XmlRpcServerConnection * sc)
{
  if ( sources . count ( ) <= 0 ) return ;
  if ( sources . contains ( sc ) )       {
    int index = sources . indexOf ( sc ) ;
    if ( index >= 0 )                    {
      sources . takeAt ( index )         ;
    }                                    ;
  }                                      ;
}

void N::XmlRpcServer::exit(void)
{
  Variables [ "Running" ] = false ;
}

void N::XmlRpcServer::shutdown(void)
{
  while ( Variables [ "Enter"] . toBool ( ) ) {
    Time::msleep ( 10 )                       ;
  }                                           ;
  if ( sources . count ( ) > 0 )              {
    QList<XmlRpcSource *> srcs = sources      ;
    while ( srcs . count ( ) > 0 )            {
      XmlRpcSource * src = srcs . first ( )   ;
      srcs . takeFirst ( )                    ;
      if ( NULL != src ) delete src           ;
    }                                         ;
  }                                           ;
  XmlRpcSource::close (    )                  ;
}
