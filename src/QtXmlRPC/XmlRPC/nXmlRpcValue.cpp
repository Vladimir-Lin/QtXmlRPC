#include <qtxmlrpc.h>

#define XMLDBLFMT "%.12f"
#define AMP       QChar('&')

static const char   rawEntity   [ ] = {   '<',   '>',    '&',    '\'',    '\"', 0 } ;
static const char * xmlEntity   [ ] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 } ;
static const int    xmlEntLen   [ ] = {     3,     3,      4,       5,       5, 0 } ;

static const char VALUE_TAG     [ ] = "<value>"     ;
static const char VALUE_ETAG    [ ] = "</value>"    ;

static const char BOOLEAN_TAG   [ ] = "<boolean>"   ;
static const char BOOLEAN_ETAG  [ ] = "</boolean>"  ;
static const char DOUBLE_TAG    [ ] = "<double>"    ;
static const char DOUBLE_ETAG   [ ] = "</double>"   ;
static const char INT_TAG       [ ] = "<int>"       ;
static const char INT_ETAG      [ ] = "</int>"      ;
static const char I4_TAG        [ ] = "<i4>"        ;
static const char I4_ETAG       [ ] = "</i4>"       ;
static const char I8_TAG        [ ] = "<i8>"        ;
static const char I8_ETAG       [ ] = "</i8>"       ;
static const char U8_TAG        [ ] = "<u8>"        ;
static const char U8_ETAG       [ ] = "</u8>"       ;
static const char STRING_TAG    [ ] = "<string>"    ;
static const char STRING_ETAG   [ ] = "</string>"   ;
static const char DATETIME_TAG  [ ] = "<datetime>"  ;
static const char DATETIME_ETAG [ ] = "</datetime>" ;
static const char BASE64_TAG    [ ] = "<base64>"    ;
static const char BASE64_ETAG   [ ] = "</base64>"   ;

static const char ARRAY_TAG     [ ] = "<array>"     ;
static const char DATA_TAG      [ ] = "<data>"      ;
static const char DATA_ETAG     [ ] = "</data>"     ;
static const char ARRAY_ETAG    [ ] = "</array>"    ;

static const char STRUCT_TAG    [ ] = "<struct>"    ;
static const char MEMBER_TAG    [ ] = "<member>"    ;
static const char NAME_TAG      [ ] = "<name>"      ;
static const char NAME_ETAG     [ ] = "</name>"     ;
static const char MEMBER_ETAG   [ ] = "</member>"   ;
static const char STRUCT_ETAG   [ ] = "</struct>"   ;

N::XmlRpcValue:: XmlRpcValue (void       )
               : _type       (TypeInvalid)
               , asUuid      (0          )
{
}

N::XmlRpcValue:: XmlRpcValue ( bool v      )
               : _type       ( TypeBoolean )
               , asBool      (      v      )
{
}

N::XmlRpcValue:: XmlRpcValue ( int v   )
               : _type       ( TypeInt )
               , asInt       (     v   )
{
}

N::XmlRpcValue:: XmlRpcValue ( qint64 v )
               : _type       ( TypeTuid )
               , asTuid      (        v )
{
}

N::XmlRpcValue:: XmlRpcValue ( quint64 v )
               : _type       ( TypeUuid  )
               , asUuid      (         v )
{
}

N::XmlRpcValue:: XmlRpcValue ( QDateTime dateTime )
               : _type       ( TypeDateTime       )
               , asTime      (           dateTime )
{
}

N::XmlRpcValue:: XmlRpcValue ( double v   )
               : _type       ( TypeDouble )
               , asDouble    (        v   )
{
}

N::XmlRpcValue:: XmlRpcValue ( const QString & v )
               : _type       ( TypeString        )
               , asString    (                 v )
{
}

N::XmlRpcValue:: XmlRpcValue ( const char * v )
               : _type       ( TypeString     )
{
  asString = ""                      ;
  if ( NULL == v ) return            ;
  asString = QString::fromUtf8 ( v ) ;
}

N::XmlRpcValue:: XmlRpcValue ( void * v,int nBytes )
               : _type       ( TypeBase64          )
{
  asBinary . clear  (                           ) ;
  asBinary . append ( (const char *) v , nBytes ) ;
}

N::XmlRpcValue:: XmlRpcValue ( QString & xml , int * offset )
               : _type       ( TypeInvalid                  )
{
  if ( ! fromXml ( xml , offset ) ) _type = TypeInvalid ;
}

N::XmlRpcValue:: XmlRpcValue (const XmlRpcValue & rhs)
               : _type       (TypeInvalid            )
{
  (*this) = rhs ;
}

N::XmlRpcValue::~XmlRpcValue (void)
{
  clear ( ) ;
}

void N::XmlRpcValue::clear(void)
{
  invalidate ( ) ;
}

const N::XmlRpcValue & N::XmlRpcValue::operator [] (int i) const
{
  if ( TypeArray != _type       ) return (*this) ;
  if ( asArray . count ( ) >= i ) return (*this) ;
  return asArray [ i ]                           ;
}

N::XmlRpcValue & N::XmlRpcValue::operator [] (int i)
{
  if ( i >= asArray . count ( ) ) {
    assertArray ( i + 1 )         ;
  }                               ;
  return asArray [ i ]            ;
}

N::XmlRpcValue & N::XmlRpcValue::operator [] (const QString & k)
{
  assertStruct ( )                          ;
  if ( _type != TypeStruct ) return (*this) ;
  return asStruct [ k ]                     ;
}

N::XmlRpcValue & N::XmlRpcValue::operator [] (const char * k)
{
  assertStruct (   )                        ;
  if ( _type != TypeStruct ) return (*this) ;
  if ( NULL  == k          ) return (*this) ;
  QString s = QString::fromUtf8 ( k )       ;
  return asStruct [ s ]                     ;
}

bool N::XmlRpcValue::valid(void) const
{
  return ( _type != TypeInvalid ) ;
}

const N::XmlRpcValue::Type & N::XmlRpcValue::getType (void) const
{
  return _type ;
}

bool N::XmlRpcValue::isType(int type)
{
  return ( _type == (Type) type ) ;
}

void N::XmlRpcValue::setSize (int size)
{
  if ( TypeArray != _type ) return ;
  assertArray ( size )             ;
}

void N::XmlRpcValue::invalidate (void)
{
  _type    = TypeInvalid                    ;
  asBool   = false                          ;
  asInt    = 0                              ;
  asTuid   = 0                              ;
  asUuid   = 0                              ;
  asDouble = 0                              ;
  asString = ""                             ;
  asTime   = QDateTime::currentDateTime ( ) ;
  asBinary . clear                      ( ) ;
  asArray  . clear                      ( ) ;
  asStruct . clear                      ( ) ;
}

void N::XmlRpcValue::assertType(N::XmlRpcValue::Type t)
{
  if ( _type != TypeInvalid ) return ;
  _type = t                          ;
}

void N::XmlRpcValue::assertArray(int size)
{
  if ( _type == TypeInvalid )          {
    _type = TypeArray                  ;
    asArray . clear ( )                ;
  }                                    ;
  //////////////////////////////////////
  if ( _type != TypeArray   ) return   ;
  XmlRpcValue XRV                      ;
  //////////////////////////////////////
  if ( size <  0 ) return              ;
  //////////////////////////////////////
  if ( size == 0 )                     {
    asArray . clear ( )                ;
    return                             ;
  }                                    ;
  //////////////////////////////////////
  while ( asArray . count ( ) < size ) {
    asArray . append ( XRV )           ;
  }                                    ;
  //////////////////////////////////////
  while ( asArray . count ( ) > size ) {
    asArray . takeLast ( )             ;
  }                                    ;
}

void N::XmlRpcValue::assertStruct(void)
{
  if ( _type != TypeInvalid ) return ;
  _type = TypeStruct                 ;
}

N::XmlRpcValue & N::XmlRpcValue::operator = (const XmlRpcValue & rhs)
{
  if ( this == &rhs ) return (*this)                      ;
  invalidate ( )                                          ;
  _type = rhs . _type                                     ;
  switch ( _type )                                        {
    case TypeBoolean  : asBool   = rhs . asBool   ; break ;
    case TypeInt      : asInt    = rhs . asInt    ; break ;
    case TypeTuid     : asTuid   = rhs . asTuid   ; break ;
    case TypeUuid     : asUuid   = rhs . asUuid   ; break ;
    case TypeDouble   : asDouble = rhs . asDouble ; break ;
    case TypeDateTime : asTime   = rhs . asTime   ; break ;
    case TypeString   : asString = rhs . asString ; break ;
    case TypeBase64   : asBinary = rhs . asBinary ; break ;
    case TypeArray    : asArray  = rhs . asArray  ; break ;
    case TypeStruct   : asStruct = rhs . asStruct ; break ;
    default           : asUuid   = 0              ; break ;
  }                                                       ;
  return (*this)                                          ;
}

N::XmlRpcValue & N::XmlRpcValue::operator = (const bool & rhs)
{
  _type  = TypeBoolean ;
  asBool = rhs         ;
  return (*this)       ;
}

N::XmlRpcValue & N::XmlRpcValue::operator = (const int & rhs)
{
  _type = TypeInt ;
  asInt = rhs     ;
  return (*this)  ;
}

N::XmlRpcValue & N::XmlRpcValue::operator = (const qint64 & rhs)
{
  _type  = TypeTuid ;
  asTuid = rhs      ;
  return (*this)    ;
}

N::XmlRpcValue & N::XmlRpcValue::operator = (const quint64 & rhs)
{
  _type  = TypeUuid ;
  asUuid = rhs      ;
  return (*this)    ;
}

N::XmlRpcValue & N::XmlRpcValue::operator = (const double & rhs)
{
  _type    = TypeDouble ;
  asDouble = rhs        ;
  return (*this)        ;
}

N::XmlRpcValue & N::XmlRpcValue::operator = (const char * rhs)
{
  _type      = TypeString                ;
  if ( NULL == rhs )                     {
    asString = ""                        ;
  } else                                 {
    asString = QString::fromUtf8 ( rhs ) ;
  }                                      ;
  return (*this)                         ;
}

N::XmlRpcValue & N::XmlRpcValue::operator = (const QString & rhs)
{
  _type    = TypeString ;
  asString = rhs        ;
  return (*this)        ;
}

N::XmlRpcValue & N::XmlRpcValue::operator = (const QDateTime & rhs)
{
  _type  = TypeDateTime ;
  asTime = rhs          ;
  return (*this)        ;
}

bool N::XmlRpcValue::operator == (const XmlRpcValue & other) const
{
  if (_type != other . _type ) return false                            ;
  switch ( _type )                                                     {
    case TypeBoolean  : return asBool   == other . asBool              ;
    case TypeInt      : return asInt    == other . asInt               ;
    case TypeTuid     : return asTuid   == other . asTuid              ;
    case TypeUuid     : return asUuid   == other . asUuid              ;
    case TypeDouble   : return asDouble == other . asDouble            ;
    case TypeDateTime : return asTime   == other . asTime              ;
    case TypeString   : return asString == other . asString            ;
    case TypeBase64   : return asBinary == other . asBinary            ;
    case TypeArray    : return asArray  == other . asArray             ;
    case TypeStruct   :                                                {
      if ( asStruct . count ( ) == other . asStruct . count ( ) )      {
        QStringList SSL =         asStruct . keys ( )                  ;
        QStringList SSO = other . asStruct . keys ( )                  ;
        if ( SSL . count ( ) != SSO . count ( ) ) return false         ;
        if (SSL        !=SSO        ) return false                     ;
        for (int i = 0 ; i < SSL . count ( ) ; i++ )                   {
          QString S = SSL [ i ]                                        ;
          if ( asStruct [ S ] != other . asStruct [ S ] ) return false ;
        }                                                              ;
      } else return false                                              ;
      return true                                                      ;
    }                                                                  ;
    default: break                                                     ;
  }                                                                    ;
  return true                                                          ;
}

bool N::XmlRpcValue::operator != (const XmlRpcValue & other) const
{
  return ! ( (*this) == other ) ;
}

int N::XmlRpcValue::size(void) const
{
  switch                         ( _type                 ) {
    case TypeString : return int ( asString . length ( ) ) ;
    case TypeBase64 : return int ( asBinary . size   ( ) ) ;
    case TypeArray  : return int ( asArray  . count  ( ) ) ;
    case TypeStruct : return int ( asStruct . count  ( ) ) ;
    default: break                                         ;
  }                                                        ;
  //////////////////////////////////////////////////////////
  return 0                                                 ;
}

bool N::XmlRpcValue::hasMember(const QString & name) const
{
  if ( _type != TypeStruct ) return false ;
  return asStruct . contains ( name )     ;
}

bool N::XmlRpcValue::fromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset     ) return false                                    ;
  int xmlen = valueXml . length ( )                                         ;
  if ( (*offset) >= xmlen ) return false                                    ;
  ///////////////////////////////////////////////////////////////////////////
  int savedOffset = ( *offset )                                             ;
  invalidate ( )                                                            ;
  if ( ! nextTagIs ( VALUE_TAG, valueXml, offset ) ) return false           ;
  int     afterValueOffset = ( *offset )                                    ;
  QString typeTag          = getNextTag ( valueXml , offset )               ;
  bool    result           = false                                          ;
  ///////////////////////////////////////////////////////////////////////////
  if ( typeTag .  isEmpty ( )  ) result = stringFromXml ( valueXml,offset ) ;
  else
  if ( typeTag == STRING_TAG   ) result = stringFromXml ( valueXml,offset ) ;
  else
  if ( typeTag == BOOLEAN_TAG  ) result = boolFromXml   ( valueXml,offset ) ;
  else
  if ( typeTag == I4_TAG       ) result = intFromXml    ( valueXml,offset ) ;
  else
  if ( typeTag == I8_TAG       ) result = tuidFromXml   ( valueXml,offset ) ;
  else
  if ( typeTag == U8_TAG       ) result = uuidFromXml   ( valueXml,offset ) ;
  else
  if ( typeTag == INT_TAG      ) result = intFromXml    ( valueXml,offset ) ;
  else
  if ( typeTag == DOUBLE_TAG   ) result = doubleFromXml ( valueXml,offset ) ;
  else
  if ( typeTag == DATETIME_TAG ) result = timeFromXml   ( valueXml,offset ) ;
  else
  if ( typeTag == BASE64_TAG   ) result = binaryFromXml ( valueXml,offset ) ;
  else
  if ( typeTag == ARRAY_TAG    ) result = arrayFromXml  ( valueXml,offset ) ;
  else
  if ( typeTag == STRUCT_TAG   ) result = structFromXml ( valueXml,offset ) ;
  else
  if ( typeTag == VALUE_ETAG   )                                            {
    *offset = afterValueOffset                                              ;
    result  = stringFromXml ( valueXml , offset )                           ;
  }                                                                         ;
  ///////////////////////////////////////////////////////////////////////////
  if ( result ) findTag ( VALUE_ETAG , valueXml , offset )                  ;
           else *offset = savedOffset                                       ;
  ///////////////////////////////////////////////////////////////////////////
  return result                                                             ;
}

QString N::XmlRpcValue::toXml(void)
{
  switch ( _type )                             {
    case TypeBoolean  : return boolToXml   ( ) ;
    case TypeInt      : return intToXml    ( ) ;
    case TypeTuid     : return tuidToXml   ( ) ;
    case TypeUuid     : return uuidToXml   ( ) ;
    case TypeDouble   : return doubleToXml ( ) ;
    case TypeString   : return stringToXml ( ) ;
    case TypeDateTime : return timeToXml   ( ) ;
    case TypeBase64   : return binaryToXml ( ) ;
    case TypeArray    : return arrayToXml  ( ) ;
    case TypeStruct   : return structToXml ( ) ;
    default           : break                  ;
  }                                            ;
  return ""                                    ;
}

bool N::XmlRpcValue::boolFromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset ) return false       ;
  //////////////////////////////////////////
  int     BL = valueXml . length ( )       ;
  int     OS = *offset                     ;
  QString BS                               ;
  bool    BM                               ;
  //////////////////////////////////////////
  do                                       {
    QChar c = valueXml . at ( OS )         ;
    BM = c . isDigit ( )                   ;
    if ( BM )                              {
      BS . append ( c )                    ;
      OS++                                 ;
      BM = ( OS < BL )                     ;
    }                                      ;
  } while ( BM )                           ;
  //////////////////////////////////////////
  if ( BS . length ( ) <= 0 ) return false ;
  BM = true                                ;
  BL = BS . toInt ( &BM )                  ;
  if ( BM )                                {
    *offset = OS                           ;
  } else return false                      ;
  //////////////////////////////////////////
  _type  = TypeBoolean                     ;
  asBool = ( BL == 1 )                     ;
  return true                              ;
}

QString N::XmlRpcValue::boolToXml(void)
{
  QString xml                                     ;
  xml  = QString::fromUtf8 ( VALUE_TAG          ) ;
  xml += QString::fromUtf8 ( BOOLEAN_TAG        ) ;
  xml +=                   ( asBool ? "1" : "0" ) ;
  xml += QString::fromUtf8 ( BOOLEAN_ETAG       ) ;
  xml += QString::fromUtf8 ( VALUE_ETAG         ) ;
  return  xml                                     ;
}

bool N::XmlRpcValue::intFromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset ) return false       ;
  //////////////////////////////////////////
  int     BL = valueXml . length ( )       ;
  int     OS = *offset                     ;
  QString BS                               ;
  bool    BM                               ;
  //////////////////////////////////////////
  do                                       {
    QChar c = valueXml . at ( OS )         ;
    if ( c == QChar('-') ) BM = true       ;
    else BM = c . isDigit ( )              ;
    if ( BM )                              {
      BS . append ( c )                    ;
      OS++                                 ;
      BM = ( OS < BL )                     ;
    }                                      ;
  } while ( BM )                           ;
  //////////////////////////////////////////
  if ( BS . length ( ) <= 0 ) return false ;
  BM = true                                ;
  BL = BS . toInt ( &BM )                  ;
  if ( BM )                                {
    *offset = OS                           ;
  } else return false                      ;
  //////////////////////////////////////////
  _type = TypeInt                          ;
  asInt =  BL                              ;
  return true                              ;
}

QString N::XmlRpcValue::intToXml(void)
{
  QString xml                             ;
  xml  = QString::fromUtf8 ( VALUE_TAG  ) ;
  xml += QString::fromUtf8 ( I4_TAG     ) ;
  xml += QString::number   ( asInt      ) ;
  xml += QString::fromUtf8 ( I4_ETAG    ) ;
  xml += QString::fromUtf8 ( VALUE_ETAG ) ;
  return  xml                             ;
}

bool N::XmlRpcValue::tuidFromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset ) return false       ;
  //////////////////////////////////////////
  qint64  BL = valueXml . length ( )       ;
  int     OS = *offset                     ;
  QString BS                               ;
  bool    BM                               ;
  //////////////////////////////////////////
  do                                       {
    QChar c = valueXml . at ( OS )         ;
    if ( c == QChar('-') ) BM = true       ;
    else BM = c . isDigit ( )              ;
    if ( BM )                              {
      BS . append ( c )                    ;
      OS++                                 ;
      BM = ( OS < BL )                     ;
    }                                      ;
  } while ( BM )                           ;
  //////////////////////////////////////////
  if ( BS . length ( ) <= 0 ) return false ;
  BM = true                                ;
  BL = BS . toLongLong ( &BM )             ;
  if ( BM )                                {
    *offset = OS                           ;
  } else return false                      ;
  //////////////////////////////////////////
  _type  = TypeTuid                        ;
  asTuid =  BL                             ;
  return true                              ;
}

QString N::XmlRpcValue::tuidToXml(void)
{
  QString xml                             ;
  xml  = QString::fromUtf8 ( VALUE_TAG  ) ;
  xml += QString::fromUtf8 ( I8_TAG     ) ;
  xml += QString::number   ( asTuid     ) ;
  xml += QString::fromUtf8 ( I8_ETAG    ) ;
  xml += QString::fromUtf8 ( VALUE_ETAG ) ;
  return  xml                             ;
}

bool N::XmlRpcValue::uuidFromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset ) return false       ;
  //////////////////////////////////////////
  quint64 BL = valueXml . length ( )       ;
  int     OS = *offset                     ;
  QString BS                               ;
  bool    BM                               ;
  //////////////////////////////////////////
  do                                       {
    QChar c = valueXml . at ( OS )         ;
    BM = c . isDigit ( )                   ;
    if ( BM )                              {
      BS . append ( c )                    ;
      OS++                                 ;
      BM = ( OS < BL )                     ;
    }                                      ;
  } while ( BM )                           ;
  //////////////////////////////////////////
  if ( BS . length ( ) <= 0 ) return false ;
  BM = true                                ;
  BL = BS . toULongLong ( &BM )            ;
  if ( BM )                                {
    *offset = OS                           ;
  } else return false                      ;
  //////////////////////////////////////////
  _type  = TypeUuid                        ;
  asUuid =  BL                             ;
  return true                              ;
}

QString N::XmlRpcValue::uuidToXml(void)
{
  QString xml                             ;
  xml  = QString::fromUtf8 ( VALUE_TAG  ) ;
  xml += QString::fromUtf8 ( U8_TAG     ) ;
  xml += QString::number   ( asUuid     ) ;
  xml += QString::fromUtf8 ( U8_ETAG    ) ;
  xml += QString::fromUtf8 ( VALUE_ETAG ) ;
  return  xml                             ;
}

bool N::XmlRpcValue::doubleFromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset ) return false       ;
  //////////////////////////////////////////
  int     BL = valueXml . length ( )       ;
  int     OS = *offset                     ;
  double  BD = 0                           ;
  QString BS                               ;
  bool    BM                               ;
  //////////////////////////////////////////
  do                                       {
    QChar c = valueXml . at ( OS )         ;
    if ( c == QChar('.') ) BM = true  ; else
    if ( c == QChar('-') ) BM = true       ;
    else BM = c . isDigit ( )              ;
    if ( BM )                              {
      BS . append ( c )                    ;
      OS++                                 ;
      BM = ( OS < BL )                     ;
    }                                      ;
  } while ( BM )                           ;
  //////////////////////////////////////////
  if ( BS . length ( ) <= 0 ) return false ;
  BM = true                                ;
  BD = BS . toDouble ( &BM )               ;
  if ( BM )                                {
    *offset = OS                           ;
  } else return false                      ;
  //////////////////////////////////////////
  _type    = TypeDouble                    ;
  asDouble = BD                            ;
  return true                              ;
}

QString N::XmlRpcValue::doubleToXml(void)
{
  QString xml                                       ;
  QString buf                                       ;
  buf . sprintf            ( XMLDBLFMT , asDouble ) ;
  xml  = QString::fromUtf8 ( VALUE_TAG            ) ;
  xml += QString::fromUtf8 ( DOUBLE_TAG           ) ;
  xml += buf                                        ;
  xml += QString::fromUtf8 ( DOUBLE_ETAG          ) ;
  xml += QString::fromUtf8 ( VALUE_ETAG           ) ;
  return xml                                        ;
}

bool N::XmlRpcValue::stringFromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset ) return false                   ;
  int epos = valueXml . indexOf ( '<' , *offset )      ;
  if ( epos < 0 ) return false                         ;
  _type           = TypeString                         ;
  QString dec = valueXml.mid((*offset),epos-(*offset)) ;
  asString    = Decode ( dec )                         ;
  *offset    += dec.length()                           ;
  return true                                          ;
}

QString N::XmlRpcValue::stringToXml(void)
{
  QString xml                              ;
  xml  = QString::fromUtf8 ( VALUE_TAG   ) ;
  xml += QString::fromUtf8 ( STRING_TAG  ) ;
  xml += Encode            ( asString    ) ;
  xml += QString::fromUtf8 ( STRING_ETAG ) ;
  xml += QString::fromUtf8 ( VALUE_ETAG  ) ;
  return  xml                              ;
}

bool N::XmlRpcValue::timeFromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset ) return false                              ;
  int epos = valueXml . indexOf ( '<' , *offset )                 ;
  if ( epos < 0 ) return false                                    ;
  QString stime = valueXml . mid ( (*offset) , epos - (*offset) ) ;
  bool    ok    = false                                           ;
  qint64  ast   = stime . toLongLong ( & ok )                     ;
  if ( ! ok     ) return false                                    ;
  asTime   = QDateTime::fromMSecsSinceEpoch ( ast )               ;
  _type    = TypeDateTime                                         ;
  *offset += stime . length ( )                                   ;
  return true                                                     ;
}

QString N::XmlRpcValue::timeToXml(void)
{
  QString xml                                                 ;
  xml  = QString::fromUtf8 ( VALUE_TAG                      ) ;
  xml += QString::fromUtf8 ( DATETIME_TAG                   ) ;
  xml += QString::number   ( asTime . toMSecsSinceEpoch ( ) ) ;
  xml += QString::fromUtf8 ( DATETIME_ETAG                  ) ;
  xml += QString::fromUtf8 ( VALUE_ETAG                     ) ;
  return  xml                                                 ;
}

bool N::XmlRpcValue::binaryFromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset ) return false                      ;
  int epos = valueXml.indexOf('<',*offset)                ;
  if ( epos < 0 ) return false                            ;
  QString    ass = valueXml.mid((*offset),epos-(*offset)) ;
  QByteArray ASB = ass . toUtf8 ( )                       ;
  _type          = TypeBase64                             ;
  asBinary       = QByteArray::fromBase64 ( ASB )         ;
  *offset       += ass . length ( )                       ;
  return true                                             ;
}

QString N::XmlRpcValue::binaryToXml(void)
{
  QByteArray ASB = asBinary . toBase64 ( ) ;
  QString    xml                           ;
  xml  = QString::fromUtf8 ( VALUE_TAG   ) ;
  xml += QString::fromUtf8 ( BASE64_TAG  ) ;
  xml += QString::fromUtf8 ( ASB         ) ;
  xml += QString::fromUtf8 ( BASE64_ETAG ) ;
  xml += QString::fromUtf8 ( VALUE_ETAG  ) ;
  return     xml                           ;
}

bool N::XmlRpcValue::arrayFromXml(QString & valueXml,int * offset)
{
  if ( NULL == offset                               ) return false ;
  if ( ! nextTagIs ( DATA_TAG , valueXml , offset ) ) return false ;
  _type   = TypeArray                                              ;
  asArray . clear ( )                                              ;
  bool c = true                                                    ;
  while ( c )                                                      {
    XmlRpcValue v                                                  ;
    c = v . fromXml ( valueXml , offset )                          ;
    if ( c ) asArray . append( v )                                 ;
  }                                                                ;
  return nextTagIs ( DATA_ETAG , valueXml , offset )               ;
}

QString N::XmlRpcValue::arrayToXml(void)
{
  QString xml                               ;
  xml  = QString::fromUtf8 ( VALUE_TAG    ) ;
  xml += QString::fromUtf8 ( ARRAY_TAG    ) ;
  xml += QString::fromUtf8 ( DATA_TAG     ) ;
  int s = asArray . count  (              ) ;
  for (int i = 0 ; i < s ; ++i )            {
    xml += asArray [ i ] . toXml ( )        ;
  }                                         ;
  xml += QString::fromUtf8 ( DATA_ETAG    ) ;
  xml += QString::fromUtf8 ( ARRAY_ETAG   ) ;
  xml += QString::fromUtf8 ( VALUE_ETAG   ) ;
  return xml                                ;
}

bool N::XmlRpcValue::structFromXml(QString & valueXml,int * offset)
{
  _type    = TypeStruct                                      ;
  asStruct . clear ( )                                       ;
  while ( nextTagIs ( MEMBER_TAG , valueXml , offset ) )     {
    QString name = parseTag ( NAME_TAG , valueXml , offset ) ;
    XmlRpcValue val ( valueXml , offset )                    ;
    if ( ! val . valid ( ) )                                 {
      invalidate ( )                                         ;
      return false                                           ;
    }                                                        ;
    asStruct [ name ] = val                                  ;
    if ( ! nextTagIs ( MEMBER_ETAG , valueXml , offset ) )   {
      invalidate ( )                                         ;
      return false                                           ;
    }                                                        ;
  }                                                          ;
  return true                                                ;
}

QString N::XmlRpcValue::structToXml(void)
{
  QString xml                                  ;
  xml    = QString::fromUtf8 ( VALUE_TAG     ) ;
  xml   += QString::fromUtf8 ( STRUCT_TAG    ) ;
  QStringList SSO = asStruct . keys ( )        ;
  for (int i = 0 ; i < SSO . count ( ) ; i++ ) {
    QString S = SSO [ i ]                      ;
    xml += QString::fromUtf8 ( MEMBER_TAG    ) ;
    xml += QString::fromUtf8 ( NAME_TAG      ) ;
    xml += Encode            ( S             ) ;
    xml += QString::fromUtf8 ( NAME_ETAG     ) ;
    xml += asStruct [ S ] . toXml ( )          ;
    xml += QString::fromUtf8 ( MEMBER_ETAG   ) ;
  }                                            ;
  xml   += QString::fromUtf8 ( STRUCT_ETAG   ) ;
  xml   += QString::fromUtf8 ( VALUE_ETAG    ) ;
  return  xml                                  ;
}

std::ostream & N::XmlRpcValue::write(std::ostream & os) const
{
  switch ( _type )                                                           {
    case TypeBoolean  : os << asBool                     ; break             ;
    case TypeInt      : os << asInt                      ; break             ;
    case TypeTuid     : os << asTuid                     ; break             ;
    case TypeDouble   : os << asDouble                   ; break             ;
    case TypeString   : os << asString . toStdString ( ) ; break             ;
    case TypeDateTime :                                                      {
      QString buf = asTime . toString ( "yyyy/MM/dd hh:mm:ss.zzz" )          ;
      os << buf . toStdString ( )                                            ;
    }                                                                        ;
    break                                                                    ;
    case TypeBase64   :                                                      {
      QByteArray B64 = asBinary . toBase64 (     )                           ;
      QString    S64 = QString::fromUtf8   ( B64 )                           ;
      os << S64 . toStdString ( )                                            ;
    }                                                                        ;
    break                                                                    ;
    case TypeArray    :                                                      {
      int s = asArray . count ( )                                            ;
      os << '{'                                                              ;
      for (int i=0;i<s;++i)                                                  {
        if (i > 0) os << ','                                                 ;
        asArray . at ( i ) . write ( os )                                    ;
      }                                                                      ;
      os << '}'                                                              ;
    }                                                                        ;
    break                                                                    ;
    case TypeStruct   :                                                      {
      os << '['                                                              ;
      QStringList SSO = asStruct . keys ( )                                  ;
      for (int i=0;i<SSO.count();i++)                                        {
        QString S = SSO [ i ]                                                ;
        if (i>0) os << ','                                                   ;
        os << S.toStdString() << ':'                                         ;
        asStruct [ S ] . write ( os )                                        ;
      }                                                                      ;
      os << ']'                                                              ;
    }                                                                        ;
    break                                                                    ;
    default                                                                  :
    break                                                                    ;
  }                                                                          ;
  return os                                                                  ;
}

QString N::XmlRpcValue::Decode(QString & encoded)
{
  int iAmp = encoded . indexOf ( AMP )                                   ;
  if ( iAmp < 0 ) return encoded                                         ;
  QString decoded = encoded . left   ( iAmp )                            ;
  int     iSize   = encoded . length (      )                            ;
  ////////////////////////////////////////////////////////////////////////
  while ( iAmp != iSize )                                                {
    if ( ( AMP == encoded . at ( iAmp ) ) && ( ( iAmp + 1 ) < iSize ) )  {
      int iEntity = 0                                                    ;
      while ( 0 != xmlEntity [ iEntity ] )                               {
        QString amp = encoded . mid ( iAmp + 1 , xmlEntLen [ iEntity ] ) ;
        QString ent = QString ( xmlEntity [ iEntity ] )                  ;
        if ( amp == ent )                                                {
          decoded . append ( rawEntity [ iEntity ] )                     ;
          iAmp   += xmlEntLen [ iEntity ] + 1                            ;
          break                                                          ;
        }                                                                ;
        ++iEntity                                                        ;
      }                                                                  ;
      if ( 0 == xmlEntity [ iEntity ] )                                  {
        decoded . append ( encoded . at ( iAmp ) )                       ;
        iAmp++                                                           ;
      }                                                                  ;
    } else                                                               {
      decoded . append ( encoded . at ( iAmp ) )                         ;
      iAmp++                                                             ;
    }                                                                    ;
  }                                                                      ;
  ////////////////////////////////////////////////////////////////////////
  return decoded                                                         ;
}

QString N::XmlRpcValue::Encode(QString & raw)
{
  if ( raw . length ( ) <= 0 ) return ""                          ;
  QString RES   ( rawEntity )                                     ;
  RES . prepend ( '['       )                                     ;
  RES . append  ( ']'       )                                     ;
  /////////////////////////////////////////////////////////////////
  QRegExp REX ( RES )                                             ;
  int iRep = raw . indexOf ( REX )                                ;
  if ( iRep < 0 ) return raw                                      ;
  /////////////////////////////////////////////////////////////////
  QString encoded = raw . left ( iRep )                           ;
  int     iSize   = raw . size (      )                           ;
  while ( iRep != iSize )                                         {
    int iEntity = 0                                               ;
    while ( 0 != rawEntity [ iEntity ] )                          {
      if ( raw . at ( iRep ) == QChar ( rawEntity [ iEntity ] ) ) {
        encoded . append ( AMP                   )                ;
        encoded . append ( xmlEntity [ iEntity ] )                ;
        break                                                     ;
      }                                                           ;
      ++iEntity                                                   ;
    }                                                             ;
    if ( 0 == rawEntity [ iEntity ] )                             {
      encoded . append ( raw . at ( iRep ) )                      ;
    }                                                             ;
    ++iRep                                                        ;
  }                                                               ;
  /////////////////////////////////////////////////////////////////
  return encoded                                                  ;
}


QString N::XmlRpcValue::parseTag(const char * tag,const QString & xml,int * offset)
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

bool N::XmlRpcValue::findTag(const char * tag,const QString & xml,int * offset)
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

bool N::XmlRpcValue::nextTagIs(const char * tag,const QString & xml,int * offset)
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

QString N::XmlRpcValue::getNextTag(const QString & xml,int * offset)
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

std::ostream & operator << (std::ostream & os,N::XmlRpcValue & v)
{
  return os << v . toXml ( ) . toStdString ( ) ;
}
