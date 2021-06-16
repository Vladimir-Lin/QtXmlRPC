#include <qtxmlrpc.h>

static
class DefaultLogHandler : public N::XmlRpcLogHandler
{
  public:

    void log (int level,const char * msg)
    {
      #ifdef XMLRPC_DEBUG
      N::printf ( QString("Log(%1) : %2").arg(level).arg(QString::fromUtf8(msg)) , true , true ) ;
      #endif
    }

} defaultLogHandler ;

int N::XmlRpcLogHandler::verbosity = 100                                   ;
N::XmlRpcLogHandler * N::XmlRpcLogHandler::logHandler = &defaultLogHandler ;

N::XmlRpcLogHandler:: XmlRpcLogHandler (void)
{
}

N::XmlRpcLogHandler::~XmlRpcLogHandler (void)
{
}

N::XmlRpcLogHandler * N::XmlRpcLogHandler::getLogHandler (void)
{
  return logHandler ;
}

void N::XmlRpcLogHandler::setLogHandler(XmlRpcLogHandler * lh)
{
  logHandler = lh ;
}

int N::XmlRpcLogHandler::getVerbosity(void)
{
  return verbosity ;
}

void N::XmlRpcLogHandler::setVerbosity(int v)
{
  verbosity = v ;
}
