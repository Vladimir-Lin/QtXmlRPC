#include <qtxmlrpc.h>

#ifndef Q_OS_WIN
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#endif

#define TRYTOREAD  5
#define TRYTOWRITE 5
#define LBUFSIZE   16384
#define SBUFSIZE   16383

N::XmlRpcSource:: XmlRpcSource  ( int FD     )
                : Controller    ( NULL       )
                , flags         ( 0xFFFFFFFF )
                , READSIZE      ( 4096       )
                , readBuf       ( NULL       )
                , errBuf        ( NULL       )
                , msgBuf        ( NULL       )
                , fd            (     FD     )
                , keepOpen      ( false      )
{
  readBuf                    = (char *) ::malloc ( READSIZE ) ;
  #ifdef XMLRPC_DEBUG
  errBuf                     = (char *) ::malloc ( LBUFSIZE ) ;
  msgBuf                     = (char *) ::malloc ( 256      ) ;
  #endif
  Variables [ "Connecting" ] = 3000                           ;
}

N::XmlRpcSource::~XmlRpcSource(void)
{
  if ( NULL != readBuf ) {
    ::free ( readBuf )   ;
  }                      ;
  #ifdef XMLRPC_DEBUG
  if ( NULL != errBuf  ) {
    ::free ( errBuf  )   ;
  }                      ;
  if ( NULL != msgBuf  ) {
    ::free ( msgBuf  )   ;
  }                      ;
  #endif
  READSIZE = 0           ;
  readBuf  = NULL        ;
  errBuf   = NULL        ;
  msgBuf   = NULL        ;
}

QString N::XmlRpcSource::parseTag(const char * tag,const QString & xml,int * offset)
{
  if ( NULL == offset            ) return ""    ;
  if ( NULL == tag               ) return ""    ;
  if ( (*offset) >= xml.length() ) return ""    ;
  ///////////////////////////////////////////////
  QString Tag    = QString::fromUtf8(tag)       ;
  if ( Tag . length ( ) <= 0   ) return ""      ;
  int istart = xml . indexOf ( Tag , *offset )  ;
  if ( istart < 0 ) return ""                   ;
  istart += Tag.length()                        ;
  QString etag = "</"                           ;
  QString TOX  = QString::fromUtf8(tag+1)       ;
  if ( TOX . length ( ) <= 0   ) return ""      ;
  etag . append ( TOX )                         ;
  int iend = xml . indexOf ( etag , istart )    ;
  if ( iend < 0 ) return ""                     ;
  (*offset) = (int)( iend + etag . length ( ) ) ;
  return xml . mid ( istart , iend - istart )   ;
}

bool N::XmlRpcSource::findTag(const char * tag,const QString & xml,int * offset)
{
  if ( NULL == offset                ) return false ;
  if ( NULL == tag                   ) return false ;
  if ( (*offset) >= xml . length ( ) ) return false ;
  ///////////////////////////////////////////////////
  QString Tag = QString::fromUtf8 ( tag )           ;
  int istart = xml . indexOf ( Tag , *offset )      ;
  if ( istart < 0 ) return false                    ;
  (*offset) = (int)( istart + Tag . length ( ) )    ;
  ///////////////////////////////////////////////////
  return true                                       ;
}

bool N::XmlRpcSource::nextTagIs(const char * tag,const QString & xml,int * offset)
{
  if ( NULL == offset                ) return false ;
  if ( NULL == tag                   ) return false ;
  if ( (*offset) >= xml . length ( ) ) return false ;
  ///////////////////////////////////////////////////
  QString Tag = QString::fromUtf8 ( tag )           ;
  int     len = Tag . length ( )                    ;
  if ( len <= 0                      ) return false ;
  int nc   = ( *offset )                            ;
  int xlen = xml . length ( )                       ;
  while ( ( nc < xlen )                            &&
          ( xml . at ( nc ) . isSpace ( )          ||
            xml . at ( nc ) == QChar('\t')         ||
            xml . at ( nc ) == QChar('\r')         ||
            xml . at ( nc ) == QChar('\n') ) ) ++nc ;
  ///////////////////////////////////////////////////
  QString ml = xml . mid ( nc , len )               ;
  if ( len != ml . length ( ) ) return false        ;
  Tag = Tag . toLower ( )                           ;
  ml  = ml  . toLower ( )                           ;
  if ( ml == Tag )                                  {
    (*offset) = ( nc + len )                        ;
    return true                                     ;
  }                                                 ;
  ///////////////////////////////////////////////////
  return false                                      ;
}

QString N::XmlRpcSource::getNextTag(const QString & xml,int * offset)
{
  if ( NULL      == offset            ) return ""                         ;
  if ( (*offset) >= xml . length ( )  ) ""                                ;
  int pos = *offset                                                       ;
  while ( ( pos < xml . length ( ) )                                     &&
          ( xml . at ( pos ) . isSpace ( )                               ||
            xml . at ( pos ) == QChar('\t')                              ||
            xml . at ( pos ) == QChar('\r')                              ||
            xml . at ( pos ) == QChar('\n')                     ) ) ++pos ;
  if ( xml . at ( pos ) != QChar('<') ) return ""                         ;
  /////////////////////////////////////////////////////////////////////////
  QString s                                                               ;
  while ( pos < xml . length ( ) && ( xml . at ( pos ) != QChar ('>') ) ) {
    s . append ( xml . at ( pos ) )                                       ;
    ++pos                                                                 ;
  }                                                                       ;
  if ( ( pos < xml . length ( ) ) && ( xml . at ( pos ) == QChar('>') ) ) {
    s . append ( xml . at ( pos ) )                                       ;
    ++pos                                                                 ;
  }                                                                       ;
  (*offset) = pos                                                         ;
  /////////////////////////////////////////////////////////////////////////
  return s                                                                ;
}

unsigned N::XmlRpcSource::Processing(bool processed,unsigned newMask)
{ Q_UNUSED ( processed ) ;
  return newMask         ;
}

int N::XmlRpcSource::getfd(void) const
{
  return fd ;
}

void N::XmlRpcSource::setfd(int FD)
{
  fd = FD;
}

bool N::XmlRpcSource::getKeepOpen(void) const
{
  return keepOpen ;
}

void N::XmlRpcSource::setKeepOpen(bool b)
{
  keepOpen = b ;
}

void N::XmlRpcSource::close(void)
{
  if ( fd < 0 ) return                                       ;
  #ifdef XMLRPC_DEBUG
  log (20,"XmlRpcSource::close: closing socket %d."     ,fd) ;
  #endif
  XmlRpcSource::close( fd                                  ) ;
  #ifdef XMLRPC_DEBUG
  log (20,"XmlRpcSource::close: done closing socket %d.",fd) ;
  #endif
  fd = -1                                                    ;
}

bool N::XmlRpcSource::nonFatalError (void)
{
  int err = getError ( )                  ;
  #ifdef Q_OS_WIN
  return ( ( err == WSAEINPROGRESS )     ||
           ( err == EAGAIN         )     ||
           ( err == WSAEWOULDBLOCK )     ||
           ( err == WSAEINTR       )    ) ;
  #else
  return ( ( err == EINPROGRESS    )     ||
           ( err == EAGAIN         )     ||
           ( err == EWOULDBLOCK    )     ||
           ( err == EINTR          )    ) ;
  #endif
}

int N::XmlRpcSource::socket(void)
{
  return (int) ::socket ( AF_INET , SOCK_STREAM , 0 ) ;
}

void N::XmlRpcSource::close(int sfd)
{
  if ( sfd < 0 ) return                             ;
  #ifdef XMLRPC_DEBUG
  log ( 40 , "XmlRpcSource::close: fd <%d>" , sfd ) ;
  #endif
  #ifdef Q_OS_WIN
  ::closesocket ( sfd             )                 ;
  #else
  ::shutdown    ( sfd , SHUT_RDWR )                 ;
  #endif
}

bool N::XmlRpcSource::setNonBlocking(int sfd)
{
  if ( sfd < 0 ) return false                                      ;
  #ifdef Q_OS_WIN
  unsigned long flag = 1                                           ;
  return ( ::ioctlsocket ( (SOCKET) sfd , FIONBIO , &flag ) == 0 ) ;
  #else
  #if defined(O_NONBLOCK)
  int flag = ::fcntl ( sfd , F_GETFL , 0    )                      ;
  if ( flag < 0 ) flag = 0                                         ;
  flag    |= O_NONBLOCK                                            ;
  flag     = ::fcntl ( sfd , F_SETFL , flag )                      ;
  return ( flag >= 0 )                                             ;
  #else
  int flags = 1                                                    ;
  return ( ::ioctl       (          sfd , FIOBIO , &flags ) == 0 ) ;
  #endif
  #endif
}

bool N::XmlRpcSource::setReuseAddr(int sfd)
{
  if ( sfd < 0 ) return false             ;
  int sflag = 1                           ;
  return ( ::setsockopt                   (
             sfd                          ,
             SOL_SOCKET                   ,
             SO_REUSEADDR                 ,
             (const char *) &sflag        ,
             sizeof(int)         ) == 0 ) ;
}

bool N::XmlRpcSource::bind(int s,int port)
{
  if ( s < 0 ) return false                                                 ;
  ///////////////////////////////////////////////////////////////////////////
  struct sockaddr_in saddr                                                  ;
  ::memset ( &saddr , 0 , sizeof(saddr) )                                   ;
  saddr . sin_family        = AF_INET                                       ;
  saddr . sin_addr . s_addr = htonl(INADDR_ANY)                             ;
  saddr . sin_port          = htons((u_short) port)                         ;
  ///////////////////////////////////////////////////////////////////////////
  return ( 0 == ::bind ( s , (struct sockaddr *) &saddr , sizeof(saddr) ) ) ;
}

bool N::XmlRpcSource::listen(int sfd,int backlog)
{
  if ( sfd < 0 ) return false                ;
  return ( 0 == ::listen ( sfd , backlog ) ) ;
}

int N::XmlRpcSource::accept(int s)
{
  if ( s < 0 ) return -1                                                         ;
  struct sockaddr_in addr                                                        ;
  int                addrlen = sizeof(addr)                                      ;
  #ifdef Q_OS_WIN
  return (int) ::accept ( s , (struct sockaddr*)&addr , &addrlen               ) ;
  #else
  return (int) ::accept ( s , (struct sockaddr*)&addr , (socklen_t *) &addrlen ) ;
  #endif
}

bool N::XmlRpcSource::connect(int s,QString & host, int port)
{
  if ( s < 0 ) return false                                                  ;
  ////////////////////////////////////////////////////////////////////////////
  IpAddresses addresses                                                      ;
  if ( ! Lookup ( host , addresses ) ) return false                          ;
  if ( addresses . count ( ) <= 0    ) return false                          ;
  ////////////////////////////////////////////////////////////////////////////
  QString     ADDRS                                                          ;
  bool        correct = false                                                ;
  int         result                                                         ;
  qint64      connecting = Variables [ "Connecting" ] . toLongLong ( )       ;
  #ifdef XMLRPC_DEBUG
  QByteArray  ADDRB                                                          ;
  #endif
  ////////////////////////////////////////////////////////////////////////////
  for (int i=0; ( ! correct ) && i < addresses . count ( ) ; i++ )           {
    if ( Types::IPv4 == addresses [ i ] . type ( ) )                         {
      ADDRS   = addresses [ i ] . toString ( false )                         ;
      if ( ADDRS . length ( ) > 0 )                                          {
        struct sockaddr_in saddr                                             ;
        #ifdef XMLRPC_DEBUG
        ADDRB   = ADDRS . toUtf8 ( )                                         ;
        ::memset ( msgBuf , 0 , 256 )                                        ;
        ::memcpy ( msgBuf , ADDRB . data ( ) , ADDRB . size ( ) )            ;
        #endif
        addresses [ i ] . setPort    ( port  )                               ;
        if ( addresses [ i ] . obtain ( saddr ) )                            {
          #ifdef XMLRPC_DEBUG
          log ( 60 , "Connect to %s:%d ..." , msgBuf , port )                ;
          #endif
          result  = ::connect ( s                                            ,
                                (struct sockaddr *)&saddr                    ,
                                sizeof(sockaddr_in)                        ) ;
          correct = ( 0 == result ) || nonFatalError ( )                     ;
          ////////////////////////////////////////////////////////////////////
          if ( correct )                                                     {
            fd_set inFd                                                      ;
            fd_set outFd                                                     ;
            FD_ZERO (     &inFd  )                                           ;
            FD_ZERO (     &outFd )                                           ;
            FD_SET  ( s , &inFd  )                                           ;
            FD_SET  ( s , &outFd )                                           ;
            struct timeval tv                                                ;
            tv . tv_sec  =   connecting / 1000                               ;
            tv . tv_usec = ( connecting % 1000 ) * 1000                      ;
            correct      = false                                             ;
            if ( ::select ( s + 1 , &inFd , &outFd , NULL , &tv ) > 0 )      {
              if ( FD_ISSET ( s , &inFd  ) || FD_ISSET ( s , &outFd ) )      {
                correct = true                                               ;
              }                                                              ;
            }                                                                ;
          }                                                                  ;
          ////////////////////////////////////////////////////////////////////
          #ifdef XMLRPC_DEBUG
          if ( correct )                                                     {
            log ( 60 , "%s:%d connected" , msgBuf , port )                   ;
          } else                                                             {
            log ( 60                                                         ,
                  "Connect %s:%d has error %d"                               ,
                  msgBuf                                                     ,
                  port                                                       ,
                  getError ( )                                             ) ;
          }                                                                  ;
          #endif
        }                                                                    ;
      }                                                                      ;
    }                                                                        ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  return correct                                                             ;
}

bool N::XmlRpcSource::nbRead(int sfd,QByteArray & s,bool * eof,int utimeout)
{
  if ( NULL == eof ) return false                                            ;
  ////////////////////////////////////////////////////////////////////////////
  struct timeval tv                                                          ;
  fd_set         rd                                                          ;
  fd_set         ed                                                          ;
  bool           wouldBlock = false                                          ;
  int            cnt        = 0                                              ;
  int            rt                                                          ;
  int            n                                                           ;
  ////////////////////////////////////////////////////////////////////////////
  *eof         = false                                                       ;
  tv . tv_sec  = utimeout / 1000000                                          ;
  tv . tv_usec = utimeout % 1000000                                          ;
  ////////////////////////////////////////////////////////////////////////////
  while ( ( ! wouldBlock ) && ( ! (*eof) ) )                                 {
    FD_ZERO ( &rd       )                                                    ;
    FD_ZERO ( &ed       )                                                    ;
    FD_SET  ( sfd , &rd )                                                    ;
    FD_SET  ( sfd , &ed )                                                    ;
    rt = ::select ( sfd + 1 , &rd , NULL , &ed , &tv )                       ;
    if ( FD_ISSET ( sfd , &ed ) ) return false                               ;
    if ( rt >= 0 )                                                           {
      if ( FD_ISSET ( sfd , &rd ) )                                          {
        n = ::recv ( sfd , readBuf , READSIZE - 1 , 0 )                      ;
        if ( n >  0 )                                                        {
          cnt           = 0                                                  ;
          readBuf [ n ] = 0                                                  ;
          s . append ( readBuf , n )                                         ;
        } else
        if ( nonFatalError ( ) )                                             {
          cnt++                                                              ;
          if ( cnt >= TRYTOREAD ) wouldBlock = true                          ;
        } else                                                               {
          if ( n < 0 ) return false                                          ;   // Error
          cnt++                                                              ;
          if ( cnt > 2 ) (*eof) = true                                       ;
        }                                                                    ;
      } else                                                                 {
        cnt++                                                                ;
        if ( cnt >= TRYTOREAD ) wouldBlock = true                            ;
      }                                                                      ;
    } else                                                                   {
      if ( nonFatalError ( ) )                                               {
        cnt++                                                                ;
        if ( cnt >= TRYTOREAD ) wouldBlock = true                            ;
      } else                                                                 {
        return false                                                         ;   // Error
      }                                                                      ;
    }                                                                        ;
    Time::msleep ( 1 )                                                       ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  return true                                                                ;
}

bool N::XmlRpcSource::nbWrite(int sfd,QByteArray & s,int * bytesSoFar,int utimeout)
{
  if ( NULL          == bytesSoFar ) return false                            ;
  if ( (*bytesSoFar) <  0          ) return false                            ;
  if ( s . size ( )  <= 0          ) return false                            ;
  ////////////////////////////////////////////////////////////////////////////
  struct timeval tv                                                          ;
  fd_set         wr                                                          ;
  fd_set         ed                                                          ;
  int            nToWrite   = int(s.size()) - (*bytesSoFar)                  ;
  bool           wouldBlock = false                                          ;
  char         * sp         = (char *) s . data ( )                          ;
  int            cnt        = 0                                              ;
  int            rt                                                          ;
  int            n                                                           ;
  ////////////////////////////////////////////////////////////////////////////
  if ( nToWrite <= 0 ) return true                                           ;
  ////////////////////////////////////////////////////////////////////////////
  tv . tv_sec  = utimeout / 1000000                                          ;
  tv . tv_usec = utimeout % 1000000                                          ;
  ////////////////////////////////////////////////////////////////////////////
  sp += (*bytesSoFar)                                                        ;
  while ( ( nToWrite > 0 ) && ( ! wouldBlock ) )                             {
    FD_ZERO ( &wr      )                                                     ;
    FD_ZERO ( &ed      )                                                     ;
    FD_SET  ( sfd , &wr )                                                    ;
    FD_SET  ( sfd , &ed )                                                    ;
    rt = ::select ( sfd + 1 , NULL , &wr , &ed , &tv )                       ;
    if ( FD_ISSET ( sfd , &ed ) ) return false                               ;
    if ( rt >= 0 )                                                           {
      if ( FD_ISSET ( sfd , &wr ) )                                          {
        n = ::send ( sfd , sp , nToWrite , 0)                                ;
        if ( n > 0 )                                                         {
          cnt          = 0                                                   ;
          sp          += n                                                   ;
          *bytesSoFar += n                                                   ;
          nToWrite    -= n                                                   ;
        } else
        if ( nonFatalError ( ) )                                             {
          cnt++                                                              ;
          if ( cnt >= TRYTOWRITE ) wouldBlock = true                         ;
        } else                                                               {
          if ( n < 0 ) return false                                          ;
        }                                                                    ;
      } else                                                                 {
        cnt++                                                                ;
        if ( cnt >= TRYTOWRITE ) wouldBlock = true                           ;
      }                                                                      ;
    } else                                                                   {
      if ( nonFatalError ( ) )                                               {
        cnt++                                                                ;
        if ( cnt >= TRYTOWRITE ) wouldBlock = true                           ;
      } else                                                                 {
        return false                                                         ;
      }                                                                      ;
    }                                                                        ;
    Time::msleep ( 1 )                                                       ;
  }                                                                          ;
  ////////////////////////////////////////////////////////////////////////////
  return true                                                                ;
}

int N::XmlRpcSource::getError(void)
{
  #ifdef Q_OS_WIN
  return ::WSAGetLastError ( ) ;
  #else
  return errno                 ;
  #endif
}

QString N::XmlRpcSource::getErrorMsg(void)
{
  return getErrorMsg ( getError ( ) ) ;
}

QString N::XmlRpcSource::getErrorMsg(int error)
{
  return QString ( "Error %1" ) . arg ( error ) ;
}

void N::XmlRpcSource::log(int level,const char * fmt,...)
{
  #ifdef XMLRPC_DEBUG
  if ( NULL == errBuf                             ) return   ;
  if ( NULL == fmt                                ) return   ;
  if ( level > XmlRpcLogHandler::getVerbosity ( ) ) return   ;
  va_list va                                                 ;
  char * buf = errBuf                                        ;
  ::memset  ( buf , 0    , LBUFSIZE     )                    ;
  va_start  ( va  , fmt                 )                    ;
  vsnprintf ( buf , SBUFSIZE , fmt , va )                    ;
  va_end    ( va                        )                    ;
  XmlRpcLogHandler::getLogHandler ( ) -> log ( level , buf ) ;
  #endif
}

void N::XmlRpcSource::error(const char * fmt,...)
{
  #ifdef XMLRPC_DEBUG
  if ( NULL == errBuf ) return                             ;
  if ( NULL == fmt    ) return                             ;
  va_list va                                               ;
  char * buf = errBuf                                      ;
  ::memset  ( buf , 0    , LBUFSIZE     )                  ;
  va_start  ( va  , fmt                 )                  ;
  vsnprintf ( buf , SBUFSIZE , fmt , va )                  ;
  va_end    ( va                        )                  ;
  XmlRpcErrorHandler::getErrorHandler ( ) -> error ( buf ) ;
  #endif
}
