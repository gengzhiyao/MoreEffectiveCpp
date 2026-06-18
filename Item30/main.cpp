#include <iostream>
#include <cstring>
/**
 * @brief ´úÀíÀà Proxy
 * 1. ÊµÏÖ×Ô¶¨ÒåµÄ¶şÎ¬Êı¾İÊı¾İ·ÃÎÊ
 * 2. Çø·Ö operator[] µÄ¶ÁĞ´¶¯×÷
 *
 * ¶ÔÓÚÒ»¸ö proxy class Ö»ÓĞÈı¼şÊÂ¿É×ö£º
 * (1) Éú³ÉÕâ¸ö´úÀíÀà (2) ÎªÆä¸³Öµ(Ğ´Èë)£¬ proxy class Ëù´ú±íµÄ¾ÍÊÇµ÷ÓÃ operator[] ×Ö·ûµÄ×óÖµÔËÓÃ (3) ÆäËû(¶ÁÈ¡)£¬´ú±íÓÒÖµÔËÓÃ
 */

#pragma region Array2D
template <typename T>
class Array2D
{
private:
    class Array1D   // ï¿½Ã»ï¿½ï¿½ï¿½ï¿½ï¿½ÒªÖªï¿½ï¿½ Array1D ï¿½Ä´ï¿½ï¿½Ú£ï¿½ï¿½Ó¹ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ëµï¿½ï¿½Array1D ï¿½ï¿½ï¿½Ã»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
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

    Array1D operator[]( int x ) { return Array1D{ data[x] }; }  // ï¿½ï¿½ï¿½Øµï¿½ï¿½ï¿½Ê±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ğ£ï¿½ï¿½Ñ¾ï¿½ï¿½ï¿½Â¼ï¿½ï¿½ï¿½Ğºï¿½(Êµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö¸ï¿½ï¿½Ò»Î¬ï¿½ï¿½ï¿½ï¿½ï¿½Ö¸ï¿½ï¿?)ï¿½ï¿½ï¿½ï¿½ï¿½Ø·ï¿½ï¿½ï¿½ Array1D& ï¿½ï¿½ï¿½ï¿½

private:
    int m_dim1;
    int m_dim2;
    T** data;   // ï¿½ï¿½Ò»ï¿½ï¿½Î¬ï¿½ï¿½ = ï¿½Ğ£ï¿½ï¿½Ú¶ï¿½ï¿½ï¿½Î¬ï¿½ï¿½ = ï¿½ï¿½ ï¿½È´ï¿½ï¿½ï¿½ï¿½ï¿½ 0 ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ ï¿½Ù´ï¿½ï¿? 1 ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ğ¡ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
};
#pragma endregion

#pragma region DistinguishReadWriteActions

// std::cout<<s[1]; ï¿½ï¿½È¡ï¿½ï¿½ï¿½ï¿½
// s2[2]='c';  s3[3]=s2[1];  Ğ´ï¿½ë¶¯ï¿½ï¿½
// ï¿½É´ï¿½ï¿½ï¿½ï¿½ï¿½Ó¹ï¿?

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
    // ï¿½ï¿½ï¿½Ç£ï¿½ï¿½ï¿½ï¿½Ï²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö¶ï¿½Ğ´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ä¸ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ç¸ï¿½ï¿½İ¶ï¿½ï¿½ï¿½ï¿½Ç·ï¿½ï¿½ï¿½ const ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
private:
    char* m_str;
};

// ï¿½ï¿½ï¿½ï¿½Òªï¿½Ä´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½İ»ï¿½ï¿½ï¿½Ö±ï¿½ï¿½Öªï¿½ï¿½ operator[] ï¿½ï¿½ï¿½Ø½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Î±ï¿½Ê¹ï¿½ï¿½ÎªÖ¹ Ò²ï¿½Ç»ï¿½Ê½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö®Ò»
class String
{
public:
    class CharProxy
    {
    private:
        int     m_index;
        String& theString;

    public:
        operator char( ) const { return theString.m_str[m_index]; } // theString[m_index] ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Şµİ¹ï¿½ï¿½ï¿½ï¿?

        CharProxy& operator=( char c )
        {
            theString.m_str[m_index] = c;
            return *this;
        }
        CharProxy( String& str, int index )
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
    // const é‡è½½ï¼šé€šè¿‡ const_cast å¤ç”¨ CharProxyï¼ˆå†™ä¿æŠ¤ç”±è¿”å›çš„ const CharProxy æä¾›ï¼?
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

        // ä½œä¸ºå·¦å€¼ï¼šè¢?èµ‹å€¼æ—¶å†™å…¥åº•å±‚æ•°ç»„
        Proxy& operator=( const T& rhs )
        {
            m_array.m_value[m_index] = rhs;
            return *this;
        }

        // ä½œä¸ºå³å€¼ï¼šéšå¼è½?æ?ä¸ºåº•å±‚å…ƒç´ çš„å€?
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

#pragma endregion

int main( )
{
    // int data[dim1][dim2];   //! [ERROR]: ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½C++ï¿½ï¿½×¼ï¿½ï·¨ï¿½ï¿½ï¿½ï¿½ MSVC ï¿½Ğ±ï¿½ï¿½ë²»Í¨ï¿½ï¿½ï¿½ï¿½C++ ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Î?ï¿½È±ï¿½ï¿½ï¿½ï¿½Ú±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Öª
    // int* arr=new int[10][20];   //![ERROR]: ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
    Array2D<int> arr( 10, 20 );
    std::cout << arr( 0, 0 ) << std::endl;    // ï¿½ï¿½Êµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ğµï¿½Ô?ï¿½Ø£ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ã£ï¿½ï¿½ï¿½È»ï¿½ï¿½ï¿½ï¿½.
    arr[5][5] = 100;
    std::cout << arr[5][5] << std::endl;

    String s1( "chartest" );
    std::cout << s1[1] << std::endl;

    String s2( "needtocopy" );
    s1[3] = s2[0];
    std::cout << s1.GetString( ) << std::endl;

    char* pChar = &s1[1];
    *pChar = 'c';
    std::cout << s1.GetString( ) << std::endl;

    // Using Proxy
    Array<int> array( 5 );
    array[0] = 1;
    // array[1] += 2;    // array[2]++;

    Array<Rational> ratArray( 2 );
    // ratArray[0].numerator( );    //! [ERROR] :
    // std::swap<int>( array[1], array[2] );    //! [ERROR] :
    return 0;
}