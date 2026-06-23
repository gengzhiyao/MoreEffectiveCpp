#include <iostream>
#include <cstring>
/**
 * @brief 代理类 Proxy
 * 1. 实现自定义的二维数据数据访问
 * 2. 区分 operator[] 的读写动作
 *
 * 对于一个 proxy class 只有三件事可做：
 * (1) 生成这个代理类 (2) 为其赋值(写入)， proxy class 所代表的就是调用 operator[] 字符的左值运用 (3) 其他(读取)，代表右值运用
 */

#pragma region Array2D
// 实现二维数组
template <typename T>
class Array2D
{
private:
    class Array1D   // 代理类
    {
    private:
        T* m_row;

    public:
        T& operator[]( int y ) { return m_row[y]; }
        explicit Array1D( T* row ) noexcept
            : m_row( row )
        {
        }
    };

public:
    Array2D( int dim1, int dim2 )
    {
        m_dim1 = dim1;
        m_dim2 = dim2;
        data = new T*[dim1];
        for ( int i = 0; i < dim1; i++ )
        {
            data[i] = new T[dim2]{ };
        }
    }
    T& operator( )( int x, int y ) { return data[x][y]; }
    ~Array2D( )
    {
        for ( int i = 0; i < m_dim1; i++ )
        {
            delete[] data[i];
        }
        delete[] data;
    }

    Array1D operator[]( int x )
    {
        return Array1D{ data[x] };
    }    // 返回 Array1D临时变量，不必返回 Array1D& 

private:
    int m_dim1;
    int m_dim2;
    T** data;    // 作为实际存储的数据存在
};
#pragma endregion

#pragma region DistinguishReadWriteActions

// std::cout<<s[1]; 读取动作：左值运用
// s2[2]='c';  s3[3]=s2[1];  写入动作：右值运用
// 如何区分读写动作呢？？？

class NormalString
{
public:
    NormalString( const char* str )
    {
        m_str = new char[strlen( str ) + 1];
        strcpy( m_str, str );
    }
    char&       operator[]( int index ) { return m_str[index]; }
    const char& operator[]( int index ) const { return m_str[index]; }
    // const 无法区分读写动作！具体调用哪个函数，实际上是由对象是否是 const 对象决定的
private:
    char* m_str;
};

// 将调用动作延缓 直到知道 operator[] 的返回结果将如何被只用，这也是缓式评估的用处之一
class String
{
public:
    class CharProxy
    {
    private:
        int     m_index;
        String& theString;

    public:
        operator char( ) const { return theString.m_str[m_index]; }    // theString[m_index] 导致无限递归调用?

        CharProxy& operator=( char c )
        {
            theString.m_str[m_index] = c;
            return *this;
        }
        CharProxy( String& str, int index ) // operator[] 返回的每一个代理类都会记住其所属的字符串以及其所在的索引位置
            : m_index( index ),
              theString( str )
        {
        }
        CharProxy& operator=( const CharProxy& rhs )
        {
            theString.m_str[m_index] = rhs.theString.m_str[rhs.m_index];
            return *this;
        }
        char* operator&( ) { return &theString.m_str[m_index]; }
    };

    String( const char* str )
    {
        m_str = new char[strlen( str ) + 1];
        strcpy( m_str, str );
    }
    CharProxy operator[]( int index ) { return CharProxy{ *this, index }; }
    
    const CharProxy operator[]( int index ) const { return CharProxy{ const_cast<String&>( *this ), index }; }

    friend class CharProxy;
    const char* GetString( ) const { return m_str; }

private:
    char* m_str;
};
#pragma endregion

#pragma region EvaluationDiscussing

template <typename T>
class Array
{
public:
    class Proxy
    {
    public:
        Proxy( Array<T>& array, int index )
            : m_array( array ),
              m_index( index )
        {
        }

        // 返回代理类的引用
        Proxy& operator=( const T& rhs )
        {
            m_array.m_value[m_index] = rhs;
            return *this;
        }

        // 浣滀负鍙冲€硷細闅愬紡杞?鎹?涓哄簳灞傚厓绱犵殑鍊?
        operator T( ) const { return m_array.m_value[m_index]; }

    private:
        Array<T>& m_array;
        int       m_index;
    };
    const Proxy operator[]( int index ) const { return Proxy( const_cast<Array<T>&>( *this ), index ); }
    Proxy       operator[]( int index ) { return Proxy( *this, index ); }
    Array( int size )
        : m_size( size )
    {
        m_value = new T[m_size];
    }

private:
    std::size_t m_size;
    T*          m_value;
};

class Rational
{
public:
    Rational( int numerator = 0, int denominator = 1 )
        : m_numerator( numerator ),
          m_denominator( denominator )
    {
    }
    int numerator( ) const { return m_numerator; }
    int denominator( ) const { return m_denominator; }

private:
    int m_numerator;
    int m_denominator;
};

// 代理类无法取代真实对象的另一个方面是：当代理类转换为真正所代表的底层对象时，会有一个用户定制的类型转换函数调用
// 但是，编译器在将一个变量隐式转换为另一个变量时，只会有一次隐式转换
// 什么意思呢？
// 在非 explicit 构造函数中存在隐式转换，如果向其传入的变量本身就是 operator[] 返回的 Proxy ，那么就不会再次发生 Proxy 向 真实对象的转换了

#pragma endregion

int main( )
{
    // int data[dim1][dim2];   //! [ERROR]: 以变量作为数组的大小，不合法，在 GCC 下采用了变长数组扩展，但是在 MSVC 下就直接编译不通过
    // int* arr=new int[10][20];   //![ERROR]: new int[10][20] 得到的类型是 int(*)[20] ，不能直接赋值给一级指针 int*
    Array2D<int> arr( 10, 20 );
    std::cout << arr( 0, 0 ) << std::endl;    
    arr[5][5] = 100;
    std::cout << arr[5][5] << std::endl;

    String s1( "chartest" );
    std::cout << s1[1] << std::endl;    // 左值运用

    String s2( "needtocopy" );
    s1[3] = s2[0];
    std::cout << s1.GetString( ) << std::endl;  // 右值运用

    char* pChar = &s1[1];
    *pChar = 'c';   // Notion:限制：返回的是代理类，如果代理类中不重载 operator& 那么编译就不会通过
    std::cout << s1.GetString( ) << std::endl;  

    // Using Proxy
    Array<int> array( 5 );
    array[0] = 1;
    // array[1] += 2;    // array[2]++; // Notion:限制： operator[] 返回的代理类的引用，如果代理类中不重载 += ++ ...那么也会编译不过

    // Notion:限制：通过代理类调用真实对象的成员函数
    Array<Rational> ratArray( 2 );
    // ratArray[0].numerator( );    //! [ERROR] :

    // std::swap<int>( array[1], array[2] );    //! [ERROR] : swap 想要的是int&，但是 operator[] 返回的是代理类
    return 0;
}