#include <qtxmlrpc.h>

N::RpcVirtualMethod:: RpcVirtualMethod   (QObject * parent,QString name,XmlRpcServer * s)
                    : QObject            (          parent                              )
                    , XmlRpcServerMethod (                         name,               s)
{
}

N::RpcVirtualMethod::~RpcVirtualMethod (void)
{
}
