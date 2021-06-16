#include <qtxmlrpc.h>

N::RpcCommands:: RpcCommands ( void      )
               : Thread      ( 0 , false )
               , Interval    ( 30        )
               , Alive       ( false     )
{
  ThreadClass ( "RpcCommands" ) ;
}

N::RpcCommands:: RpcCommands ( const RpcCommands & rpc )
               : Thread      ( 0 , false               )
               , Interval    ( 30                      )
               , Alive       ( false                   )
{ Q_UNUSED    ( rpc           ) ;
  ThreadClass ( "RpcCommands" ) ;
}

N::RpcCommands::~RpcCommands (void)
{
}

bool N::RpcCommands::Exists(QString host,int port)
{
  XmlRpcClient c ( host , port )                  ;
  XmlRpcValue  Args                               ;
  XmlRpcValue  result                             ;
  Args       . setSize     ( 1        )           ;
  Args [ 0 ] = XmlRpcValue ( "Native" )           ;
  return c . execute ( "Exists" , Args , result ) ;
}

bool N::RpcCommands::Login(QString host,int port,QString username,QString hostname)
{
  XmlRpcClient c ( host , port )                  ;
  XmlRpcValue  Args                               ;
  XmlRpcValue  result                             ;
  Args       . setSize     ( 3        )           ;
  Args [ 0 ] = XmlRpcValue ( "Remote" )           ;
  Args [ 1 ] = XmlRpcValue ( username )           ;
  Args [ 2 ] = XmlRpcValue ( hostname )           ;
  return c . execute ( "Exists" , Args , result ) ;
}

SUID N::RpcCommands::addRequest(SUID uuid,QString host,int port)
{
  Requests   [ uuid ] = new XmlRpcClient ( host , port ) ;
  Heartbeats [ uuid ] = StarDate::now    (             ) ;
  return uuid                                            ;
}

SUID N::RpcCommands::addRequest(SUID uuid,XmlRpcClient * request)
{
  if ( NULL == request ) return 0         ;
  Requests   [ uuid ] = request           ;
  Heartbeats [ uuid ] = StarDate::now ( ) ;
  return       uuid                       ;
}

bool N::RpcCommands::removeRequest(SUID uuid)
{
  if ( ! Requests . contains ( uuid ) ) return false ;
  mutex [ "Request" ] . lock    ( )                  ;
  XmlRpcClient * client = Requests [ uuid ]          ;
  Requests . take ( uuid )                           ;
  if ( Heartbeats . contains ( uuid ) )              {
    Heartbeats . take ( uuid )                       ;
  }                                                  ;
  if ( NULL != client )                              {
    client -> setKeepOpen ( false )                  ;
    client -> close       (       )                  ;
    delete client                                    ;
  }                                                  ;
  mutex [ "Request" ] . unlock  ( )                  ;
  return true                                        ;
}

bool N::RpcCommands::execute (
        SUID          uuid   ,
        const char  * method ,
        XmlRpcValue & params ,
        XmlRpcValue & result )
{
  if ( ! Requests . contains ( uuid ) ) return false                     ;
  mutex [ "Request" ] . lock   ( )                                       ;
  bool reply = Requests [ uuid ] -> execute ( method , params , result ) ;
  Heartbeats [ uuid ] = StarDate::now()                                  ;
  mutex [ "Request" ] . unlock ( )                                       ;
  return reply                                                           ;
}

void N::RpcCommands::run(int T,ThreadData * d)
{
  switch ( T )         {
    case 10001         :
      Requesting ( d ) ;
    break              ;
  }                    ;
}

void N::RpcCommands::Requesting(ThreadData * d)
{
  Alive = true                                                              ;
  while ( IsContinue ( d ) && Alive )                                       {
    mutex [ "Request" ] . lock ( )                                          ;
    UUIDs Uuids = Requests . keys ( )                                       ;
    SUID  uuid                                                              ;
    foreach ( uuid , Uuids )                                                {
      XmlRpcValue Args                                                      ;
      XmlRpcValue result                                                    ;
      SUID heartbeat = StarDate::now ( )                                    ;
      if ( ( heartbeat - Heartbeats [ uuid ] ) >= Interval )                {
        if ( Requests [ uuid ] -> execute ( "Heartbeat" , Args , result ) ) {
          Heartbeats  [ uuid ] =  StarDate::now()                           ;
        }                                                                   ;
      }                                                                     ;
    }                                                                       ;
    mutex [ "Request" ] . unlock ( )                                        ;
    for (int i=0;IsContinue ( d ) && Alive && i<100;i++) Time::skip(10)     ;
  }                                                                         ;
}

void N::RpcCommands::startRequesting(void)
{
  start ( 10001 ) ;
}
