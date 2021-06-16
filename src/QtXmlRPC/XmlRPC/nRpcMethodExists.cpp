#include <qtxmlrpc.h>

N::RpcMethodExists:: RpcMethodExists  (QObject * parent,XmlRpcServer * s)
                   : RpcVirtualMethod (          parent,"Exists"     , s)
{
}

N::RpcMethodExists::~RpcMethodExists(void)
{
}

int N::RpcMethodExists::Type (void) const
{
  return 1 ;
}

void N::RpcMethodExists::execute(XmlRpcValue & params,XmlRpcValue & result)
{
  if     ( params . size ( ) > 0                    ) {
    if   ( params [ 0 ] == XmlRpcValue ( "Native" ) ) {
      emit safeShowMaximized ( )                      ;
    } else
    if   ( params [ 0 ] == XmlRpcValue ( "Remote" ) ) {
      if ( params . size ( ) > 2                    ) {
        QString login = params [ 1 ]                  ;
        QString host  = params [ 2 ]                  ;
        emit safeLogin ( login , host )               ;
      }                                               ;
    }                                                 ;
  }                                                   ;
  result = true                                       ;
}

void N::RpcMethodExists::acceptShowMaximized(void)
{
  emit showMaximized ( ) ;
}

void N::RpcMethodExists::acceptLogin(QString login,QString hostname)
{
  emit Login ( login , hostname ) ;
}
