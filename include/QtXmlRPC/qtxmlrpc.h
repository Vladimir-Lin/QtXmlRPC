/****************************************************************************
 *                                                                          *
 * Copyright (C) 2001 ~ 2016 Neutrino International Inc.                    *
 *                                                                          *
 * Author : Brian Lin <lin.foxman@gmail.com>, Skype: wolfram_lin            *
 *                                                                          *
 ****************************************************************************/

#ifndef QT_XMLRPC_H
#define QT_XMLRPC_H

#include <QtCore>
#include <QtNetwork>
#include <QtSql>
#include <QtScript>
#include <Essentials>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#    if defined(QT_BUILD_QTXMLRPC_LIB)
#      define Q_XMLRPC_EXPORT Q_DECL_EXPORT
#    else
#      define Q_XMLRPC_EXPORT Q_DECL_IMPORT
#    endif
#else
#      define Q_XMLRPC_EXPORT
#endif

namespace N                                  {

class Q_XMLRPC_EXPORT XmlRpcErrorHandler     ;
class Q_XMLRPC_EXPORT XmlRpcLogHandler       ;
class Q_XMLRPC_EXPORT XmlRpcSource           ;
class Q_XMLRPC_EXPORT XmlRpcUtil             ;
class Q_XMLRPC_EXPORT XmlRpcValue            ;
class Q_XMLRPC_EXPORT XmlRpcClient           ;
class Q_XMLRPC_EXPORT XmlRpcServerMethod     ;
class Q_XMLRPC_EXPORT XmlRpcServerConnection ;
class Q_XMLRPC_EXPORT XmlRpcServer           ;
class Q_XMLRPC_EXPORT RpcVirtualMethod       ;
class Q_XMLRPC_EXPORT RpcMethodExists        ;
class Q_XMLRPC_EXPORT RpcMethodHeartbeat     ;
class Q_XMLRPC_EXPORT Rpc                    ;
class Q_XMLRPC_EXPORT RpcCommands            ;

/*****************************************************************************\
 *                                                                           *
 *                     XML Remote Procedure Call / XML-RPC                   *
 *                                                                           *
\*****************************************************************************/

class Q_XMLRPC_EXPORT XmlRpcErrorHandler
{
  public:

    explicit XmlRpcErrorHandler                  (void) ;
    virtual ~XmlRpcErrorHandler                  (void) ;

    static  XmlRpcErrorHandler * getErrorHandler (void) ;
    static  void                 setErrorHandler (XmlRpcErrorHandler * eh) ;
    virtual void                 error           (const char * msg) = 0 ;

  protected:

    static XmlRpcErrorHandler * errorHandler ;

  private:

} ;

class Q_XMLRPC_EXPORT XmlRpcLogHandler
{
  public:

    explicit XmlRpcLogHandler                (void) ;
    virtual ~XmlRpcLogHandler                (void) ;

    static  XmlRpcLogHandler * getLogHandler (void) ;
    static  void               setLogHandler (XmlRpcLogHandler * lh) ;
    static  int                getVerbosity  (void) ;
    static  void               setVerbosity  (int v) ;
    virtual void               log           (int level,const char * msg) = 0 ;

  protected:

    static XmlRpcLogHandler * logHandler ;
    static int                verbosity  ;

  private:

} ;

class Q_XMLRPC_EXPORT XmlRpcSource
{
  public:

    bool       * Controller ;
    WMAPs        Variables  ;
    unsigned int flags      ;

    explicit         XmlRpcSource   (int fd = -1) ;
    virtual         ~XmlRpcSource   (void) ;

    int              socket         (void) ;
    void             close          (int socket) ;
    bool             setNonBlocking (int socket) ;
    bool             nbRead         (int socket,QByteArray & s, bool * eof       ,int utimeout = 100000) ;
    bool             nbWrite        (int socket,QByteArray & s, int  * bytesSoFar,int utimeout = 100000) ;
    bool             setReuseAddr   (int socket) ;
    bool             bind           (int socket,int port   ) ;
    bool             listen         (int socket,int backlog) ;
    int              accept         (int socket) ;
    bool             connect        (int socket,QString & host,int port) ;
    bool             nonFatalError  (void) ;
    int              getError       (void) ;
    QString          getErrorMsg    (void) ;
    QString          getErrorMsg    (int error) ;
    void             log            (int level,const char * fmt,...) ;
    void             error          (const char * fmt, ...) ;

    int              getfd          (void) const ;
    void             setfd          (int fd) ;
    bool             getKeepOpen    (void) const ;
    void             setKeepOpen    (bool b = true) ;
    virtual void     close          (void) ;
    virtual unsigned handleEvent    (unsigned eventType) = 0 ;
    virtual unsigned Processing     (bool processed,unsigned newMask) ;

    QString          parseTag       (const char    * tag,const QString & xml,int * offset) ;
    bool             findTag        (const char    * tag,const QString & xml,int * offset) ;
    QString          getNextTag     (const QString & xml,int * offset) ;
    bool             nextTagIs      (const char    * tag,const QString & xml,int * offset)  ;

  protected:

    int    READSIZE ;
    char * readBuf  ;
    char * errBuf   ;
    char * msgBuf   ;

  private:

    int  fd            ;
    bool deleteOnClose ;
    bool keepOpen      ;

} ;

class Q_XMLRPC_EXPORT XmlRpcValue
{
  public:

    enum Type             {
      TypeInvalid  =  0   ,
      TypeBoolean  =  1   ,
      TypeInt      =  2   ,
      TypeDouble   =  3   ,
      TypeString   =  4   ,
      TypeDateTime =  5   ,
      TypeBase64   =  6   ,
      TypeArray    =  7   ,
      TypeStruct   =  8   ,
      TypeTuid     =  9   ,
      TypeUuid     = 10   ,
      TypeEnd      = 11 } ;

    typedef QByteArray                    BinaryData  ;
    typedef QList < XmlRpcValue         > ValueArray  ;
    typedef QMap  < QString,XmlRpcValue > ValueStruct ;

  protected:

    Type        _type    ;
    bool        asBool   ;
    int         asInt    ;
    qint64      asTuid   ;
    quint64     asUuid   ;
    double      asDouble ;
    QString     asString ;
    QDateTime   asTime   ;
    BinaryData  asBinary ;
    ValueArray  asArray  ;
    ValueStruct asStruct ;

  public:

    explicit       XmlRpcValue (void                                  ) ;
    explicit       XmlRpcValue (bool                value             ) ;
    explicit       XmlRpcValue (int                 value             ) ;
    explicit       XmlRpcValue (qint64              value             ) ;
    explicit       XmlRpcValue (quint64             value             ) ;
    explicit       XmlRpcValue (double              value             ) ;
    explicit       XmlRpcValue (QDateTime           dateTime          ) ;
    explicit       XmlRpcValue (const QString     & value             ) ;
    explicit       XmlRpcValue (const char        * value             ) ;
    explicit       XmlRpcValue (void              * value,int   nBytes) ;
    explicit       XmlRpcValue (QString           & xml  ,int * offset) ;
                   XmlRpcValue (const XmlRpcValue & rhs               ) ;
    virtual       ~XmlRpcValue (void                                  ) ;

    virtual void   clear       (void) ;

    XmlRpcValue &  operator =  (const XmlRpcValue & rhs) ;
    XmlRpcValue &  operator =  (const bool        & rhs) ;
    XmlRpcValue &  operator =  (const int         & rhs) ;
    XmlRpcValue &  operator =  (const qint64      & rhs) ;
    XmlRpcValue &  operator =  (const quint64     & rhs) ;
    XmlRpcValue &  operator =  (const double      & rhs) ;
    XmlRpcValue &  operator =  (const char        * rhs) ;
    XmlRpcValue &  operator =  (const QString     & rhs) ;
    XmlRpcValue &  operator =  (const QDateTime   & rhs) ;

    bool           operator == (const XmlRpcValue & other) const ;
    bool           operator != (const XmlRpcValue & other) const ;

    const Type &   getType     (void) const ;
    bool           isType      (int type) ;

    operator bool        & ( ) { assertType ( TypeBoolean  ) ; return asBool   ; }
    operator int         & ( ) { assertType ( TypeInt      ) ; return asInt    ; }
    operator qint64      & ( ) { assertType ( TypeTuid     ) ; return asTuid   ; }
    operator quint64     & ( ) { assertType ( TypeUuid     ) ; return asUuid   ; }
    operator double      & ( ) { assertType ( TypeDouble   ) ; return asDouble ; }
    operator QString     & ( ) { assertType ( TypeString   ) ; return asString ; }
    operator QDateTime   & ( ) { assertType ( TypeDateTime ) ; return asTime   ; }
    operator BinaryData  & ( ) { assertType ( TypeBase64   ) ; return asBinary ; }

    const XmlRpcValue    & operator [ ] (int i) const ;
          XmlRpcValue    & operator [ ] (int i) ;

    XmlRpcValue          & operator [ ] (const QString & k) ;
    XmlRpcValue          & operator [ ] (const char    * k) ;

    bool           valid       (void) const ;
    int            size        (void) const ;
    void           setSize     (int size) ;

    bool           hasMember   (const QString & name) const ;

    bool           fromXml     (QString & valueXml,int * offset) ;
    QString        toXml       (void) ;

    std::ostream & write       (std::ostream & os) const ;

    QString Encode             (QString & raw) ;
    QString Decode             (QString & encoded) ;
    QString parseTag           (const char    * tag,const QString & xml,int * offset) ;
    bool    findTag            (const char    * tag,const QString & xml,int * offset) ;
    QString getNextTag         (const QString & xml,int * offset) ;
    bool    nextTagIs          (const char    * tag,const QString & xml,int * offset)  ;

  protected:

    void    invalidate         (void) ;

    void    assertType         (Type t) ;
    void    assertArray        (int size) ;
    void    assertStruct       (void) ;

    bool    boolFromXml        (QString & valueXml,int * offset) ;
    bool    intFromXml         (QString & valueXml,int * offset) ;
    bool    tuidFromXml        (QString & valueXml,int * offset) ;
    bool    uuidFromXml        (QString & valueXml,int * offset) ;
    bool    doubleFromXml      (QString & valueXml,int * offset) ;
    bool    stringFromXml      (QString & valueXml,int * offset) ;
    bool    timeFromXml        (QString & valueXml,int * offset) ;
    bool    binaryFromXml      (QString & valueXml,int * offset) ;
    bool    arrayFromXml       (QString & valueXml,int * offset) ;
    bool    structFromXml      (QString & valueXml,int * offset) ;

    QString boolToXml          (void) ;
    QString intToXml           (void) ;
    QString tuidToXml          (void) ;
    QString uuidToXml          (void) ;
    QString doubleToXml        (void) ;
    QString stringToXml        (void) ;
    QString timeToXml          (void) ;
    QString binaryToXml        (void) ;
    QString arrayToXml         (void) ;
    QString structToXml        (void) ;

  private:

} ;

class Q_XMLRPC_EXPORT XmlRpcClient : public XmlRpcSource
{
  public:

    explicit         XmlRpcClient    (const char * host              ,
                                      int          port              ,
                                      const char * uri = NULL      ) ;
    explicit         XmlRpcClient    (QString      host              ,
                                      int          port              ,
                                      QString      uri = QString() ) ;
    virtual         ~XmlRpcClient    (void) ;

    bool             execute         (QString       method   ,
                                      XmlRpcValue & params   ,
                                      XmlRpcValue & result ) ;
    bool             execute         (const char  * method   ,
                                      XmlRpcValue & params   ,
                                      XmlRpcValue & result ) ;
    bool             isFault         (void) const ;
    virtual void     close           (void) ;
    virtual unsigned handleEvent     (unsigned eventType) ;

  protected:

    virtual bool     doConnect       (void) ;
    virtual bool     setupConnection (void) ;
    virtual void     Transmit        (void) ;

    virtual bool     generateRequest (const char    * method  ,
                                      XmlRpcValue   & params) ;
    virtual QString  generateHeader  (const QString & body  ) ;
    virtual bool     writeRequest    (void) ;
    virtual bool     readHeader      (void) ;
    virtual bool     readResponse    (void) ;
    virtual bool     parseResponse   (XmlRpcValue & result) ;

    typedef enum            {
      NO_CONNECTION         ,
      CONNECTING            ,
      WRITE_REQUEST         ,
      READ_HEADER           ,
      READ_RESPONSE         ,
      IDLE                  }
      ClientConnectionState ;

    typedef enum            {
      ReadableEvent = 1     ,
      WritableEvent = 2     ,
      Exception     = 4     }
      EventType             ;

    ClientConnectionState connectionState ;
    QString               host            ;
    QString               uri             ;
    int                   port            ;
    QString               request         ;
    QString               header          ;
    QString               response        ;
    int                   bytesWritten    ;
    bool                  executing       ;
    bool                  eof             ;
    bool                  IsFault         ;
    int                   contentLength   ;

  private:

} ;

class Q_XMLRPC_EXPORT XmlRpcServerMethod
{
  public:

    explicit        XmlRpcServerMethod (const QString & name            ,
                                        XmlRpcServer  * server = NULL ) ;
    virtual        ~XmlRpcServerMethod (void);

    QString &       name               (void) ;
    virtual QString help               (void) ;

    virtual void    execute            (XmlRpcValue & params       ,
                                        XmlRpcValue & result ) = 0 ;

  protected:

    QString        Name   ;
    XmlRpcServer * Server ;

  private:

} ;

class Q_XMLRPC_EXPORT XmlRpcServerConnection : public XmlRpcSource
{
  public:

    explicit XmlRpcServerConnection (int fd,XmlRpcServer * server) ;
    virtual ~XmlRpcServerConnection (void);

    virtual unsigned handleEvent    (unsigned eventType) ;
    virtual unsigned Processing     (bool processed,unsigned newMask) ;

  protected:

    bool    readHeader              (void);
    bool    readRequest             (void);
    bool    writeResponse           (void);

    virtual void executeRequest     (void);

    QString parseRequest            (XmlRpcValue & params) ;

    bool    executeMethod           (const QString & methodName,XmlRpcValue & params,XmlRpcValue & result) ;
    bool    executeMulticall        (const QString & methodName,XmlRpcValue & params,XmlRpcValue & result) ;

    void    generateResponse        (const QString & resultXml) ;
    void    generateFaultResponse   (const QString & msg,int errorCode = -1) ;
    QString generateHeader          (const QString & body) ;

    XmlRpcServer * Server ;

    typedef enum            {
      READ_HEADER           ,
      READ_REQUEST          ,
      WRITE_RESPONSE        }
      ServerConnectionState ;

    typedef enum            {
      ReadableEvent = 1     ,
      WritableEvent = 2     ,
      Exception     = 4     }
      EventType             ;

    ServerConnectionState ConnectionState ;

    QString header        ;
    QString request       ;
    QString response      ;
    int     contentLength ;
    int     bytesWritten  ;
    bool    keepAlive     ;

  private:

} ;

class Q_XMLRPC_EXPORT XmlRpcServer : public XmlRpcSource
{
  public:

    explicit XmlRpcServer           (void) ;
    virtual ~XmlRpcServer           (void) ;

    void enableIntrospection        (bool enabled = true) ;
    void addMethod                  (XmlRpcServerMethod * method) ;
    void removeMethod               (XmlRpcServerMethod * method) ;
    void removeMethod               (const QString      & methodName) ;
    XmlRpcServerMethod * findMethod (const QString      & name) const ;
    bool bindAndListen              (int port,int backlog = 5) ;
    void work                       (double msTime) ;
    void exit                       (void) ;
    void shutdown                   (void) ;
    void listMethods                (XmlRpcValue& result) ;
    virtual unsigned handleEvent    (unsigned eventType) ;
    virtual void removeConnection   (XmlRpcServerConnection *) ;

  protected:

    typedef enum        {
      ReadableEvent = 1 ,
      WritableEvent = 2 ,
      Exception     = 4 }
      EventType         ;

    virtual void acceptConnection                     (void) ;
    virtual XmlRpcServerConnection * createConnection (int socket) ;

    bool                                    introspectionEnabled ;
    QMap < QString , XmlRpcServerMethod * > methods              ;
    XmlRpcServerMethod                  *   listmethods          ;
    XmlRpcServerMethod                  *   methodHelp           ;
    QList < XmlRpcSource                * > sources              ;

  private:

} ;

class Q_XMLRPC_EXPORT RpcVirtualMethod : public QObject
                                       , public XmlRpcServerMethod
{
  Q_OBJECT
  public:

    explicit    RpcVirtualMethod (QObject * parent,QString name,XmlRpcServer * s) ;
    virtual    ~RpcVirtualMethod (void);

    virtual int Type             (void) const = 0 ;

  protected:

  private:

  public slots:

  protected slots:

  private slots:

  signals:

} ;

class Q_XMLRPC_EXPORT RpcMethodExists : public RpcVirtualMethod
{
  Q_OBJECT
  public:

    explicit     RpcMethodExists (QObject * parent,XmlRpcServer * s) ;
    virtual     ~RpcMethodExists (void) ;

    virtual int  Type            (void) const ;

    virtual void execute         (XmlRpcValue & params   ,
                                  XmlRpcValue & result ) ;

  protected:

  private:

  public slots:

  protected slots:

    void acceptShowMaximized     (void);
    void acceptLogin             (QString login,QString hostname) ;

  private slots:

  signals:

    void safeShowMaximized       (void);
    void showMaximized           (void);
    void safeLogin               (QString login,QString hostname) ;
    void Login                   (QString login,QString hostname) ;

} ;

class Q_XMLRPC_EXPORT RpcMethodHeartbeat : public RpcVirtualMethod
{
  Q_OBJECT
  public:

    explicit     RpcMethodHeartbeat (QObject * parent,XmlRpcServer * s) ;
    virtual     ~RpcMethodHeartbeat (void) ;

    virtual int  Type               (void) const ;
    virtual void execute            (XmlRpcValue & params   ,
                                     XmlRpcValue & result ) ;

  protected:

  private:

  public slots:

  protected slots:

  private slots:

  signals:

} ;

class Q_XMLRPC_EXPORT Rpc : public Thread
{
  public:

    XmlRpcServer RPC ;

    explicit Rpc         (void) ;
             Rpc         (const Rpc & rpc) ;
    virtual ~Rpc         (void) ;

    void     setParent   (QObject * parent,const char * method,const char * login) ;
    bool     Start       (int port) ;
    bool     Stop        (void)     ;

  protected:

    RpcMethodExists    * Exists    ;
    RpcMethodHeartbeat * Heartbeat ;

    virtual void run     (int Type,ThreadData * data) ;

    virtual void Working (ThreadData * data) ;

  private:

} ;

class Q_XMLRPC_EXPORT RpcCommands : public Thread
{
  public:

    bool                      Alive      ;
    int                       Interval   ;
    QMap<SUID,XmlRpcClient *> Requests   ;
    SMAPs                     Heartbeats ;
    Mutexz                    mutex      ;

    explicit     RpcCommands     (void) ;
                 RpcCommands     (const RpcCommands & rpc) ;
    virtual     ~RpcCommands     (void) ;

    virtual void startRequesting (void) ;
    virtual bool Exists          (QString host,int port) ;
    virtual bool Login           (QString host,int port,QString username,QString hostname) ;
    virtual SUID addRequest      (SUID uuid,QString host,int port) ;
    virtual SUID addRequest      (SUID uuid,XmlRpcClient * requst) ;
    virtual bool removeRequest   (SUID uuid) ;
    virtual bool execute         (SUID          uuid     ,
                                  const char  * method   ,
                                  XmlRpcValue & params   ,
                                  XmlRpcValue & result ) ;

  protected:

    virtual void run             (int Type,ThreadData * data) ;

    virtual void Requesting      (ThreadData * data) ;

  private:

} ;

}

Q_XMLRPC_EXPORT std::ostream & operator << (std::ostream & os,N::XmlRpcValue & v) ;

Q_DECLARE_METATYPE(N::XmlRpcValue)

QT_END_NAMESPACE

#endif
