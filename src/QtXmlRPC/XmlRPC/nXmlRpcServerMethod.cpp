#include <qtxmlrpc.h>

N::XmlRpcServerMethod:: XmlRpcServerMethod (
                        const QString & n  ,
                        XmlRpcServer  * s  )
                      : Name   (        n  )
                      , Server (        s  )
{
  if ( NotNull ( Server ) )      {
    Server -> addMethod ( this ) ;
  }                              ;
}

N::XmlRpcServerMethod::~XmlRpcServerMethod(void)
{
  if ( NotNull ( Server ) )         {
    Server -> removeMethod ( this ) ;
  }                                 ;
}

QString & N::XmlRpcServerMethod::name(void)
{
  return Name ;
}

QString N::XmlRpcServerMethod::help(void)
{
  return QString ( ) ;
}
