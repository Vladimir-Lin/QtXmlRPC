#include <qtxmlrpc.h>

N::Rpc:: Rpc    ( void      )
       : Thread ( 0 , false )
{
  ThreadClass                        ( "Rpc"       ) ;
  Exists    = new RpcMethodExists    ( NULL , &RPC ) ;
  Heartbeat = new RpcMethodHeartbeat ( NULL , &RPC ) ;
}

N::Rpc:: Rpc    ( const Rpc & rpc )
       : Thread ( 0 , false       )
{ Q_UNUSED ( rpc )                                   ;
  ThreadClass                        ( "Rpc"       ) ;
  Exists    = new RpcMethodExists    ( NULL , &RPC ) ;
  Heartbeat = new RpcMethodHeartbeat ( NULL , &RPC ) ;
}

N::Rpc::~Rpc (void)
{
  if ( NotNull(Exists   ) ) RPC . removeMethod ( Exists    ) ;
  if ( NotNull(Heartbeat) ) RPC . removeMethod ( Heartbeat ) ;
  if ( NotNull(Exists   ) ) delete Exists                    ;
  if ( NotNull(Heartbeat) ) delete Heartbeat                 ;
  Exists    = NULL                                           ;
  Heartbeat = NULL                                           ;
}

void N::Rpc::setParent(QObject * parent,const char * method,const char * login)
{
  Exists    -> setParent ( parent )                              ;
  Heartbeat -> setParent ( parent )                              ;
  nConnect ( Exists , SIGNAL ( safeShowMaximized         ( ) )   ,
             Exists , SLOT   ( acceptShowMaximized       ( ) ) ) ;
  if (NotNull(method))
  nConnect ( Exists , SIGNAL ( showMaximized             ( ) )   ,
             parent , method                                   ) ;
  nConnect ( Exists , SIGNAL ( safeLogin   (QString,QString) )   ,
             Exists , SLOT   ( acceptLogin (QString,QString) ) ) ;
  if (NotNull(login))
  nConnect ( Exists , SIGNAL ( Login       (QString,QString) ) ,
             parent , login                                    ) ;
}

bool N::Rpc::Start(int port)
{
  VarArgs args           ;
  args << port           ;
  start ( 10001 , args ) ;
  return true            ;
}

bool N::Rpc::Stop(void)
{
  cleanup        (     )                         ;
  if ( AllThreads . count ( ) <= 0 ) return true ;
  RPC . exit     (     )                         ;
  Time::skip     ( 100 )                         ;
  RPC . shutdown (     )                         ;
  return true                                    ;
}

void N::Rpc::run(int T,ThreadData * d)
{
  switch ( T )      {
    case 10001      :
      Working ( d ) ;
    break           ;
  }                 ;
}

void N::Rpc::Working(ThreadData * d)
{
  if ( ! IsContinue ( d )             ) return                         ;
  if ( d -> Arguments . count ( ) < 1 ) return                         ;
  int port = d -> Arguments [ 0 ] . toInt ( )                          ;
  RPC . Controller = Data . Controller                                 ;
  while ( IsContinue ( d ) && ( ! RPC . bindAndListen ( port , 0 ) ) ) {
    Time::msleep ( 100 )                                               ;
  }                                                                    ;
  if ( IsContinue ( d ) ) RPC . work ( -1 )                            ;
}
