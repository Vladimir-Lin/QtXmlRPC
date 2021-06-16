#include <qtxmlrpc.h>

static
class DefaultErrorHandler : public N::XmlRpcErrorHandler
{
  public:

    void error(const char * msg)
    {
      #ifdef XMLRPC_DEBUG
      N::printf ( QString::fromUtf8(msg) , true , true ) ;
      #endif
    }

} defaultErrorHandler ;

N::XmlRpcErrorHandler * N::XmlRpcErrorHandler :: errorHandler = &defaultErrorHandler ;

N::XmlRpcErrorHandler:: XmlRpcErrorHandler (void)
{
}

N::XmlRpcErrorHandler::~XmlRpcErrorHandler (void)
{
}

N::XmlRpcErrorHandler * N::XmlRpcErrorHandler::getErrorHandler(void)
{
  return errorHandler;
}

void N::XmlRpcErrorHandler::setErrorHandler(XmlRpcErrorHandler * eh)
{
  errorHandler = eh ;
}
