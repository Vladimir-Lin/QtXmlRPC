#include <qtxmlrpc.h>

N::RpcMethodHeartbeat:: RpcMethodHeartbeat (QObject * parent,XmlRpcServer * s)
                      : RpcVirtualMethod   (          parent,"Heartbeat"  , s)
{
}

N::RpcMethodHeartbeat::~RpcMethodHeartbeat(void)
{
}

int N::RpcMethodHeartbeat::Type (void) const
{
  return 2 ;
}

void N::RpcMethodHeartbeat::execute(XmlRpcValue & params,XmlRpcValue & result)
{
  result = QString::number ( StarDate::now ( ) ) ;
}
