#include <iostream>
#include <cstring>
/**
 * @brief 引用计数
 * 目的：等值对象各自拥有自己的数据，一方面会造成空间的浪费，另一方面，多个等值对象在每次使用时都会存在构造和析构的成员，这对效率而言实在是一种浪费。
 * 1. 引用计数的实现细节
 * !强调引用计数是属于值的，而不是属于对象的。也就是说，值和引用计数是耦合的。
 */

#define COPY_ON_WRITE

#pragma region ReferenceCountImplementation
class String
{
public:
    String( const char* initValue );
    String( const String& rhs );
    ~String( );
    String& operator=( const String& rhs ); // non-const operator[] : 拷贝赋值运算符需要考虑左侧对象引用计数自减并考虑是否需要释放左侧对象的内存
#ifdef COPY_ON_WRITE
    char& operator[]( int index );
#endif
private:
    struct StringValue
    {
        StringValue( const char* initValue );
        ~StringValue( );
        char* data;
        int   refCount;
#ifdef COPY_ON_WRITE
        bool shareable;
#endif
    };
    StringValue* value;
};

String::StringValue::StringValue( const char* initValue )
    : refCount( 1 )
#ifdef COPY_ON_WRITE
      ,
      shareable( true )
#endif
{
    data = new char[strlen( initValue ) + 1];
    strcpy( data, initValue );
}
String::StringValue::~StringValue( ) { delete[] data; }

String::String( const char* initValue )
    : value( new StringValue( initValue ) ) // 分开构造但初值相同，并不共享同一份
{
}

String::String( const String& rhs ) // 拷贝构造函数只负责拷贝一份指针和增加一次引用计数
{
#ifdef COPY_ON_WRITE
    if ( value->shareable )
    {
        value = rhs.value;
        ++value->refCount;
    }
    else
        value = new StringValue( rhs.value->data );
#else
    value = rhs.value;
    ++value->refCount;
#endif
}

String::~String( )
{
    if ( --value->refCount == 0 ) delete value;// 当引用计数为0时，调用真正的 StringValue 析构函数
}

String& String::operator=( const String& rhs )
{
    /* s1=s2时，s1放弃原来指向的数值，指向s2指向的数值，同时增加s2指向的数值的引用计数，减少s1指向的数值的引用计数。 */
    if ( this->value == rhs.value ) return *this;
    if ( --this->value->refCount == 0 ) delete this->value;
    this->value = rhs.value;
    ++this->value->refCount;
    return *this;
}

#pragma endregion

#pragma region CopyOnWrite && LazyEvaluation
//! 引用计数考虑到 operator[] 时，可能引发意想不到的行为。为什么？
// String s1 = "Hello";  char* p=&s1[0];   String s2=s1;    *p='W';  // 该代码本来只想更改s1，但s2也同时被更改了。
// 另外，如果有人将 non-const operator[] 返回的引用保存起来，同样可以随便修改代码。
// 目前程序库采用了三种办法处理此问题：1. 不理睬 2. Doc 中给出警告
// 3. 为每个 StringValue 实例添加一个 bool 标志位，用于标记是否可共享，一旦 non-const operator[] 作用于对象身上，就将标志清除，后续永远不再让用户改变状态
#ifdef COPY_ON_WRITE
char& String::operator[]( int index )   // 一旦调用该函数，悲观假设会修改对象，立刻产生新对象，并减少引用计数，将标志位设置为false，后续不再共享该对象
{
    if ( value->refCount > 1 )
    {
        --value->refCount;
        value = new StringValue( value->data );
    }
    value->shareable = false;
    return value->data[index];
}
#endif
#pragma endregion

#pragma region RefCountBaseClass && Auto RefCount
//*===================================================RCObject==================================================*
class RCObject
{
public:
    RCObject( )
        : refCount( 0 ),
          shareable( true )
    {
    }   // 为什么 refCount 初始化为0？
    RCObject( const RCObject& rhs ) // 拷贝构造的语义是创建一个全新独立的值对象，新对象引用计数强制初始化为 0，完全独立于源对象。
        : refCount( 0 ),
          shareable( true )
    {
    }    // 为什么 refCount 初始化为 0？
    // 为什么不适用默认的拷贝赋值？默认合成的拷贝赋值运算符会将 RCObject 中的数据成员逐个拷贝，这意味这，当误写 sv1 = sv2 时，sv1 的引用计数也会被拷贝，原本的 sv1 引用计数不减，拷贝后的引用计数不增。
    // 为什么不设为 private？如果基类把 operator= 设为 private，派生类无法重载覆盖赋值运算符，未来若某个派生自RCObject的类确实需要支持实例间赋值（书中提及的扩展场景），私有赋值会直接锁死扩展能力
    // 假设一些类在将来可能从RCObject派生出来，并希望允许被引用计数的值被赋值，RCObject的赋值运算符并没有实现更改值而不更改引用计数的情况
    // 这是因为：未来某个派生类需要支持实例赋值，全部逻辑由派生类自己重载operator=实现，基类空实现不会干扰它，基类没有权限、也没有能力去拷贝派生类独有的业务内容。

    /** RCObject 的两个成员 refCount、shareable 完全保持 this 对象自身原来的值，不会复制 rhs 的 refCount/shareable
     * StringValue& StringValue::operator=(const StringValue& rhs) {
     * if (this == &rhs) return *this;
     * delete[] data;
     * size_t len = strlen(rhs.data);
     * data = new char[len + 1];
     * strcpy(data, rhs.data);
     * return *this;// 无任何父类赋值调用 RCObject::operator=() 全程没有执行，空函数逻辑不会跑一遍
     * } 贴合原文设计意图：底层值对象互相拷贝内容时，引用计数不受干扰
     */
    RCObject& operator=( const RCObject& rhs ) { return *this; }    // 为什么什么也不做？看着好像不对劲。
    virtual ~RCObject( ) = 0;    // 析构函数纯虚，表示只被用于作为基类
    void addRef( ) { ++refCount; }
    void removeRef( )
    {
        if ( --refCount == 0 ) delete this;
    }
    void markUnShareable( ) { shareable = false; }
    bool isShareable( ) const { return shareable; }
    bool isShared( ) const { return refCount > 1; }

private:
    int  refCount;
    bool shareable;
};
RCObject::~RCObject( ) = default;    // 析构函数即便纯虚，也要实现出来

//*===================================================RCPtr==================================================*

// RCPtr 作为自动引用计数的类存在，T 代表实际需要被引用计数的类型
template <typename T>
class RCPtr
{
public:
    RCPtr( T* realPtr = 0 );
    RCPtr( const RCPtr& rhs );
    ~RCPtr( );
    RCPtr& operator=( const RCPtr& rhs );
    T*     operator->( ) const;
    T&     operator*( ) const;

private:
    T*   pointee;
    void init( );
};

template <typename T>
RCPtr<T>::RCPtr( T* realPtr )
    : pointee( realPtr )
{
    init( );
}

template <typename T>
RCPtr<T>::RCPtr( const RCPtr& rhs )
    : pointee( rhs.pointee )
{
    init( );
}

template <typename T>
RCPtr<T>::~RCPtr( )
{
    if ( pointee ) pointee->removeRef( );
}

template <typename T>
RCPtr<T>& RCPtr<T>::operator=( const RCPtr& rhs )
{
    if ( pointee != rhs.pointee )
    {
        if ( pointee )
        {
            pointee->removeRef( );
            pointee = rhs.pointee;
            init( );
        }
    }
    return *this;
}

template <typename T>
T* RCPtr<T>::operator->( ) const
{
    return pointee;
}

template <typename T>
T& RCPtr<T>::operator*( ) const
{
    return *pointee;
}

template <typename T>
void RCPtr<T>::init( )
{
    if ( pointee == 0 ) return;
    if ( !pointee->isShareable( ) ) pointee = new T( *pointee );
    pointee->addRef( );
}

//*===================================================RCString==================================================*

class RCString
{
public:
    RCString( const char* initValue );
    const char& operator[]( int index ) const;
    char&       operator[]( int index );

private:
    struct StringValue : public RCObject
    {
        StringValue( const char* initValue );
        StringValue( const StringValue& rhs );
        ~StringValue( );
        void  init( const char* initValue );
        char* data;
    };

    RCPtr<StringValue> value;
};

RCString::RCString( const char* initValue )
    : value( new StringValue( initValue ) )
{
}

const char& RCString::operator[]( int index ) const { return value->data[index]; }

char& RCString::operator[]( int index )
{
    if ( value->isShared( ) )
    {
        value = new StringValue( value->data );
    }
    value->markUnShareable( );
    return value->data[index];
}

RCString::StringValue::StringValue( const char* initValue ) { init( initValue ); }

RCString::StringValue::~StringValue( ) { delete[] data; }

RCString::StringValue::StringValue( const StringValue& rhs ) { init( rhs.data ); }

void RCString::StringValue::init( const char* initValue )
{
    data = new char[strlen( initValue ) + 1];
    strcpy( data, initValue );
}

#pragma endregion

#pragma region Add ReferenceCounting to the library

//*===================================================RCIPtr==================================================*
template <typename T>
class RCIPtr
{
public:
    RCIPtr( T* realPtr = 0 );
    RCIPtr( const RCIPtr& rhs );
    ~RCIPtr( );
    RCIPtr&  operator=( const RCIPtr& rhs );
    const T* operator->( ) const;
    T*       operator->( );
    const T& operator*( ) const;
    T&       operator*( );

private:
    struct CountHolder : public RCObject
    {
        ~CountHolder( ) { delete pointee; }
        T* pointee;
    };
    CountHolder* counter;
    void         init( );
    void         makeCopy( );
};

template <typename T>
void RCIPtr<T>::init( )
{
    if ( counter->isShareable( ) == false )
    {
        T* oldValue = counter->pointee;
        counter = new CountHolder;
        counter->pointee = new T( *oldValue );
    }
    counter->addRef( );
}

template <typename T>
RCIPtr<T>::RCIPtr( T* realPtr )
    : counter( new CountHolder )
{
    counter->pointee = realPtr;
    init( );
}

template <typename T>
RCIPtr<T>::RCIPtr( const RCIPtr& rhs )
    : counter( rhs.counter )
{
    init( );
}

template <typename T>
RCIPtr<T>::~RCIPtr( )
{
    counter->removeRef( );
}

template <typename T>
RCIPtr<T>& RCIPtr<T>::operator=( const RCIPtr& rhs )
{
    if ( counter != rhs.counter )
    {
        counter->removeRef( );
        counter = rhs.counter;
        init( );
    }
    return *this;
}

template <typename T>
const T* RCIPtr<T>::operator->( ) const
{
    return counter->pointee;
}

template <typename T>
const T& RCIPtr<T>::operator*( ) const
{
    return *( counter->pointee );
}

template <typename T>
void RCIPtr<T>::makeCopy( )
{
    if ( counter->isShared( ) )
    {
        T* oldValue = counter->pointee;
        counter->removeRef( );
        counter = new CountHolder;
        counter->pointee = new T( *oldValue );
        counter->addRef( );
    }
}

template <typename T>
T* RCIPtr<T>::operator->( )    // COW
{
    makeCopy( );
    return counter->pointee;
}

template <typename T>
T& RCIPtr<T>::operator*( )    // COW
{
    makeCopy( );
    return *( counter->pointee );
}

class Widget    // 作为程序库中不可更改的类存在
{
public:
    Widget( int size );
    void doThis( );
    int  showThat( ) const;
};

//*===================================================RCWidget==================================================*
class RCWidget
{
public:
    RCWidget( int size )
        : value( new Widget( size ) )
    {
    }
    void doThis( ) { value->doThis( ); }
    void showThat( ) const { value->showThat( ); }

private:
    RCIPtr<Widget> value;
};
#pragma endregion
int main( ) { return 0; }
